// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMIT_EXPRESSION_WRITER_H
#define CSPCC_EMIT_EXPRESSION_WRITER_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_symbol.h"
#include "cspcc/data/data_type_base.h"

#include <map>
#include <string>
#include <string_view>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Renders checked expressions as Ada, fully parenthesized.
    class EmitExpressionWriter
    {
    public:
        // Render an expression.
        std::string render(const data::DataExpression &expression,
                           data::DataTypeBase want = data::DataTypeBase::None) const;

        // Render a symbol as fixed text from now on, such as a bound variable as a constant.
        void substitute(const data::DataSymbol *symbol, std::string text);

        // Render a symbol by its Ada name again.
        void forget(const data::DataSymbol *symbol);

        // Name the Ada type of a CSP type.
        static std::string_view ada_type(data::DataTypeBase base);

        // Give the initial value of a variable of a type.
        static std::string_view default_value(data::DataTypeBase base);

        // Render a real number as an Ada literal.
        static std::string real_literal(double value);

        // Spell an integer, parenthesizing a negative one.
        static std::string number(long long value);

    private:
        // Texts standing for symbols, such as bound variables in unrolled alternatives.
        std::map<const data::DataSymbol *, std::string> m_substitutions;
    };

}

#endif
