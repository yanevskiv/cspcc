/*
 * C++ header for the Ada form of CSP expressions.
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

#ifndef CSPCC_EMIT_EXPRESSION_WRITER_H
#define CSPCC_EMIT_EXPRESSION_WRITER_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_symbol.h"
#include "cspcc/data/data_type_base.h"

#include <map>
#include <string>
#include <string_view>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Renders checked expressions as Ada, fully parenthesized. */
    class EmitExpressionWriter
    {
    public:
        /**
         * Render an expression.
         *
         * @param expression Checked expression to render.
         * @param want Type the context needs, Real to convert an integer.
         * @return The Ada expression.
         */
        std::string render(const data::DataExpression &expression,
                           data::DataTypeBase want = data::DataTypeBase::None) const;

        /**
         * Render a symbol as fixed text from now on, such as a bound variable as a constant.
         *
         * @param symbol Symbol to replace.
         * @param text Ada text to render instead.
         */
        void substitute(const data::DataSymbol *symbol, std::string text);

        /**
         * Render a symbol by its Ada name again.
         *
         * @param symbol Symbol no longer replaced.
         */
        void forget(const data::DataSymbol *symbol);

        /**
         * Name the Ada type of a CSP type.
         *
         * @param base CSP type.
         * @return Integer, Long_Float, Character or Boolean.
         */
        static std::string_view ada_type(data::DataTypeBase base);

        /**
         * Give the initial value of a variable of a type.
         *
         * @param base CSP type.
         * @return An Ada literal.
         */
        static std::string_view default_value(data::DataTypeBase base);

        /**
         * Render a real number as an Ada literal.
         *
         * @param value Number to render.
         * @return Text with a point, such as 1.0E+20.
         */
        static std::string real_literal(double value);

        /**
         * Spell an integer, parenthesizing a negative one.
         *
         * @param value Integer to spell.
         * @return Text such as 5 or (-5).
         */
        static std::string number(long long value);

    private:
        /** Texts standing for symbols, such as bound variables in unrolled alternatives. */
        std::map<const data::DataSymbol *, std::string> m_substitutions;
    };

}

#endif
