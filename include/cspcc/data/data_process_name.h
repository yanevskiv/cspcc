/*
 * C++ header for a process name in a CSP input or output.
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

#ifndef CSPCC_DATA_PROCESS_NAME_H
#define CSPCC_DATA_PROCESS_NAME_H

#include "cspcc/data/data_expression.h"
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
        struct DataFamily;

        /** Describes a process name such as X, X(i) or M(i, j - 1). */
        struct DataProcessName
        {
            /** Name of the process or of its array. */
            std::string m_name;

            /** Position of the name. */
            DataLocation m_location;

            /** Subscripts of an element of a process array. */
            std::vector<std::unique_ptr<DataExpression>> m_subscripts;

            /** Family sema resolves the name to. */
            DataFamily *m_family = nullptr;
        };

    }
}

#endif
