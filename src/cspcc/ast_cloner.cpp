/*
 * C++ file for deep copies of CSP syntax trees.
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

// Project header for the copies.
#include "cspcc/ast_cloner.h"

// Namespace for cspcc's syntax trees.
namespace cspcc
{

    // Copy an expression's syntax, not its type or symbol.
    std::unique_ptr<data::DataExpression>
    AstCloner::clone_expression(const data::DataExpression *expression)
    {
        if (!expression) {
            return nullptr;
        }
        auto copy = std::make_unique<data::DataExpression>();
        copy->m_kind = expression->m_kind;
        copy->m_location = expression->m_location;
        copy->m_name = expression->m_name;
        copy->m_op = expression->m_op;
        copy->m_args = clone_expressions(expression->m_args);
        copy->m_integer = expression->m_integer;
        copy->m_real = expression->m_real;
        copy->m_boolean = expression->m_boolean;
        return copy;
    }

    // Copy a command's syntax, not its symbols or its message.
    std::unique_ptr<data::DataCommand> AstCloner::clone_command(const data::DataCommand *command)
    {
        if (!command) {
            return nullptr;
        }
        auto copy = std::make_unique<data::DataCommand>();
        copy->m_kind = command->m_kind;
        copy->m_location = command->m_location;
        copy->m_targets = clone_expressions(command->m_targets);
        copy->m_values = clone_expressions(command->m_values);
        copy->m_names = command->m_names;
        copy->m_name_locations = command->m_name_locations;
        copy->m_type.m_name = command->m_type.m_name;
        copy->m_type.m_location = command->m_type.m_location;
        copy->m_type.m_lows = clone_expressions(command->m_type.m_lows);
        copy->m_type.m_highs = clone_expressions(command->m_type.m_highs);
        copy->m_process.m_name = command->m_process.m_name;
        copy->m_process.m_location = command->m_process.m_location;
        copy->m_process.m_subscripts = clone_expressions(command->m_process.m_subscripts);
        copy->m_message = clone_expression(command->m_message.get());
        copy->m_name = command->m_name;
        copy->m_args = clone_expressions(command->m_args);
        copy->m_has_parens = command->m_has_parens;
        copy->m_body = clone_commands(command->m_body);
        for (const std::unique_ptr<data::DataGuardedCommand> &guard : command->m_guards) {
            copy->m_guards.push_back(clone_guarded(*guard));
        }
        return copy;
    }

    // Copy each command of a list.
    std::vector<std::unique_ptr<data::DataCommand>>
    AstCloner::clone_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands)
    {
        std::vector<std::unique_ptr<data::DataCommand>> copies;
        for (const std::unique_ptr<data::DataCommand> &command : commands) {
            copies.push_back(clone_command(command.get()));
        }
        return copies;
    }

    // Copy each expression of a list.
    std::vector<std::unique_ptr<data::DataExpression>> AstCloner::clone_expressions(
        const std::vector<std::unique_ptr<data::DataExpression>> &expressions)
    {
        std::vector<std::unique_ptr<data::DataExpression>> copies;
        for (const std::unique_ptr<data::DataExpression> &expression : expressions) {
            copies.push_back(clone_expression(expression.get()));
        }
        return copies;
    }

    // Copy a range's syntax.
    std::unique_ptr<data::DataRange> AstCloner::clone_range(const data::DataRange &range)
    {
        auto copy = std::make_unique<data::DataRange>();
        copy->m_variable = range.m_variable;
        copy->m_location = range.m_location;
        copy->m_low = clone_expression(range.m_low.get());
        copy->m_high = clone_expression(range.m_high.get());
        return copy;
    }

    // Copy a guarded command's ranges, elements, input and body.
    std::unique_ptr<data::DataGuardedCommand>
    AstCloner::clone_guarded(const data::DataGuardedCommand &guard)
    {
        auto copy = std::make_unique<data::DataGuardedCommand>();
        copy->m_location = guard.m_location;
        for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
            copy->m_ranges.push_back(clone_range(*range));
        }
        for (const data::DataGuardElement &element : guard.m_elements) {
            data::DataGuardElement elementCopy;
            elementCopy.m_condition = clone_expression(element.m_condition.get());
            elementCopy.m_declaration = clone_command(element.m_declaration.get());
            copy->m_elements.push_back(std::move(elementCopy));
        }
        copy->m_input = clone_command(guard.m_input.get());
        copy->m_body = clone_commands(guard.m_body);
        return copy;
    }

}
