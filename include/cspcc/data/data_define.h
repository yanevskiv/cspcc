/*
 * C++ header for a constant given on the command line.
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

#ifndef CSPCC_DATA_DEFINE_H
#define CSPCC_DATA_DEFINE_H

#include <string>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a -D NAME=VALUE option. */
        struct DataDefine
        {
            /** Name of the constant. */
            std::string m_name;

            /** Value as written. */
            std::string m_value;
        };

    }
}

#endif
