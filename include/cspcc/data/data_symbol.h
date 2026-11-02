/*
 * C++ header for a resolved CSP name.
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

#ifndef CSPCC_DATA_SYMBOL_H
#define CSPCC_DATA_SYMBOL_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_symbol_kind.h"
#include "cspcc/data/data_type_base.h"

#include <string>
#include <utility>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a constant, a variable, a label index or a bound variable. */
        struct DataSymbol
        {
            /** Name as written in CSP. */
            std::string m_name;

            /** What the name stands for. */
            DataSymbolKind m_kind = DataSymbolKind::Variable;

            /** Scalar type, or the element type of an array. */
            DataTypeBase m_base = DataTypeBase::Integer;

            /** Bounds of each dimension of an array. */
            std::vector<std::pair<long long, long long>> m_dimensions;

            /** Position of the declaration. */
            DataLocation m_location;

            /** Name the Ada program gives the symbol. */
            std::string m_ada;

            /** Position of a label index among the label's subscripts, from 1. */
            int m_index_position = 0;

            /** Value of a constant. */
            DataExpression *m_value = nullptr;

            /** Whether the integer value of a constant is known. */
            bool m_known = false;

            /** Integer value of a constant. */
            long long m_integer = 0;
        };

    }
}

#endif
