// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DIAGNOSTICS_ENGINE_H
#define CSPCC_DIAGNOSTICS_ENGINE_H

#include "cspcc/token.h"

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Reports errors, warnings and notes about one CSP file.
    class DiagnosticsEngine
    {
    public:
        // Prepare diagnostics for one file.
        DiagnosticsEngine(std::string path, std::string_view source, std::ostream &stream);

        // Report an error.
        void error(const Location &location, std::string_view code,
                   std::string_view message);

        // Report a warning, unless warnings are off.
        void warning(const Location &location, std::string_view code,
                     std::string_view message);

        // Add a note to the last error or warning.
        void note(const Location &location, std::string_view message);

        // Turn warnings off or into errors.
        void set_warnings(bool enabled, bool asErrors);

        // Count the errors reported so far.
        int error_count() const;

        // Return the file's name.
        const std::string &path() const;

        // Return the destination of the diagnostics.
        std::ostream &stream() const;

    private:
        // Print one diagnostic.
        void print(const Location &location, std::string_view level,
                   std::string_view code, std::string_view message);

        // Print a location's line with a caret under its column.
        void quote(const Location &location);

        // Name the diagnostics print.
        std::string m_path;

        // Lines of the file.
        std::vector<std::string> m_lines;

        // Destination of the diagnostics.
        std::ostream &m_stream;

        // Number of errors reported.
        int m_errors = 0;

        // Whether warnings are reported.
        bool m_warnings_enabled = true;

        // Whether warnings are reported as errors.
        bool m_warnings_as_errors = false;

        // Whether the last error or warning was printed, so its notes are too.
        bool m_last_shown = true;
    };

}

#endif
