// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_TEST_H
#define CSPCC_TEST_H

#include "checker.h"
#include "diagnostics.h"
#include "emitter.h"
#include "lexer.h"
#include "parser.h"
#include "runtime.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <vector>

// A CSP program translated to Ada, or the diagnostics that stopped it.
struct Translation
{
    // Whether the program translated.
    bool m_ok = false;

    // Errors and warnings, as cspcc prints them.
    std::string m_messages;

    // The Ada main procedure.
    std::string m_ada;
};

// A translated program's run.
struct Run
{
    // Exit status.
    int m_status = -1;

    // Standard output.
    std::string m_output;

    // Standard error.
    std::string m_error;

    // cspcc's warnings.
    std::string m_messages;
};

// Check whether a text holds a piece.
inline bool contains(const std::string &text, std::string_view piece)
{
    return text.find(piece) != std::string::npos;
}

// Split a text into lines.
inline std::vector<std::string> lines(const std::string &text)
{
    std::vector<std::string> result;
    std::istringstream stream(text);
    for (std::string line; std::getline(stream, line);) {
        result.push_back(line);
    }
    return result;
}

// Split a text into lines, sorted.
inline std::vector<std::string> sorted_lines(const std::string &text)
{
    std::vector<std::string> result = lines(text);
    std::sort(result.begin(), result.end());
    return result;
}

// Read a whole file.
inline std::string read_file(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Read examples/NAME.csp.
inline std::string example(const std::string &name)
{
    return read_file("examples/" + name + ".csp");
}

// Translate a CSP program to the Ada unit unit, naming the source file in messages.
inline Translation translate(std::string_view source, const std::vector<Define> &defines = {},
                             const std::string &unit = "test", const std::string &file = "test.csp")
{
    Translation result;
    std::ostringstream messages;
    Diagnostics diagnostics(file, source, messages);
    std::vector<Token> tokens = Lexer(source, diagnostics).tokenize();
    if (!diagnostics.error_count()) {
        std::unique_ptr<Ast> ast = Parser(tokens, diagnostics).parse_program();
        if (ast) {
            std::unique_ptr<Program> program = Checker(diagnostics).check(*ast, defines);
            if (program) {
                result.m_ada = AdaWriter(*program, unit, file).write();
                result.m_ok = true;
            }
        }
    }
    result.m_messages = messages.str();
    return result;
}

// Compile the Ada runtime once into obj/tests/runtime, for every program to use.
inline bool build_runtime()
{
    std::filesystem::path directory = "obj/tests/runtime";
    std::filesystem::create_directories(directory);
    std::ofstream(directory / "csp_runtime.ads") << runtime_spec();
    std::ofstream(directory / "csp_runtime.adb") << runtime_body();
    return std::system("cd obj/tests/runtime && gcc -c -gnat2022 csp_runtime.adb > build 2>&1") ==
           0;
}

// Translate a program into obj/tests/NAME, build it with GNAT and run it with arguments.
inline Run run(const std::string &name, std::string_view source, const std::string &arguments = "",
               const std::vector<Define> &defines = {})
{
    Run result;
    std::string unit = "t_" + name;
    Translation translation = translate(source, defines, unit, name + ".csp");
    result.m_messages = translation.m_messages;
    INFO(translation.m_messages);
    REQUIRE(translation.m_ok);
    std::filesystem::path directory = std::filesystem::path("obj/tests") / name;
    std::filesystem::create_directories(directory);
    std::ofstream(directory / (unit + ".adb")) << translation.m_ada;
    static bool runtimeBuilt = build_runtime();
    REQUIRE(runtimeBuilt);
    std::string cd = "cd '" + directory.string() + "' && ";
    int built = std::system(
        (cd + "gnatmake -q -gnat2022 -aI../runtime -aO../runtime " + unit + ".adb > build 2>&1")
            .c_str());
    INFO(read_file(directory / "build"));
    REQUIRE(built == 0);
    int status =
        std::system((cd + "timeout 30 ./" + unit + " " + arguments + " > output 2> error").c_str());
    result.m_status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    result.m_output = read_file(directory / "output");
    result.m_error = read_file(directory / "error");
    return result;
}

#endif
