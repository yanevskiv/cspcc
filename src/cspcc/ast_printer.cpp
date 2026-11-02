/*
 * C++ file for printing CSP syntax trees.
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

// Project header for the printer.
#include "cspcc/ast_printer.h"

// Standard header for formatting reals.
#include <sstream>

// Namespace for cspcc's syntax trees.
namespace cspcc
{

    // Prepare to print to a stream.
    AstPrinter::AstPrinter(std::ostream &stream)
        : m_stream(stream)
    {
        // Empty.
    }

    // Print the constants, then each definition and its commands.
    void AstPrinter::print(const data::DataAst &ast)
    {
        for (const std::unique_ptr<data::DataConstantDefinition> &constant : ast.m_constants) {
            m_stream << "const " << constant->m_name << " = "
                     << expression_text(constant->m_value.get()) << '\n';
        }
        for (const std::unique_ptr<data::DataDefinition> &definition : ast.m_definitions) {
            std::string label = definition->m_name;
            if (definition->m_has_subscripts) {
                label += '(';
                for (size_t i = 0; i < definition->m_subscripts.size(); i++) {
                    const data::DataLabelSubscript &subscript = definition->m_subscripts[i];
                    label += (i ? ", " : "") + (subscript.m_range
                                                    ? range_text(*subscript.m_range)
                                                    : expression_text(subscript.m_value.get()));
                }
                label += ')';
            }
            m_stream << (definition->m_in_parallel ? "process " : "define ") << label << '\n';
            print_commands(definition->m_body, 1);
        }
    }

    // Render an expression recursively.
    std::string AstPrinter::expression_text(const data::DataExpression *expression)
    {
        if (!expression) {
            return "<none>";
        }
        switch (expression->m_kind) {
            case data::DataExpressionKind::Integer:
                return std::to_string(expression->m_integer);
            case data::DataExpressionKind::Real: {
                std::ostringstream text;
                text << expression->m_real;
                return text.str();
            }
            case data::DataExpressionKind::Character:
                return std::string("'") + static_cast<char>(expression->m_integer) + "'";
            case data::DataExpressionKind::Boolean:
                return expression->m_boolean ? "true" : "false";
            case data::DataExpressionKind::Name:
                return expression->m_name;
            case data::DataExpressionKind::Call:
                return expression->m_name + "(" + list_text(expression->m_args) + ")";
            case data::DataExpressionKind::Tuple:
                return "(" + list_text(expression->m_args) + ")";
            case data::DataExpressionKind::Unary:
                return "(" + std::string(op_name(expression->m_op)) + " " +
                       expression_text(expression->m_args[0].get()) + ")";
            case data::DataExpressionKind::Binary:
                return "(" + expression_text(expression->m_args[0].get()) + " " +
                       std::string(op_name(expression->m_op)) + " " +
                       expression_text(expression->m_args[1].get()) + ")";
        }
        return "?";
    }

    // Render a process name and its subscripts.
    std::string AstPrinter::process_text(const data::DataProcessName &process)
    {
        if (process.m_subscripts.empty()) {
            return process.m_name;
        }
        return process.m_name + "(" + list_text(process.m_subscripts) + ")";
    }

    // Spell an operator.
    std::string_view AstPrinter::op_name(data::DataTokenKind op)
    {
        switch (op) {
            case data::DataTokenKind::Plus:
                return "+";
            case data::DataTokenKind::Minus:
                return "-";
            case data::DataTokenKind::Star:
                return "*";
            case data::DataTokenKind::Slash:
                return "/";
            case data::DataTokenKind::Mod:
                return "mod";
            case data::DataTokenKind::Div:
                return "div";
            case data::DataTokenKind::Equal:
                return "=";
            case data::DataTokenKind::NotEqual:
                return "<>";
            case data::DataTokenKind::Less:
                return "<";
            case data::DataTokenKind::LessEqual:
                return "<=";
            case data::DataTokenKind::Greater:
                return ">";
            case data::DataTokenKind::GreaterEqual:
                return ">=";
            case data::DataTokenKind::And:
                return "and";
            case data::DataTokenKind::Or:
                return "or";
            case data::DataTokenKind::Not:
                return "not";
            default:
                return "?";
        }
    }

    // Print each command of a list.
    void AstPrinter::print_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands,
                                    int depth)
    {
        for (const std::unique_ptr<data::DataCommand> &command : commands) {
            print_command(*command, depth);
        }
    }

    // Print a command by its kind, nesting what it holds one level deeper.
    void AstPrinter::print_command(const data::DataCommand &command, int depth)
    {
        std::string pad(depth * 2, ' ');
        switch (command.m_kind) {
            case data::DataCommandKind::Skip:
                m_stream << pad << "skip\n";
                return;
            case data::DataCommandKind::Assign:
                m_stream << pad << "assign " << list_text(command.m_targets)
                         << " := " << list_text(command.m_values) << '\n';
                return;
            case data::DataCommandKind::Declare: {
                std::string names;
                for (size_t i = 0; i < command.m_names.size(); i++) {
                    names += (i ? ", " : "") + command.m_names[i];
                }
                std::string bounds;
                for (size_t i = 0; i < command.m_type.m_lows.size(); i++) {
                    bounds += (i ? ", " : "") + expression_text(command.m_type.m_lows[i].get()) +
                              ".." + expression_text(command.m_type.m_highs[i].get());
                }
                m_stream << pad << "decl " << names << ": "
                         << (bounds.empty() ? "" : "(" + bounds + ") ") << command.m_type.m_name
                         << '\n';
                return;
            }
            case data::DataCommandKind::Input:
            case data::DataCommandKind::Output:
                m_stream << pad
                         << (command.m_kind == data::DataCommandKind::Input ? "input " : "output ")
                         << process_text(command.m_process)
                         << (command.m_kind == data::DataCommandKind::Input ? " ? " : " ! ")
                         << expression_text(command.m_message.get()) << '\n';
                return;
            case data::DataCommandKind::Macro:
                m_stream << pad << "macro " << command.m_name
                         << (command.m_has_parens ? "(" + list_text(command.m_args) + ")" : "")
                         << '\n';
                return;
            case data::DataCommandKind::Block:
                m_stream << pad << "block\n";
                print_commands(command.m_body, depth + 1);
                return;
            case data::DataCommandKind::Alternative:
            case data::DataCommandKind::Repetitive:
                m_stream << pad
                         << (command.m_kind == data::DataCommandKind::Alternative ? "alt\n"
                                                                                  : "rep\n");
                for (const std::unique_ptr<data::DataGuardedCommand> &guard : command.m_guards) {
                    m_stream << pad << "  guard";
                    for (const std::unique_ptr<data::DataRange> &range : guard->m_ranges) {
                        m_stream << " (" << range_text(*range) << ")";
                    }
                    m_stream << '\n';
                    for (const data::DataGuardElement &element : guard->m_elements) {
                        if (element.m_declaration) {
                            print_command(*element.m_declaration, depth + 2);
                        } else {
                            m_stream << pad << "    when "
                                     << expression_text(element.m_condition.get()) << '\n';
                        }
                    }
                    if (guard->m_input) {
                        print_command(*guard->m_input, depth + 2);
                    }
                    m_stream << pad << "  do\n";
                    print_commands(guard->m_body, depth + 2);
                }
                return;
        }
    }

    // Render a range.
    std::string AstPrinter::range_text(const data::DataRange &range)
    {
        return range.m_variable + ":" + expression_text(range.m_low.get()) + ".." +
               expression_text(range.m_high.get());
    }

    // Render expressions separated by commas.
    std::string
    AstPrinter::list_text(const std::vector<std::unique_ptr<data::DataExpression>> &expressions)
    {
        std::string text;
        for (size_t i = 0; i < expressions.size(); i++) {
            text += (i ? ", " : "") + expression_text(expressions[i].get());
        }
        return text;
    }

}
