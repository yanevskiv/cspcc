// The syntax tree of a CSP program, as the parser builds it.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CSPCC_AST_H
#define CSPCC_AST_H

#include "token.h"

#include <memory>
#include <string>
#include <vector>

struct Symbol;
struct Entry;
struct Family;
struct Command;

// Describes a scalar type, or the element type of an array.
enum class BaseType {
    None,
    Integer,
    Real,
    Character,
    Boolean,
};

// Describes the kind of an expression or of a target.
enum class ExpressionKind {
    Integer,
    Real,
    Character,
    Boolean,
    Name,
    Call,
    Unary,
    Binary,
    Tuple,
};

// Describes an expression, or the target of an input or an assignment.
struct Expression
{
    // Kind of the expression.
    ExpressionKind m_kind = ExpressionKind::Integer;

    // Position of the expression.
    Location m_location;

    // Name of a variable, an array or a constructor.
    std::string m_name;

    // Operator of a unary or binary expression.
    TokenKind m_op = TokenKind::Eof;

    // Subscripts, arguments, operands or tuple items.
    std::vector<std::unique_ptr<Expression>> m_args;

    // Value of an integer literal or the code of a character literal.
    long long m_integer = 0;

    // Value of a real literal.
    double m_real = 0;

    // Value of a boolean literal.
    bool m_boolean = false;

    // Type the checker gives the expression.
    BaseType m_type = BaseType::None;

    // Symbol a name or an element resolves to.
    Symbol *m_symbol = nullptr;
};

// Describes a range in a process label or a guard.
struct Range
{
    // Name of the bound variable.
    std::string m_variable;

    // Position of the bound variable.
    Location m_location;

    // Lower bound as written.
    std::unique_ptr<Expression> m_low;

    // Upper bound as written.
    std::unique_ptr<Expression> m_high;

    // Lower bound the checker computes.
    long long m_first = 0;

    // Upper bound the checker computes.
    long long m_last = 0;

    // Symbol of the bound variable.
    Symbol *m_symbol = nullptr;
};

// Describes a declared type: a name and, for an array, its bounds.
struct TypeSpecifier
{
    // Name of the scalar type.
    std::string m_name;

    // Position of the type's name.
    Location m_location;

    // Lower bound of each dimension.
    std::vector<std::unique_ptr<Expression>> m_lows;

    // Upper bound of each dimension.
    std::vector<std::unique_ptr<Expression>> m_highs;
};

// Describes a process name such as X, X(i) or M(i, j - 1).
struct ProcessName
{
    // Name of the process or of its array.
    std::string m_name;

    // Position of the name.
    Location m_location;

    // Subscripts of an element of a process array.
    std::vector<std::unique_ptr<Expression>> m_subscripts;

    // Family the checker resolves the name to.
    Family *m_family = nullptr;
};

// Describes the kind of a command.
enum class CommandKind {
    Skip,
    Assign,
    Declare,
    Input,
    Output,
    Macro,
    Block,
    Alternative,
    Repetitive,
};

// Describes an element of a guard: a condition or a declaration.
struct GuardElement
{
    // Boolean condition, when the element is one.
    std::unique_ptr<Expression> m_condition;

    // Declaration, when the element is one.
    std::unique_ptr<Command> m_declaration;
};

// Describes a guarded command: (ranges) elements; input -> commands.
struct GuardedCommand
{
    // Position of the guard.
    Location m_location;

    // Ranges the guarded command is repeated over.
    std::vector<std::unique_ptr<Range>> m_ranges;

    // Conditions and declarations of the guard.
    std::vector<GuardElement> m_elements;

    // Input that ends the guard, if any.
    std::unique_ptr<Command> m_input;

    // Commands run when the guard is chosen.
    std::vector<std::unique_ptr<Command>> m_body;
};

// Describes a command and, after checking, its resolved message.
struct Command
{
    // Kind of the command.
    CommandKind m_kind = CommandKind::Skip;

