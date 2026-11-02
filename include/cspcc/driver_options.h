/*
 * C++ header for cspcc's command-line options.
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

#ifndef CSPCC_DRIVER_OPTIONS_H
#define CSPCC_DRIVER_OPTIONS_H

#include "cspcc/data/data_define.h"

#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Parses the options of one cspcc invocation. */
    class DriverOptions
    {
    public:
        /**
         * Parse the arguments after the program's name.
         *
         * @param args Command-line arguments.
         * @param error Destination for a description of a bad argument.
         * @return Whether every argument was valid.
         */
        bool parse(std::span<const char *const> args, std::string &error);

        /**
         * Print the options cspcc takes.
         *
         * @param stream Destination of the help.
         * @param program Name the program was run as.
         */
        static void print_help(std::ostream &stream, std::string_view program);

        /**
         * Return the CSP file to translate.
         *
         * @return Path of the input, empty when none was given.
         */
        const std::string &input() const;

        /**
         * Return the Ada file to write.
         *
         * @return Path given with -o, empty for the default.
         */
        const std::string &output() const;

        /**
         * Return the constants given with -D.
         *
         * @return Names and values in command-line order.
         */
        const std::vector<data::DataDefine> &defines() const;

        /**
         * Report whether help was requested.
         *
         * @return True when -h or --help was given.
         */
        bool wants_help() const;

        /**
         * Report whether the version was requested.
         *
         * @return True when --version was given.
         */
        bool wants_version() const;

        /**
         * Report whether the tokens should be printed instead of translated.
         *
         * @return True when --dump-tokens was given.
         */
        bool dumps_tokens() const;

        /**
         * Report whether the syntax tree should be printed instead of translated.
         *
         * @return True when --dump-ast was given.
         */
        bool dumps_ast() const;

        /**
         * Report whether the Ada runtime should be written next to the output.
         *
         * @return False when --no-runtime was given.
         */
        bool writes_runtime() const;

        /**
         * Report whether warnings are reported.
         *
         * @return False when -w was given.
         */
        bool warnings_enabled() const;

        /**
         * Report whether warnings are errors.
         *
         * @return True when -Werror was given.
         */
        bool warnings_as_errors() const;

    private:
        /**
         * Record one -D value.
         *
         * @param text NAME=VALUE as written.
         * @param error Destination for a description of a bad value.
         * @return Whether the value was well-formed.
         */
        bool add_define(std::string_view text, std::string &error);

        /** CSP file to translate. */
        std::string m_input;

        /** Ada file to write, empty for the default. */
        std::string m_output;

        /** Constants given with -D. */
        std::vector<data::DataDefine> m_defines;

        /** Whether help was requested. */
        bool m_wants_help = false;

        /** Whether the version was requested. */
        bool m_wants_version = false;

        /** Whether the tokens are printed. */
        bool m_dumps_tokens = false;

        /** Whether the syntax tree is printed. */
        bool m_dumps_ast = false;

        /** Whether the Ada runtime is written. */
        bool m_writes_runtime = true;

        /** Whether warnings are reported. */
        bool m_warnings_enabled = true;

        /** Whether warnings are errors. */
        bool m_warnings_as_errors = false;
    };

}

#endif
