// Parses tokens into a syntax tree.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CSPCC_PARSER_H
#define CSPCC_PARSER_H

#include "ast.h"
#include "diagnostics.h"
#include "token.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

// Parses CSP tokens into a syntax tree, by recursive descent.
class Parser
{
public:
    // Prepare to parse tokens.
    Parser(const std::vector<Token> &tokens, Diagnostics &diagnostics);

    // Parse a whole program.
    std::unique_ptr<Ast> parse_program();

    // Parse one expression with nothing after it, such as the value of -D.
    std::unique_ptr<Expression> parse_lone_expression();

private:
    // Look at a token ahead of the current one.
    const Token &peek(size_t ahead = 0) const;

    // Check the kind of a token ahead.
    bool is(TokenKind kind, size_t ahead = 0) const;

    // Move past the current token.
    const Token &next();

    // Move past the current token if it has a kind.
    bool accept(TokenKind kind);

    // Move past a token that must have a kind, or stop with an error.
    const Token &expect(TokenKind kind, std::string_view context);

    // Report an error and stop the parse.
    [[noreturn]] void fail(const Location &location, std::string_view code,
                           std::string_view message);

    // Find the token after the parenthesis that closes one.
    size_t after_parens(size_t ahead) const;

    // Tell what a [ ... ] command holds at its top level.
    BracketShape bracket_shape(size_t ahead) const;

    // Check whether a label and '::' start at a token.
    bool starts_definition(size_t ahead) const;

    // Check whether NAME :: starts a constant, its value a literal.
    bool starts_named_constant(size_t ahead) const;

    // Check whether an input or an output starts at a token.
    bool starts_communication(size_t ahead, TokenKind &kind) const;

    // Check whether a declaration, x, y: type, starts at a token.
    bool starts_declaration(size_t ahead) const;

    // Check whether a guard's range prefix, (i:1..n), starts at a token.
    bool starts_range_prefix(size_t ahead) const;

    // Parse const NAME = value.
    void parse_constant(Ast &ast);

    // Parse NAME :: value, a constant as the notes write it.
    void parse_named_constant(Ast &ast);

    // Parse a parallel command, [ A::... || B(i:1..n)::... ].
    void parse_parallel(Ast &ast);

    // Parse label :: commands.
    std::unique_ptr<Definition> parse_definition(bool inParallel);

    // Parse a label subscript: i:low..high or a value.
    LabelSubscript parse_label_subscript();

    // Parse i:low..high.
    std::unique_ptr<Range> parse_range();

    // Parse commands separated by semicolons.
    std::vector<std::unique_ptr<Command>> parse_commands(ListContext context);

    // Check whether the current token ends a command list.
    bool at_list_end(ListContext context) const;

    // Parse one command.
    std::unique_ptr<Command> parse_command();

    // Parse a command that starts with an identifier.
    std::unique_ptr<Command> parse_named_command();

    // Parse x, y: type.
    std::unique_ptr<Command> parse_declaration();

    // Parse a type, (low..high, ...) name.
    void parse_type(TypeSpecifier &type);

    // Parse X(...)?target or X(...)!value.
    std::unique_ptr<Command> parse_communication();

    // Parse the guarded commands of [ ... ] or *[ ... ] after the '['.
    void parse_guarded_commands(Command &command, bool guarded);

    // Parse (ranges) elements; input -> commands.
    std::unique_ptr<GuardedCommand> parse_guarded_command();

    // Parse an expression.
    std::unique_ptr<Expression> parse_expression();

    // Parse operands joined by or.
    std::unique_ptr<Expression> parse_or();

    // Parse operands joined by and.
    std::unique_ptr<Expression> parse_and();

    // Parse not, or a comparison.
    std::unique_ptr<Expression> parse_not();

    // Parse a comparison, or a sum.
    std::unique_ptr<Expression> parse_comparison();

    // Parse terms joined by + and -.
    std::unique_ptr<Expression> parse_sum();

    // Parse factors joined by *, /, mod and div.
    std::unique_ptr<Expression> parse_product();

    // Parse a signed factor.
    std::unique_ptr<Expression> parse_unary();

    // Parse a literal, a name, name(...), (expression) or a tuple.
    std::unique_ptr<Expression> parse_primary();

    // Parse ( expression, ... ) after a name.
    std::vector<std::unique_ptr<Expression>> parse_arguments();

    // Join two operands with an operator.
    static std::unique_ptr<Expression> make_binary(TokenKind op, const Location &location,
                                                   std::unique_ptr<Expression> left,
                                                   std::unique_ptr<Expression> right);

    // Tokens being parsed.
    const std::vector<Token> &m_tokens;

    // Destination of syntax errors.
    Diagnostics &m_diagnostics;

    // Index of the current token.
    size_t m_position = 0;

    // Count of label ranges written without a variable, as in Producer(1..N).
    int m_anonymous = 0;
};

#endif
