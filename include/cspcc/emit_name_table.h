// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMIT_NAME_TABLE_H
#define CSPCC_EMIT_NAME_TABLE_H

#include <set>
#include <string>
#include <string_view>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Hands out Ada names that clash neither with Ada's reserved words nor with each other.
    class EmitNameTable
    {
    public:
        // Start with Ada's reserved words taken.
        EmitNameTable();

        // Take a name as it is.
        void reserve(std::string_view name);

        // Check whether a name is taken, ignoring case as Ada does.
        bool taken(std::string_view name) const;

        // Take a name made from a CSP name, adding _2, _3 and so on until it is free.
        std::string make(std::string_view base);

        // Turn a CSP name into a valid Ada identifier.
        static std::string sanitize(std::string_view name);

    private:
        // Names taken, in lower case.
        std::set<std::string> m_taken;
    };

}

#endif
