/*
 * C++ header for a CSP process family.
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

#ifndef CSPCC_DATA_FAMILY_H
#define CSPCC_DATA_FAMILY_H

#include "cspcc/data/data_entry.h"
#include "cspcc/data/data_instance.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_variant.h"

#include <memory>
#include <string>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes the processes sharing a name, such as Phil(0) to Phil(4). */
        struct DataFamily
        {
            /** Name of the processes. */
            std::string m_name;

            /** Position of the first definition. */
            DataLocation m_location;

            /** Number of subscripts each process has. */
            int m_arity = -1;

            /** Definitions of the family. */
            std::vector<std::unique_ptr<DataVariant>> m_variants;

            /** Processes of the family. */
            std::vector<DataInstance> m_instances;

            /** Messages the family receives. */
            std::vector<std::unique_ptr<DataEntry>> m_entries;

            /** Number of the family's first process. */
            int m_first = 0;

            /** Number of the family's last process. */
            int m_last = 0;

            /** Smallest subscript in each dimension. */
            std::vector<long long> m_lows;

            /** Largest subscript in each dimension. */
            std::vector<long long> m_highs;

            /** Prefix of the family's Ada names. */
            std::string m_ada;
        };

    }
}

#endif
