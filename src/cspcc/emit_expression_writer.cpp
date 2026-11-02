// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/emit_expression_writer.h"

#include <charconv>
#include <utility>
#include <vector>

namespace cspcc
{

    namespace
    {

        // Check whether an operator compares.
        bool compares(data::DataTokenKind op)
        {
            using Kind = data::DataTokenKind;
            return op == Kind::Equal || op == Kind::NotEqual || op == Kind::Less ||
                   op == Kind::LessEqual || op == Kind::Greater || op == Kind::GreaterEqual;
        }

        // Spell a binary operator in Ada, and and or short-circuited.
        std::string_view ada_operator(data::DataTokenKind op)
        {
            switch (op) {
                case data::DataTokenKind::Plus:
                    return "+";
                case data::DataTokenKind::Minus:
                    return "-";
                case data::DataTokenKind::Star:
                    return "*";
                case data::DataTokenKind::Slash:
                case data::DataTokenKind::Div:
                    return "/";
                case data::DataTokenKind::Mod:
                    return "mod";
                case data::DataTokenKind::Equal:
                    return "=";
                case data::DataTokenKind::NotEqual:
                    return "/=";
                case data::DataTokenKind::Less:
                    return "<";
                case data::DataTokenKind::LessEqual:
                    return "<=";
                case data::DataTokenKind::Greater:
                    return ">";
                case data::DataTokenKind::GreaterEqual:
                    return ">=";
                case data::DataTokenKind::And:
                    return "and then";
                case data::DataTokenKind::Or:
                    return "or else";
                default:
                    return "?";
            }
        }

    }

    // Render by kind; a real result or a comparison with a real converts integer operands.
    std::string EmitExpressionWriter::render(const data::DataExpression &expression,
                                             data::DataTypeBase want) const
    {
        using Base = data::DataTypeBase;
        std::string text;
        switch (expression.m_kind) {
            case data::DataExpressionKind::Integer:
                text = std::to_string(expression.m_integer);
                break;
            case data::DataExpressionKind::Real:
                text = real_literal(expression.m_real);
                break;
            case data::DataExpressionKind::Character:
                text = std::string("'") + static_cast<char>(expression.m_integer) + "'";
                break;
            case data::DataExpressionKind::Boolean:
                text = expression.m_boolean ? "True" : "False";
                break;
            case data::DataExpressionKind::Name: {
                auto substitution = m_substitutions.find(expression.m_symbol);
                text = substitution != m_substitutions.end() ? substitution->second
                                                             : expression.m_symbol->m_ada;
                break;
            }
            case data::DataExpressionKind::Call: {
                text = expression.m_symbol->m_ada + " (";
                for (size_t i = 0; i < expression.m_args.size(); i++) {
                    text += (i ? ", " : "") + render(*expression.m_args[i]);
                }
                text += ")";
                break;
            }
            case data::DataExpressionKind::Unary:
                text = expression.m_op == data::DataTokenKind::Not
                           ? "(not " + render(*expression.m_args[0]) + ")"
                           : "(-" + render(*expression.m_args[0], expression.m_type) + ")";
                break;
            case data::DataExpressionKind::Binary: {
                const data::DataExpression &left = *expression.m_args[0];
                const data::DataExpression &right = *expression.m_args[1];
                Base operands = Base::None;
                if (expression.m_type == Base::Real ||
                    (compares(expression.m_op) &&
                     (left.m_type == Base::Real || right.m_type == Base::Real))) {
                    operands = Base::Real;
                }
                text = "(" + render(left, operands) + " " +
                       std::string(ada_operator(expression.m_op)) + " " + render(right, operands) +
                       ")";
                break;
            }
            case data::DataExpressionKind::Tuple:
                text = "()";
                break;
        }
        if (want == Base::Real && expression.m_type == Base::Integer) {
            return "Long_Float (" + text + ")";
        }
        return text;
    }

    // Replace a symbol.
    void EmitExpressionWriter::substitute(const data::DataSymbol *symbol, std::string text)
    {
        m_substitutions[symbol] = std::move(text);
    }

    // Stop replacing a symbol.
    void EmitExpressionWriter::forget(const data::DataSymbol *symbol)
    {
        m_substitutions.erase(symbol);
    }

    // Name an Ada type.
    std::string_view EmitExpressionWriter::ada_type(data::DataTypeBase base)
    {
        switch (base) {
            case data::DataTypeBase::Real:
                return "Long_Float";
            case data::DataTypeBase::Character:
                return "Character";
            case data::DataTypeBase::Boolean:
                return "Boolean";
            default:
                return "Integer";
        }
    }

    // Give an initial value.
    std::string_view EmitExpressionWriter::default_value(data::DataTypeBase base)
    {
        switch (base) {
            case data::DataTypeBase::Real:
                return "0.0";
            case data::DataTypeBase::Character:
                return "' '";
            case data::DataTypeBase::Boolean:
                return "False";
            default:
                return "0";
        }
    }

    // Write the shortest round-trip digits with the point and exponent Ada needs.
    std::string EmitExpressionWriter::real_literal(double value)
    {
        char buffer[64];
        auto [end, status] = std::to_chars(buffer, buffer + sizeof buffer, value);
        std::string text(buffer, status == std::errc() ? end : buffer);
        size_t exponent = text.find('e');
        std::string mantissa = text.substr(0, exponent);
        if (mantissa.find('.') == std::string::npos) {
            mantissa += ".0";
        }
        return exponent == std::string::npos ? mantissa
                                             : mantissa + "E" + text.substr(exponent + 1);
    }

    // Spell an integer.
    std::string EmitExpressionWriter::number(long long value)
    {
        return value < 0 ? "(" + std::to_string(value) + ")" : std::to_string(value);
    }

}
