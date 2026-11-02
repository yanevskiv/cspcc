/*
 * C++ header for a CSP range such as i:1..n.
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

#ifndef CSPCC_DATA_RANGE_H
#define CSPCC_DATA_RANGE_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"

#include <memory>
#include <string>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        struct DataSymbol;

        /** Describes a range in a process label or a guard. */
        struct DataRange
        {
            /** Name of the bound variable. */
            std::string m_variable;

            /** Position of the bound variable. */
            DataLocation m_location;

            /** Lower bound as written. */
            std::unique_ptr<DataExpression> m_low;

            /** Upper bound as written. */
            std::unique_ptr<DataExpression> m_high;

            /** Lower bound sema computes. */
            long long m_first = 0;

            /** Upper bound sema computes. */
            long long m_last = 0;

            /** Symbol of the bound variable. */
            DataSymbol *m_symbol = nullptr;
        };

    }
}

#endif
