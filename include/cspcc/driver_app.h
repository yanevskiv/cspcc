/*
 * C++ header for the cspcc driver.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * cspcc  is  free  software;  you  can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * cspcc  is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with cspcc; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSPCC_DRIVER_APP_H
#define CSPCC_DRIVER_APP_H

#include "cspcc/data/data_program.h"
#include "cspcc/driver_options.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Runs one cspcc invocation, from the CSP file to the Ada files. */
    class DriverApp
    {
    public:
        /**
         * Keep the command line.
         *
         * @param argc Number of arguments, the program's name included.
         * @param argv Arguments, the program's name first.
         */
        DriverApp(int argc, const char *const *argv);

        /**
         * Run the invocation.
         *
         * @return 0 on success, 1 after errors in the program, 2 after a bad command line.
         */
        int run();

    private:
        /**
         * Translate the input, already read.
         *
         * @param source Text of the input.
         * @return Exit status.
         */
        int translate(const std::string &source);

        /**
         * Write the Ada main procedure and, unless --no-runtime, the runtime next to it.
         *
         * @param program Checked program.
         * @return Exit status.
         */
        int emit(data::DataProgram &program);

        /**
         * Read a whole file.
         *
         * @param path File to read.
         * @param text Destination of the contents.
         * @return Whether the file could be read.
         */
        static bool read_file(const std::string &path, std::string &text);

        /**
         * Write a file, leaving it untouched when it already holds the text.
         *
         * @param path File to write.
         * @param text Contents.
         * @return Whether the file holds the text afterwards.
         */
        static bool write_file(const std::filesystem::path &path, std::string_view text);

        /**
         * Spell a unit's file name as an Ada name.
         *
         * @param unit Lower-case unit name, such as dining_philosophers.
         * @return Capitalized name, such as Dining_Philosophers.
         */
        static std::string ada_name(std::string_view unit);

        /** Name the program was run as. */
        std::string m_program;

        /** Arguments after the program's name. */
        std::vector<const char *> m_args;

        /** Parsed options. */
        DriverOptions m_options;
    };

}

#endif
