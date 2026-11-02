/*
 * C++ header for an Ada entry of a translated CSP process.
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

#ifndef CSPCC_DATA_ENTRY_H
#define CSPCC_DATA_ENTRY_H

#include "cspcc/data/data_type_base.h"

#include <string>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes one kind of message a process family receives. */
        struct DataEntry
        {
            /** Constructor of the message, empty when it has none. */
            std::string m_constructor;

            /** Whether the message is a plain value or tuple. */
            bool m_anonymous = true;

            /** Types of the message's parts. */
            std::vector<DataTypeBase> m_types;

            /** Name of the Ada entry. */
            std::string m_ada;
        };

    }
}

#endif
