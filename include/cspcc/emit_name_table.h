/*
 * C++ header for the Ada names of a translated program.
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

#ifndef CSPCC_EMIT_NAME_TABLE_H
#define CSPCC_EMIT_NAME_TABLE_H

#include <set>
#include <string>
#include <string_view>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Hands out Ada names that clash neither with Ada's reserved words nor with each other. */
    class EmitNameTable
    {
    public:
        /**
         * Start with Ada's reserved words taken.
         */
        EmitNameTable();

        /**
         * Take a name as it is.
         *
         * @param name Name to take.
         */
        void reserve(std::string_view name);

        /**
         * Check whether a name is taken, ignoring case as Ada does.
         *
         * @param name Name to check.
         * @return Whether it is taken.
         */
        bool taken(std::string_view name) const;

        /**
         * Take a name made from a CSP name, adding _2, _3 and so on until it is free.
         *
         * @param base CSP name to start from.
         * @return The name taken.
         */
        std::string make(std::string_view base);

        /**
         * Turn a CSP name into a valid Ada identifier.
         *
         * @param name CSP name.
         * @return A name with no leading, trailing or doubled underscore.
         */
        static std::string sanitize(std::string_view name);

    private:
        /** Names taken, in lower case. */
        std::set<std::string> m_taken;
    };

}

#endif
