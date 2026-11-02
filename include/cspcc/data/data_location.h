/*
 * C++ header for a position in a CSP source file.
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

#ifndef CSPCC_DATA_LOCATION_H
#define CSPCC_DATA_LOCATION_H

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a position in a CSP source file. */
        struct DataLocation
        {
            /** Line counted from 1, or 0 when there is no position. */
            int m_line = 0;

            /** Column in characters counted from 1. */
            int m_column = 0;
        };

    }
}

#endif
