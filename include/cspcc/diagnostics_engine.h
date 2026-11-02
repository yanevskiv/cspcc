/*
 * C++ header for cspcc's diagnostics.
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

#ifndef CSPCC_DIAGNOSTICS_ENGINE_H
#define CSPCC_DIAGNOSTICS_ENGINE_H

#include "cspcc/data/data_location.h"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Reports errors, warnings and notes about one CSP file. */
    class DiagnosticsEngine
    {
    public:
        /**
         * Prepare diagnostics for one file.
         *
         * @param path Name the diagnostics print.
         * @param source Text of the file, for quoting its lines.
         * @param stream Destination of the diagnostics.
         */
        DiagnosticsEngine(std::string path, std::string_view source, std::ostream &stream);

        /**
         * Report an error.
         *
         * @param location Position the error is about.
         * @param code Code of the error, such as ERR_PARSE_EXPECTED.
         * @param message Description of the error.
         */
        void error(const data::DataLocation &location, std::string_view code,
                   std::string_view message);

        /**
         * Report a warning, unless warnings are off.
         *
         * @param location Position the warning is about.
         * @param code Code of the warning, such as WARN_SEMA_UNKNOWN_TYPE.
         * @param message Description of the warning.
         */
        void warning(const data::DataLocation &location, std::string_view code,
                     std::string_view message);

        /**
         * Add a note to the last error or warning.
         *
         * @param location Position the note is about.
         * @param message Text of the note.
         */
        void note(const data::DataLocation &location, std::string_view message);

        /**
         * Turn warnings off or into errors.
         *
         * @param enabled Whether warnings are reported.
         * @param asErrors Whether warnings are reported as errors.
         */
        void set_warnings(bool enabled, bool asErrors);

        /**
         * Count the errors reported so far.
         *
         * @return Number of errors.
         */
        int error_count() const;

        /**
         * Return the file's name.
         *
         * @return Name the diagnostics print.
         */
        const std::string &path() const;

    private:
        /**
         * Print one diagnostic.
         *
         * @param location Position the diagnostic is about.
         * @param level Level printed, such as error.
         * @param code Code printed in brackets, or empty.
         * @param message Description printed.
         */
        void print(const data::DataLocation &location, std::string_view level,
                   std::string_view code, std::string_view message);

        /**
         * Print a location's line with a caret under its column.
         *
         * @param location Position to quote.
         */
        void quote(const data::DataLocation &location);

        /** Name the diagnostics print. */
        std::string m_path;

        /** Lines of the file. */
        std::vector<std::string> m_lines;

        /** Destination of the diagnostics. */
        std::ostream &m_stream;

        /** Number of errors reported. */
        int m_errors = 0;

        /** Whether warnings are reported. */
        bool m_warnings_enabled = true;

        /** Whether warnings are reported as errors. */
        bool m_warnings_as_errors = false;

        /** Whether the last error or warning was printed, so its notes are too. */
        bool m_last_shown = true;
    };

}

#endif
