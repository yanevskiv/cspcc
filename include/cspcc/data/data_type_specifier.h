/*
 * C++ header for the type written in a CSP declaration.
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

#ifndef CSPCC_DATA_TYPE_SPECIFIER_H
#define CSPCC_DATA_TYPE_SPECIFIER_H

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
        /** Describes a declared type: a name and, for an array, its bounds. */
        struct DataTypeSpecifier
        {
            /** Name of the scalar type. */
            std::string m_name;

            /** Position of the type's name. */
            DataLocation m_location;

            /** Lower bound of each dimension. */
            std::vector<std::unique_ptr<DataExpression>> m_lows;

            /** Upper bound of each dimension. */
            std::vector<std::unique_ptr<DataExpression>> m_highs;
        };

    }
}

#endif
