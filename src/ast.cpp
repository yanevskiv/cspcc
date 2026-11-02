// Copies of syntax trees.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ast.h"

// Copy an expression's syntax, not its type or symbol.
std::unique_ptr<Expression> clone_expression(const Expression *expression)
{
    if (!expression) {
        return nullptr;
    }
    auto copy = std::make_unique<Expression>();
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
std::unique_ptr<Command> clone_command(const Command *command)
{
    if (!command) {
        return nullptr;
    }
    auto copy = std::make_unique<Command>();
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
    for (const std::unique_ptr<GuardedCommand> &guard : command->m_guards) {
        copy->m_guards.push_back(clone_guarded(*guard));
    }
    return copy;
}

// Copy each command of a list.
std::vector<std::unique_ptr<Command>>
clone_commands(const std::vector<std::unique_ptr<Command>> &commands)
{
    std::vector<std::unique_ptr<Command>> copies;
    for (const std::unique_ptr<Command> &command : commands) {
        copies.push_back(clone_command(command.get()));
    }
    return copies;
}

// Copy each expression of a list.
std::vector<std::unique_ptr<Expression>>
clone_expressions(const std::vector<std::unique_ptr<Expression>> &expressions)
{
    std::vector<std::unique_ptr<Expression>> copies;
    for (const std::unique_ptr<Expression> &expression : expressions) {
        copies.push_back(clone_expression(expression.get()));
    }
    return copies;
}

// Copy a range's syntax.
std::unique_ptr<Range> clone_range(const Range &range)
{
    auto copy = std::make_unique<Range>();
    copy->m_variable = range.m_variable;
    copy->m_location = range.m_location;
    copy->m_low = clone_expression(range.m_low.get());
    copy->m_high = clone_expression(range.m_high.get());
    return copy;
}

// Copy a guarded command's ranges, elements, input and body.
std::unique_ptr<GuardedCommand> clone_guarded(const GuardedCommand &guard)
{
    auto copy = std::make_unique<GuardedCommand>();
    copy->m_location = guard.m_location;
    for (const std::unique_ptr<Range> &range : guard.m_ranges) {
        copy->m_ranges.push_back(clone_range(*range));
    }
    for (const GuardElement &element : guard.m_elements) {
        GuardElement elementCopy;
        elementCopy.m_condition = clone_expression(element.m_condition.get());
        elementCopy.m_declaration = clone_command(element.m_declaration.get());
        copy->m_elements.push_back(std::move(elementCopy));
    }
    copy->m_input = clone_command(guard.m_input.get());
    copy->m_body = clone_commands(guard.m_body);
    return copy;
}
