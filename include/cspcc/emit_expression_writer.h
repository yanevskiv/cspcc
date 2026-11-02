// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMIT_EXPRESSION_WRITER_H
#define CSPCC_EMIT_EXPRESSION_WRITER_H

#include "cspcc/ast.h"
#include "cspcc/program.h"

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
        std::string render(const Expression &expression,
                           BaseType want = BaseType::None) const;

        // Render a symbol as fixed text from now on, such as a bound variable as a constant.
        void substitute(const Symbol *symbol, std::string text);

        // Render a symbol by its Ada name again.
        void forget(const Symbol *symbol);

        // Name the Ada type of a CSP type.
        static std::string_view ada_type(BaseType base);

        // Give the initial value of a variable of a type.
        static std::string_view default_value(BaseType base);

        // Render a real number as an Ada literal.
        static std::string real_literal(double value);

        // Spell an integer, parenthesizing a negative one.
        static std::string number(long long value);

    private:
        // Texts standing for symbols, such as bound variables in unrolled alternatives.
        std::map<const Symbol *, std::string> m_substitutions;
    };

}

#endif
