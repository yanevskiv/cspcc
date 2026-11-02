/*
 * C++ header for a CSP definition, label :: commands.
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

#ifndef CSPCC_DATA_DEFINITION_H
#define CSPCC_DATA_DEFINITION_H

#include "cspcc/data/data_command.h"
#include "cspcc/data/data_label_subscript.h"
#include "cspcc/data/data_location.h"

#include <memory>
#include <string>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a process or a macro definition. */
        struct DataDefinition
        {
            /** Name of the process, the process array or the macro. */
            std::string m_name;

            /** Position of the name. */
            DataLocation m_location;

            /** Whether the label has subscripts. */
            bool m_has_subscripts = false;

            /** Subscripts of the label. */
            std::vector<DataLabelSubscript> m_subscripts;

            /** Commands of the process or the macro. */
            std::vector<std::unique_ptr<DataCommand>> m_body;

            /** Whether the definition is an element of a parallel command. */
            bool m_in_parallel = false;
        };

    }
}

#endif
