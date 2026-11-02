/*
 * C++ header for one process of a CSP process family.
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

#ifndef CSPCC_DATA_INSTANCE_H
#define CSPCC_DATA_INSTANCE_H

#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes one process: its subscripts, its body and its number. */
        struct DataInstance
        {
            /** Subscripts of the process. */
            std::vector<long long> m_indices;

            /** Variant whose commands the process runs, from 1. */
            int m_variant = 0;

            /** Number of the process in the whole program, from 1. */
            int m_pid = 0;
        };

    }
}

#endif
