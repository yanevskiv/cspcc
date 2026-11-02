// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/emit_name_table.h"

#include <algorithm>
#include <cctype>

namespace cspcc
{

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
