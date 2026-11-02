/*
 * C++ file for the Ada names of a translated program.
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

// Project header for the name table.
#include "cspcc/emit_name_table.h"

// Standard headers for case folding.
#include <algorithm>
#include <cctype>

// Namespace for cspcc's Ada emitter.
namespace cspcc
{

    // Namespace for the name table's word lists.
    namespace
    {

        // Ada 2022's reserved words.
        constexpr std::string_view emitReservedWords[] = {
            "abort",     "abs",        "abstract",     "accept",   "access", "aliased",
            "all",       "and",        "array",        "at",       "begin",  "body",
            "case",      "constant",   "declare",      "delay",    "delta",  "digits",
            "do",        "else",       "elsif",        "end",      "entry",  "exception",
            "exit",      "for",        "function",     "generic",  "goto",   "if",
            "in",        "interface",  "is",           "limited",  "loop",   "mod",
            "new",       "not",        "null",         "of",       "or",     "others",
            "out",       "overriding", "package",      "parallel", "pragma", "private",
            "procedure", "protected",  "raise",        "range",    "record", "rem",
            "renames",   "requeue",    "return",       "reverse",  "select", "separate",
            "some",      "subtype",    "synchronized", "tagged",   "task",   "terminate",
            "then",      "type",       "until",        "use",      "when",   "while",
            "with",      "xor",
        };

        // Fold a name to lower case.
        std::string lower(std::string_view text)
        {
            std::string result(text);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            return result;
        }

    }

    // Take the reserved words.
    EmitNameTable::EmitNameTable()
    {
        for (std::string_view word : emitReservedWords) {
            reserve(word);
        }
    }

    // Take a name.
    void EmitNameTable::reserve(std::string_view name)
    {
        m_taken.insert(lower(name));
    }

    // Check a name.
    bool EmitNameTable::taken(std::string_view name) const
    {
        return m_taken.count(lower(name)) != 0;
    }

    // Take the sanitized name, or the first free numbered one.
    std::string EmitNameTable::make(std::string_view base)
    {
        std::string name = sanitize(base);
        std::string candidate = name;
        for (int n = 2; taken(candidate); n++) {
            candidate = name + "_" + std::to_string(n);
        }
        reserve(candidate);
        return candidate;
    }

    // Drop underscores Ada forbids and start with a letter.
    std::string EmitNameTable::sanitize(std::string_view name)
    {
        std::string result;
        for (char c : name) {
            if (c == '_' && (result.empty() || result.back() == '_')) {
                continue;
            }
            result += c;
        }
        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }
        if (result.empty() || !std::isalpha(static_cast<unsigned char>(result[0]))) {
            result = "V_" + result;
        }
        return result;
    }

}
