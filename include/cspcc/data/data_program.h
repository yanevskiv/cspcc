/*
 * C++ header for a checked CSP program.
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

#ifndef CSPCC_DATA_PROGRAM_H
#define CSPCC_DATA_PROGRAM_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_family.h"
#include "cspcc/data/data_symbol.h"

#include <memory>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a checked program: its constants and its process families. */
        struct DataProgram
        {
            /** Constants in the order they are defined. */
            std::vector<DataSymbol *> m_constants;

            /** Process families in source order. */
            std::vector<std::unique_ptr<DataFamily>> m_families;

            /** Every symbol of the program. */
            std::vector<std::unique_ptr<DataSymbol>> m_symbols;

            /** Expressions sema creates, such as the values of -D. */
            std::vector<std::unique_ptr<DataExpression>> m_expressions;

            /** Number of processes. */
            int m_total = 0;
        };

    }
}

#endif
