# CMake configuration for the cspcc compiler.
#
# Copyright (C) 2026 Ivan Janevski
#
# cspcc is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# cspcc is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with cspcc; if not, see
# <https://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.20)

project(cspcc VERSION 0.1.0 LANGUAGES CXX)

# Build defaults.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(
        CMAKE_INSTALL_PREFIX
        "${CMAKE_CURRENT_SOURCE_DIR}/build/install"
        CACHE PATH "Installation directory"
        FORCE
    )
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)

# The Ada runtime, embedded as raw strings that cspcc writes next to its output.
set(CSPCC_RUNTIME_SPEC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/runtime/csp_runtime.ads")
set(CSPCC_RUNTIME_BODY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/runtime/csp_runtime.adb")
set_property(
    DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CSPCC_RUNTIME_SPEC_FILE}" "${CSPCC_RUNTIME_BODY_FILE}"
)
file(READ "${CSPCC_RUNTIME_SPEC_FILE}" CSPCC_RUNTIME_SPEC)
file(READ "${CSPCC_RUNTIME_BODY_FILE}" CSPCC_RUNTIME_BODY)
configure_file(
    cmake/emit_runtime_sources.inc.in
    "${CMAKE_BINARY_DIR}/generated/cspcc/emit_runtime_sources.inc"
    @ONLY
)

# The compiler as a library, shared by the driver and the unit tests.
file(
    GLOB CSPCC_CORE_SOURCES
    CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/cspcc/*.cpp"
)
add_library(cspcc-core STATIC ${CSPCC_CORE_SOURCES})
target_compile_features(cspcc-core PUBLIC cxx_std_20)
target_compile_options(cspcc-core PRIVATE -Wall -Wextra)
target_include_directories(
    cspcc-core
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_BINARY_DIR}/generated"
)

# The driver.
add_executable(cspcc src/main.cpp)
target_compile_options(cspcc PRIVATE -Wall -Wextra)
target_link_libraries(cspcc PRIVATE cspcc-core)
target_compile_definitions(cspcc-core PRIVATE CSPCC_VERSION="${PROJECT_VERSION}")
set_target_properties(cspcc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
install(TARGETS cspcc RUNTIME DESTINATION bin)

# The slides' examples, translated and built with GNAT.
find_program(CSPCC_GNATMAKE gnatmake)
file(GLOB CSPCC_EXAMPLES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/examples/*.csp")
set(CSPCC_EXAMPLE_TARGETS)
foreach(example IN LISTS CSPCC_EXAMPLES)
    get_filename_component(name "${example}" NAME_WE)
    set(dir "${CMAKE_BINARY_DIR}/examples/${name}")
    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/bin/${name}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${dir}"
        COMMAND cspcc -o "${dir}/${name}.adb" "${example}"
        COMMAND "${CSPCC_GNATMAKE}" -q -D "${dir}" "${dir}/${name}.adb"
                -o "${CMAKE_BINARY_DIR}/bin/${name}"
        WORKING_DIRECTORY "${dir}"
        DEPENDS cspcc "${example}"
        VERBATIM
    )
    list(APPEND CSPCC_EXAMPLE_TARGETS "${CMAKE_BINARY_DIR}/bin/${name}")
endforeach()
add_custom_target(examples DEPENDS ${CSPCC_EXAMPLE_TARGETS})

# Unit tests and CSP tests.
if(BUILD_TESTING)
    set(CSPCC_CATCH2_SOURCE_DIR "" CACHE PATH "A local Catch2 source tree to use instead of fetching")
    if(CSPCC_CATCH2_SOURCE_DIR)
        add_subdirectory("${CSPCC_CATCH2_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/catch2" EXCLUDE_FROM_ALL)
        list(APPEND CMAKE_MODULE_PATH "${CSPCC_CATCH2_SOURCE_DIR}/extras")
    else()
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(Catch2)
        list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
    endif()

    file(
        GLOB_RECURSE CSPCC_UNIT_TEST_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/*.cpp"
    )
    add_executable(cspcc-tests ${CSPCC_UNIT_TEST_SOURCES})
    set_target_properties(cspcc-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    target_link_libraries(cspcc-tests PRIVATE cspcc-core Catch2::Catch2WithMain)
    include(Catch)
    catch_discover_tests(cspcc-tests WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    file(
        GLOB CSPCC_CSP_TESTS
        CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/lang/*.csp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/errs/*.csp"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/examples/*.csp"
    )
    foreach(test IN LISTS CSPCC_CSP_TESTS)
        file(RELATIVE_PATH name "${CMAKE_CURRENT_SOURCE_DIR}/tests" "${test}")
        add_test(
            NAME "csp/${name}"
            COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_test" "${test}"
        )
        set_tests_properties(
            "csp/${name}"
            PROPERTIES
                ENVIRONMENT "CSPCC=$<TARGET_FILE:cspcc>;CSPCC_TEST_DIR=${CMAKE_BINARY_DIR}/csp-tests"
                TIMEOUT 120
        )
    endforeach()
endif()
