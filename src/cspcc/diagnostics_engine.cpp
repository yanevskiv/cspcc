/*
 * C++ file for cspcc's diagnostics.
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

// Project header for the diagnostics.
#include "cspcc/diagnostics_engine.h"

// Standard header for moving the path.
#include <utility>

// Namespace for cspcc's diagnostics.
namespace cspcc
{

    // Split the source into lines for quoting.
    DiagnosticsEngine::DiagnosticsEngine(std::string path, std::string_view source,
                                         std::ostream &stream)
        : m_path(std::move(path))
        , m_stream(stream)
    {
        std::string line;
        for (char c : source) {
            if (c == '\n') {
                m_lines.push_back(line);
                line.clear();
            } else {
                line += c;
            }
        }
        m_lines.push_back(line);
    }

    // Report an error.
    void DiagnosticsEngine::error(const data::DataLocation &location, std::string_view code,
                                  std::string_view message)
    {
        m_errors++;
        m_last_shown = true;
        print(location, "error", code, message);
    }

    // Report a warning, or an error under -Werror.
    void DiagnosticsEngine::warning(const data::DataLocation &location, std::string_view code,
                                    std::string_view message)
    {
        m_last_shown = m_warnings_enabled;
        if (!m_warnings_enabled) {
            return;
        }
        if (m_warnings_as_errors) {
            error(location, code, message);
            return;
        }
        print(location, "warning", code, message);
    }

    // Add a note to the last diagnostic that was printed.
    void DiagnosticsEngine::note(const data::DataLocation &location, std::string_view message)
    {
        if (m_last_shown) {
            print(location, "note", "", message);
        }
    }

    // Set the warning switches.
    void DiagnosticsEngine::set_warnings(bool enabled, bool asErrors)
    {
        m_warnings_enabled = enabled;
        m_warnings_as_errors = asErrors;
    }

    // Count the errors.
    int DiagnosticsEngine::error_count() const
    {
        return m_errors;
    }

    // Return the file's name.
    const std::string &DiagnosticsEngine::path() const
    {
        return m_path;
    }

    // Return the destination of the diagnostics.
    std::ostream &DiagnosticsEngine::stream() const
    {
        return m_stream;
    }

    // Print the position, the level, the message and the code, then the quoted line.
    void DiagnosticsEngine::print(const data::DataLocation &location, std::string_view level,
                                  std::string_view code, std::string_view message)
    {
        m_stream << m_path << ':';
        if (location.m_line > 0) {
            m_stream << location.m_line << ':' << location.m_column << ':';
        }
        m_stream << ' ' << level << ": " << message;
        if (!code.empty()) {
            m_stream << " [" << code << ']';
        }
        m_stream << '\n';
        quote(location);
    }

    // Print the line under the diagnostic with a caret, keeping its tabs.
    void DiagnosticsEngine::quote(const data::DataLocation &location)
    {
        if (location.m_line < 1 || location.m_line > static_cast<int>(m_lines.size())) {
            return;
        }
        const std::string &text = m_lines[location.m_line - 1];
        std::string caret;
        int column = 1;
        for (size_t i = 0; i < text.size() && column < location.m_column; i++) {
            unsigned char c = text[i];
            if ((c & 0xC0) == 0x80) {
                continue;
            }
            caret += c == '\t' ? '\t' : ' ';
            column++;
        }
        m_stream << "    " << text << "\n    " << caret << "^\n";
    }

}