    // Position of the command.
    Location m_location;

    // Targets of an assignment, one or a tuple.
    std::vector<std::unique_ptr<Expression>> m_targets;

    // Values of an assignment, one or a tuple.
    std::vector<std::unique_ptr<Expression>> m_values;

    // Names a declaration declares.
    std::vector<std::string> m_names;

    // Position of each declared name.
    std::vector<Location> m_name_locations;

    // Type a declaration gives its names.
    TypeSpecifier m_type;

    // Symbols a declaration creates.
    std::vector<Symbol *> m_symbols;

    // Process an input reads from or an output writes to.
    ProcessName m_process;

    // Message of an input or an output as written.
    std::unique_ptr<Expression> m_message;

    // Name of a macro or a hook.
    std::string m_name;

    // Arguments of a hook.
    std::vector<std::unique_ptr<Expression>> m_args;

    // Whether a macro or a hook was written with parentheses.
    bool m_has_parens = false;

    // Commands of a block.
    std::vector<std::unique_ptr<Command>> m_body;

    // Guarded commands of an alternative or a repetitive command.
    std::vector<std::unique_ptr<GuardedCommand>> m_guards;

    // Constructor of the message, empty when it has none.
    std::string m_constructor;

    // Whether the message is a plain value or tuple, with no constructor.
    bool m_anonymous = true;

    // Values or targets the message carries.
    std::vector<Expression *> m_parts;

    // Entry of the receiving process the message travels through.
    Entry *m_entry = nullptr;

    // Whether a macro command calls a hook.
    bool m_is_hook = false;
};

// Describes a label subscript: a fixed value or a range.
struct LabelSubscript
{
    // Fixed value, when the subscript is one.
    std::unique_ptr<Expression> m_value;

    // Range, when the subscript is one.
    std::unique_ptr<Range> m_range;
};

// Describes a process or a macro definition.
struct Definition
{
    // Name of the process, the process array or the macro.
    std::string m_name;

    // Position of the name.
    Location m_location;

    // Whether the label has subscripts.
    bool m_has_subscripts = false;

    // Subscripts of the label.
    std::vector<LabelSubscript> m_subscripts;

    // Commands of the process or the macro.
    std::vector<std::unique_ptr<Command>> m_body;

    // Whether the definition is an element of a parallel command.
    bool m_in_parallel = false;
};

// Describes a named constant.
struct ConstantDefinition
{
    // Name of the constant.
    std::string m_name;

    // Position of the name.
    Location m_location;

    // Value as written.
    std::unique_ptr<Expression> m_value;
};

// Describes a parsed program: its constants and its definitions.
struct Ast
{
    // Constants in source order.
    std::vector<std::unique_ptr<ConstantDefinition>> m_constants;

    // Process and macro definitions in source order.
    std::vector<std::unique_ptr<Definition>> m_definitions;
};

// Describes a [ ... ] command by what its top level holds.
enum class BracketShape {
    Block,
    Alternative,
    Parallel,
};

// Describes where a command list appears, which decides what ends it.
enum class ListContext {
    TopLevel,
    Element,
    Guarded,
    Block,
};

// Copy an expression's syntax, leaving out what the checker fills in, for expanding macros.
std::unique_ptr<Expression> clone_expression(const Expression *expression);

// Copy a list of expressions.
std::vector<std::unique_ptr<Expression>>
clone_expressions(const std::vector<std::unique_ptr<Expression>> &expressions);

// Copy a range.
std::unique_ptr<Range> clone_range(const Range &range);

// Copy a guarded command.
std::unique_ptr<GuardedCommand> clone_guarded(const GuardedCommand &guard);

// Copy a command.
std::unique_ptr<Command> clone_command(const Command *command);

// Copy a list of commands.
std::vector<std::unique_ptr<Command>>
clone_commands(const std::vector<std::unique_ptr<Command>> &commands);

#endif
