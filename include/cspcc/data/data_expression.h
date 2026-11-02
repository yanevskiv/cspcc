/*
 * C++ header for a CSP expression.
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

#ifndef CSPCC_DATA_EXPRESSION_H
#define CSPCC_DATA_EXPRESSION_H

#include "cspcc/data/data_expression_kind.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_token_kind.h"
#include "cspcc/data/data_type_base.h"

#include <memory>
#include <string>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        struct DataSymbol;

        /** Describes an expression, or the target of an input or an assignment. */
        struct DataExpression
        {
            /** Kind of the expression. */
            DataExpressionKind m_kind = DataExpressionKind::Integer;

            /** Position of the expression. */
            DataLocation m_location;

            /** Name of a variable, an array or a constructor. */
            std::string m_name;

            /** Operator of a unary or binary expression. */
            DataTokenKind m_op = DataTokenKind::Eof;

            /** Subscripts, arguments, operands or tuple items. */
            std::vector<std::unique_ptr<DataExpression>> m_args;

            /** Value of an integer literal or the code of a character literal. */
            long long m_integer = 0;

            /** Value of a real literal. */
            double m_real = 0;

            /** Value of a boolean literal. */
            bool m_boolean = false;

            /** Type sema gives the expression. */
            DataTypeBase m_type = DataTypeBase::None;

            /** Symbol a name or an element resolves to. */
            DataSymbol *m_symbol = nullptr;
        };

    }
}

#endif
