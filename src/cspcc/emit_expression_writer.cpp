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
        bool compares(TokenKind op)
        {
            using Kind = TokenKind;
            return op == Kind::Equal || op == Kind::NotEqual || op == Kind::Less ||
                   op == Kind::LessEqual || op == Kind::Greater || op == Kind::GreaterEqual;
        }

        // Spell a binary operator in Ada, and and or short-circuited.
        std::string_view ada_operator(TokenKind op)
        {
            switch (op) {
                case TokenKind::Plus:
                    return "+";
                case TokenKind::Minus:
                    return "-";
                case TokenKind::Star:
                    return "*";
                case TokenKind::Slash:
                case TokenKind::Div:
                    return "/";
                case TokenKind::Mod:
                    return "mod";
                case TokenKind::Equal:
                    return "=";
                case TokenKind::NotEqual:
                    return "/=";
                case TokenKind::Less:
                    return "<";
                case TokenKind::LessEqual:
                    return "<=";
                case TokenKind::Greater:
                    return ">";
                case TokenKind::GreaterEqual:
                    return ">=";
                case TokenKind::And:
                    return "and then";
                case TokenKind::Or:
                    return "or else";
                default:
                    return "?";
            }
        }

    }

    // Render by kind; a real result or a comparison with a real converts integer operands.
    std::string EmitExpressionWriter::render(const Expression &expression,
                                             BaseType want) const
    {
        using Base = BaseType;
        std::string text;
        switch (expression.m_kind) {
            case ExpressionKind::Integer:
                text = std::to_string(expression.m_integer);
                break;
            case ExpressionKind::Real:
                text = real_literal(expression.m_real);
                break;
            case ExpressionKind::Character:
                text = std::string("'") + static_cast<char>(expression.m_integer) + "'";
                break;
            case ExpressionKind::Boolean:
                text = expression.m_boolean ? "True" : "False";
                break;
            case ExpressionKind::Name: {
                auto substitution = m_substitutions.find(expression.m_symbol);
                text = substitution != m_substitutions.end() ? substitution->second
                                                             : expression.m_symbol->m_ada;
                break;
            }
            case ExpressionKind::Call: {
                text = expression.m_symbol->m_ada + " (";
                for (size_t i = 0; i < expression.m_args.size(); i++) {
                    text += (i ? ", " : "") + render(*expression.m_args[i]);
                }
                text += ")";
                break;
            }
            case ExpressionKind::Unary:
                text = expression.m_op == TokenKind::Not
                           ? "(not " + render(*expression.m_args[0]) + ")"
                           : "(-" + render(*expression.m_args[0], expression.m_type) + ")";
                break;
            case ExpressionKind::Binary: {
                const Expression &left = *expression.m_args[0];
                const Expression &right = *expression.m_args[1];
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
            case ExpressionKind::Tuple:
                text = "()";
                break;
        }
        if (want == Base::Real && expression.m_type == Base::Integer) {
            return "Long_Float (" + text + ")";
        }
        return text;
    }

    // Replace a symbol.
    void EmitExpressionWriter::substitute(const Symbol *symbol, std::string text)
    {
        m_substitutions[symbol] = std::move(text);
    }

    // Stop replacing a symbol.
    void EmitExpressionWriter::forget(const Symbol *symbol)
    {
        m_substitutions.erase(symbol);
    }

    // Name an Ada type.
    std::string_view EmitExpressionWriter::ada_type(BaseType base)
    {
        switch (base) {
            case BaseType::Real:
                return "Long_Float";
            case BaseType::Character:
                return "Character";
            case BaseType::Boolean:
                return "Boolean";
            default:
                return "Integer";
        }
    }

    // Give an initial value.
    std::string_view EmitExpressionWriter::default_value(BaseType base)
    {
        switch (base) {
            case BaseType::Real:
                return "0.0";
            case BaseType::Character:
                return "' '";
            case BaseType::Boolean:
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
