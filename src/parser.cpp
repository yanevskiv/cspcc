// Parses tokens into a syntax tree.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "parser.h"
#include "diagnostics.h"
#include "lexer.h"

#include <string>
#include <utility>

// Name a token as a message quotes it.
static std::string describe(const Token &token)
{
    if (token.m_kind == TokenKind::Identifier) {
        return "'" + token.m_text + "'";
    }
    return std::string(Lexer::kind_name(token.m_kind));
}

// Check whether a token compares two values.
static bool is_comparison(TokenKind kind)
{
    using Kind = TokenKind;
    return kind == Kind::Equal || kind == Kind::NotEqual || kind == Kind::Less ||
           kind == Kind::LessEqual || kind == Kind::Greater || kind == Kind::GreaterEqual;
}

// Make an expression of a kind at a position.
static std::unique_ptr<Expression> make_expression(ExpressionKind kind, const Location &location)
{
    auto expression = std::make_unique<Expression>();
    expression->m_kind = kind;
    expression->m_location = location;
    return expression;
}

// Make a command of a kind at a position.
static std::unique_ptr<Command> make_command(CommandKind kind, const Location &location)
{
    auto command = std::make_unique<Command>();
    command->m_kind = kind;
    command->m_location = location;
    return command;
}

// Prepare to parse tokens.
Parser::Parser(const std::vector<Token> &tokens, Diagnostics &diagnostics)
    : m_tokens(tokens)
    , m_diagnostics(diagnostics)
{}

// Parse constants, parallel commands and definitions until the end.
std::unique_ptr<Ast> Parser::parse_program()
{
    auto ast = std::make_unique<Ast>();
    try {
        while (!is(TokenKind::Eof)) {
            if (is(TokenKind::Const)) {
                parse_constant(*ast);
            } else if (starts_named_constant(0)) {
                parse_named_constant(*ast);
            } else if (is(TokenKind::LeftBracket)) {
                if (bracket_shape(0) != BracketShape::Parallel) {
                    fail(peek().m_location, "ERR_PARSE_TOP_LEVEL",
                         "a [ ... ] at the top level must be a parallel command of labeled "
                         "processes");
                }
                parse_parallel(*ast);
            } else if (starts_definition(0)) {
                ast->m_definitions.push_back(parse_definition(false));
            } else {
                fail(peek().m_location, "ERR_PARSE_TOP_LEVEL",
                     "expected a definition such as X::..., a parallel command or a "
                     "constant, found " +
                         describe(peek()));
            }
        }
    } catch (const DiagnosticsAbort &) {
        return nullptr;
    }
    return ast;
}

// Parse one expression that must end the tokens.
std::unique_ptr<Expression> Parser::parse_lone_expression()
{
    try {
        std::unique_ptr<Expression> expression = parse_expression();
        if (!is(TokenKind::Eof)) {
            fail(peek().m_location, "ERR_PARSE_TRAILING",
                 "unexpected " + describe(peek()) + " after the expression");
        }
        return expression;
    } catch (const DiagnosticsAbort &) {
        return nullptr;
    }
}

// Look at a token ahead, clamped to the Eof token.
const Token &Parser::peek(size_t ahead) const
{
    size_t at = m_position + ahead;
    return at < m_tokens.size() ? m_tokens[at] : m_tokens.back();
}

// Check the kind of a token ahead.
bool Parser::is(TokenKind kind, size_t ahead) const
{
    return peek(ahead).m_kind == kind;
}

// Move past the current token, staying on Eof.
const Token &Parser::next()
{
    const Token &token = peek();
    if (m_position + 1 < m_tokens.size()) {
        m_position++;
    }
    return token;
}

// Move past a token of a kind, if it is there.
bool Parser::accept(TokenKind kind)
{
    if (!is(kind)) {
        return false;
    }
    next();
    return true;
}

// Move past a required token.
const Token &Parser::expect(TokenKind kind, std::string_view context)
{
    if (!is(kind)) {
        fail(peek().m_location, "ERR_PARSE_EXPECTED",
             "expected " + std::string(Lexer::kind_name(kind)) + " in " + std::string(context) +
                 ", found " + describe(peek()));
    }
    return next();
}

// Report the error and unwind to parse_program.
void Parser::fail(const Location &location, std::string_view code, std::string_view message)
{
    m_diagnostics.error(location, code, message);
    throw DiagnosticsAbort();
}

// Count parentheses from a '(' to the one that closes it.
size_t Parser::after_parens(size_t ahead) const
{
    int depth = 0;
    for (size_t i = ahead; !is(TokenKind::Eof, i); i++) {
        if (is(TokenKind::LeftParen, i)) {
            depth++;
        } else if (is(TokenKind::RightParen, i) && --depth == 0) {
            return i + 1;
        }
    }
    return 0;
}

// Scan a bracket's top level for '::', '||' or an arrow.
BracketShape Parser::bracket_shape(size_t ahead) const
{
    using Kind = TokenKind;
    int depth = 0;
    for (size_t i = ahead + 1; !is(Kind::Eof, i); i++) {
        Kind kind = peek(i).m_kind;
        if (kind == Kind::LeftBracket || kind == Kind::LeftParen) {
            depth++;
        } else if (kind == Kind::RightBracket || kind == Kind::RightParen) {
            if (depth == 0) {
                break;
            }
            depth--;
        } else if (depth == 0 && kind == Kind::Arrow) {
            return BracketShape::Alternative;
        } else if (depth == 0 && (kind == Kind::ColonColon || kind == Kind::Parallel)) {
            return BracketShape::Parallel;
        }
    }
    return BracketShape::Block;
}

// Check whether NAME :: starts a constant, its value a literal.
bool Parser::starts_named_constant(size_t ahead) const
{
    using Kind = TokenKind;
    if (!is(Kind::Identifier, ahead) || !is(Kind::ColonColon, ahead + 1)) {
        return false;
    }
    Kind kind = peek(ahead + 2).m_kind;
    return kind == Kind::Integer || kind == Kind::Real || kind == Kind::Character ||
           kind == Kind::True || kind == Kind::False ||
           (kind == Kind::Minus && (is(Kind::Integer, ahead + 3) || is(Kind::Real, ahead + 3)));
}

// Check for NAME:: or NAME(...)::.
bool Parser::starts_definition(size_t ahead) const
{
    if (!is(TokenKind::Identifier, ahead)) {
        return false;
    }
    if (is(TokenKind::ColonColon, ahead + 1)) {
        return true;
    }
    if (!is(TokenKind::LeftParen, ahead + 1)) {
        return false;
    }
    size_t after = after_parens(ahead + 1);
    return after && is(TokenKind::ColonColon, after);
}

// Check for X? X! X(...)? or X(...)!.
bool Parser::starts_communication(size_t ahead, TokenKind &kind) const
{
    if (!is(TokenKind::Identifier, ahead)) {
        return false;
    }
    size_t after = ahead + 1;
    if (is(TokenKind::LeftParen, after)) {
        after = after_parens(after);
        if (!after) {
            return false;
        }
    }
    kind = peek(after).m_kind;
    return kind == TokenKind::Question || kind == TokenKind::Bang;
}

// Check for x: or x, y, z: that isn't ': ='.
bool Parser::starts_declaration(size_t ahead) const
{
    if (!is(TokenKind::Identifier, ahead)) {
        return false;
    }
    size_t at = ahead + 1;
    while (is(TokenKind::Comma, at) && is(TokenKind::Identifier, at + 1)) {
        at += 2;
    }
    return is(TokenKind::Colon, at) && !is(TokenKind::Equal, at + 1);
}

// Check for (i: at the start of a guard element.
bool Parser::starts_range_prefix(size_t ahead) const
{
    return is(TokenKind::LeftParen, ahead) && is(TokenKind::Identifier, ahead + 1) &&
           is(TokenKind::Colon, ahead + 2) && !is(TokenKind::Equal, ahead + 3);
}

// Parse const NAME = value, with an optional semicolon.
void Parser::parse_constant(Ast &ast)
{
    next();
    auto constant = std::make_unique<ConstantDefinition>();
    const Token &name = expect(TokenKind::Identifier, "a constant");
    constant->m_name = name.m_text;
    constant->m_location = name.m_location;
    if (is(TokenKind::Assign)) {
        fail(peek().m_location, "ERR_PARSE_EXPECTED", "a constant is written const NAME = value");
    }
    expect(TokenKind::Equal, "a constant");
    constant->m_value = parse_expression();
    accept(TokenKind::Semicolon);
    ast.m_constants.push_back(std::move(constant));
}

// Parse NAME :: value, a constant as the notes write it.
void Parser::parse_named_constant(Ast &ast)
{
    auto constant = std::make_unique<ConstantDefinition>();
    const Token &name = next();
    constant->m_name = name.m_text;
    constant->m_location = name.m_location;
    next();
    constant->m_value = parse_expression();
    accept(TokenKind::Semicolon);
    ast.m_constants.push_back(std::move(constant));
}

// Parse [ definition || definition ... ].
void Parser::parse_parallel(Ast &ast)
{
    expect(TokenKind::LeftBracket, "a parallel command");
    do {
        ast.m_definitions.push_back(parse_definition(true));
    } while (accept(TokenKind::Parallel));
    expect(TokenKind::RightBracket, "a parallel command");
    accept(TokenKind::Semicolon);
}

// Parse NAME(subscripts) :: commands.
std::unique_ptr<Definition> Parser::parse_definition(bool inParallel)
{
    auto definition = std::make_unique<Definition>();
    std::vector<std::unique_ptr<Range>> prefix;
    while (inParallel && starts_range_prefix(0)) {
        next();
        do {
            prefix.push_back(parse_range());
        } while (accept(TokenKind::Comma));
        expect(TokenKind::RightParen, "a range");
    }
    const Token &name = expect(TokenKind::Identifier, "a process label");
    definition->m_name = name.m_text;
    definition->m_location = name.m_location;
    definition->m_in_parallel = inParallel;
    if (accept(TokenKind::LeftParen)) {
        definition->m_has_subscripts = true;
        do {
            definition->m_subscripts.push_back(parse_label_subscript());
        } while (accept(TokenKind::Comma));
        expect(TokenKind::RightParen, "a process label");
    }
    for (LabelSubscript &subscript : definition->m_subscripts) {
        Expression *value = subscript.m_value.get();
        if (!value || value->m_kind != ExpressionKind::Name) {
            continue;
        }
        for (std::unique_ptr<Range> &range : prefix) {
            if (range && range->m_variable == value->m_name) {
                subscript.m_range = std::move(range);
                subscript.m_value.reset();
                break;
            }
        }
    }
    for (std::unique_ptr<Range> &range : prefix) {
        if (range) {
            fail(range->m_location, "ERR_PARSE_UNUSED_RANGE",
                 "the label doesn't use '" + range->m_variable + "', as in (" +
                     range->m_variable + ":1..N) " + definition->m_name + "(" +
                     range->m_variable + ")");
        }
    }
    expect(TokenKind::ColonColon, "a process label");
    definition->m_body = parse_commands(inParallel ? ListContext::Element : ListContext::TopLevel);
    return definition;
}

// Parse i:low..high or a fixed value.
LabelSubscript Parser::parse_label_subscript()
{
    LabelSubscript subscript;
    if (is(TokenKind::Identifier) && is(TokenKind::Colon, 1)) {
        subscript.m_range = parse_range();
    } else {
        subscript.m_value = parse_expression();
        if (is(TokenKind::DotDot)) {
            auto range = std::make_unique<Range>();
            range->m_variable = "_" + std::to_string(++m_anonymous);
            range->m_location = subscript.m_value->m_location;
            range->m_low = std::move(subscript.m_value);
            next();
            range->m_high = parse_expression();
            subscript.m_range = std::move(range);
        }
    }
    return subscript;
}

// Parse i:low..high.
std::unique_ptr<Range> Parser::parse_range()
{
    auto range = std::make_unique<Range>();
    const Token &variable = expect(TokenKind::Identifier, "a range");
    range->m_variable = variable.m_text;
    range->m_location = variable.m_location;
    expect(TokenKind::Colon, "a range");
    range->m_low = parse_expression();
    expect(TokenKind::DotDot, "a range");
    range->m_high = parse_expression();
    return range;
}

// Parse commands separated by semicolons, a trailing one allowed.
std::vector<std::unique_ptr<Command>> Parser::parse_commands(ListContext context)
{
    std::vector<std::unique_ptr<Command>> commands;
    commands.push_back(parse_command());
    while (accept(TokenKind::Semicolon)) {
        if (at_list_end(context)) {
            break;
        }
        commands.push_back(parse_command());
    }
    if (!at_list_end(context)) {
        fail(peek().m_location, "ERR_PARSE_EXPECTED_SEMICOLON",
             "expected ';' between commands, found " + describe(peek()));
    }
    return commands;
}

// Check for a token that can't start a command, or for the next top-level item.
bool Parser::at_list_end(ListContext context) const
{
    using Kind = TokenKind;
    if (is(Kind::Eof) || is(Kind::RightBracket) || is(Kind::Box) || is(Kind::Parallel)) {
        return true;
    }
    if (context != ListContext::TopLevel) {
        return false;
    }
    return is(Kind::Const) || starts_definition(0) ||
           (is(Kind::LeftBracket) && bracket_shape(0) == BracketShape::Parallel);
}

// Parse one command by its first token.
std::unique_ptr<Command> Parser::parse_command()
{
    using Kind = TokenKind;
    Location location = peek().m_location;
    if (accept(Kind::Skip)) {
        return make_command(CommandKind::Skip, location);
    }
    if (is(Kind::Star) && is(Kind::LeftBracket, 1)) {
        bool guarded = bracket_shape(1) == BracketShape::Alternative;
        next();
        next();
        auto command = make_command(CommandKind::Repetitive, location);
        parse_guarded_commands(*command, guarded);
        expect(Kind::RightBracket, "a repetitive command");
        return command;
    }
    if (is(Kind::LeftBracket)) {
        BracketShape shape = bracket_shape(0);
        if (shape == BracketShape::Parallel) {
            fail(location, "ERR_PARSE_NESTED_PARALLEL",
                 "a parallel command inside a process isn't supported; put the processes "
                 "in the top-level one");
        }
        next();
        if (shape == BracketShape::Alternative) {
            auto command = make_command(CommandKind::Alternative, location);
            parse_guarded_commands(*command, true);
            expect(Kind::RightBracket, "an alternative command");
            return command;
        }
        auto command = make_command(CommandKind::Block, location);
        command->m_body = parse_commands(ListContext::Block);
        expect(Kind::RightBracket, "a block");
        return command;
    }
    if (is(Kind::LeftParen)) {
        auto command = make_command(CommandKind::Assign, location);
        std::unique_ptr<Expression> targets = parse_primary();
        expect(Kind::Assign, "a tuple assignment");
        std::unique_ptr<Expression> values = parse_expression();
        if (targets->m_kind == ExpressionKind::Tuple) {
            command->m_targets = std::move(targets->m_args);
        } else {
            command->m_targets.push_back(std::move(targets));
        }
        if (values->m_kind == ExpressionKind::Tuple) {
            command->m_values = std::move(values->m_args);
        } else {
            command->m_values.push_back(std::move(values));
        }
        return command;
    }
    if (is(Kind::Identifier)) {
        return parse_named_command();
    }
    fail(location, "ERR_PARSE_EXPECTED_COMMAND", "expected a command, found " + describe(peek()));
}

// Parse a declaration, an input, an output, an assignment, a macro or a hook.
std::unique_ptr<Command> Parser::parse_named_command()
{
    using Kind = TokenKind;
    Kind communication = Kind::Eof;
    if (starts_declaration(0)) {
        return parse_declaration();
    }
    if (starts_communication(0, communication)) {
        return parse_communication();
    }
    const Token &name = next();
    if (is(Kind::Colon) && is(Kind::Equal, 1)) {
        fail(peek().m_location, "ERR_PARSE_COLON_EQUALS",
             "':' and '=' written apart; an assignment is written ':='");
    }
    std::unique_ptr<Expression> target = make_expression(ExpressionKind::Name, name.m_location);
    target->m_name = name.m_text;
    if (is(Kind::LeftParen)) {
        target->m_kind = ExpressionKind::Call;
        target->m_args = parse_arguments();
    }
    if (accept(Kind::Assign)) {
        auto command = make_command(CommandKind::Assign, name.m_location);
        command->m_targets.push_back(std::move(target));
        command->m_values.push_back(parse_expression());
        return command;
    }
    auto command = make_command(CommandKind::Macro, name.m_location);
    command->m_name = name.m_text;
    command->m_has_parens = target->m_kind == ExpressionKind::Call;
    command->m_args = std::move(target->m_args);
    return command;
}

// Parse x, y: type.
std::unique_ptr<Command> Parser::parse_declaration()
{
    auto command = make_command(CommandKind::Declare, peek().m_location);
    do {
        const Token &name = expect(TokenKind::Identifier, "a declaration");
        command->m_names.push_back(name.m_text);
        command->m_name_locations.push_back(name.m_location);
    } while (accept(TokenKind::Comma));
    expect(TokenKind::Colon, "a declaration");
    parse_type(command->m_type);
    return command;
}

// Parse (low..high, ...) name.
void Parser::parse_type(TypeSpecifier &type)
{
    if (accept(TokenKind::LeftParen)) {
        do {
            type.m_lows.push_back(parse_expression());
            expect(TokenKind::DotDot, "an array's bounds");
            type.m_highs.push_back(parse_expression());
        } while (accept(TokenKind::Comma));
        expect(TokenKind::RightParen, "an array's bounds");
    }
    const Token &name = expect(TokenKind::Identifier, "a type");
    type.m_name = name.m_text;
    type.m_location = name.m_location;
}

// Parse a process name, ? or !, and the message.
std::unique_ptr<Command> Parser::parse_communication()
{
    const Token &name = next();
    auto command = make_command(CommandKind::Input, name.m_location);
    command->m_process.m_name = name.m_text;
    command->m_process.m_location = name.m_location;
    if (is(TokenKind::LeftParen)) {
        command->m_process.m_subscripts = parse_arguments();
    }
    if (next().m_kind == TokenKind::Bang) {
        command->m_kind = CommandKind::Output;
    }
    command->m_message = parse_expression();
    return command;
}

// Parse guarded commands separated by boxes, or one unguarded list.
void Parser::parse_guarded_commands(Command &command, bool guarded)
{
    if (!guarded) {
        auto forever = std::make_unique<GuardedCommand>();
        forever->m_location = peek().m_location;
        forever->m_body = parse_commands(ListContext::Block);
        command.m_guards.push_back(std::move(forever));
        return;
    }
    do {
        command.m_guards.push_back(parse_guarded_command());
    } while (accept(TokenKind::Box));
}

// Parse range prefixes, conditions, declarations and a last input, then the arrow.
std::unique_ptr<GuardedCommand> Parser::parse_guarded_command()
{
    using Kind = TokenKind;
    auto guard = std::make_unique<GuardedCommand>();
    guard->m_location = peek().m_location;
    while (true) {
        if (starts_range_prefix(0)) {
            next();
            do {
                guard->m_ranges.push_back(parse_range());
            } while (accept(Kind::Comma));
            expect(Kind::RightParen, "a guard's ranges");
            continue;
        }
        if (is(Kind::Arrow)) {
            break;
        }
        Kind communication = Kind::Eof;
        GuardElement element;
        if (starts_declaration(0)) {
            element.m_declaration = parse_declaration();
        } else if (starts_communication(0, communication)) {
            if (communication == Kind::Bang) {
                fail(peek().m_location, "ERR_PARSE_OUTPUT_GUARD",
                     "an output can't be part of a guard; a guard ends with an input");
            }
            guard->m_input = parse_communication();
            accept(Kind::Semicolon);
            if (!is(Kind::Arrow)) {
                fail(peek().m_location, "ERR_PARSE_INPUT_NOT_LAST",
                     "the input must be the last element of a guard, found " + describe(peek()));
            }
            break;
        } else {
            element.m_condition = parse_expression();
        }
        guard->m_elements.push_back(std::move(element));
        if (!accept(Kind::Comma) && !accept(Kind::Semicolon)) {
            break;
        }
    }
    if (guard->m_elements.empty() && !guard->m_input) {
        fail(peek().m_location, "ERR_PARSE_EMPTY_GUARD",
             "a guard needs a condition or an input before the arrow");
    }
    expect(Kind::Arrow, "a guarded command");
    guard->m_body = parse_commands(ListContext::Guarded);
    return guard;
}

// Parse an expression.
std::unique_ptr<Expression> Parser::parse_expression()
{
    return parse_or();
}

// Parse left-associative or.
std::unique_ptr<Expression> Parser::parse_or()
{
    std::unique_ptr<Expression> left = parse_and();
    while (is(TokenKind::Or)) {
        Location location = next().m_location;
        left = make_binary(TokenKind::Or, location, std::move(left), parse_and());
    }
    return left;
}

// Parse left-associative and.
std::unique_ptr<Expression> Parser::parse_and()
{
    std::unique_ptr<Expression> left = parse_not();
    while (is(TokenKind::And)) {
        Location location = next().m_location;
        left = make_binary(TokenKind::And, location, std::move(left), parse_not());
    }
    return left;
}

// Parse not, which binds looser than a comparison: not a = b is not (a = b).
std::unique_ptr<Expression> Parser::parse_not()
{
    if (!is(TokenKind::Not)) {
        return parse_comparison();
    }
    auto expression = make_expression(ExpressionKind::Unary, next().m_location);
    expression->m_op = TokenKind::Not;
    expression->m_args.push_back(parse_not());
    return expression;
}

// Parse one comparison; comparisons don't chain.
std::unique_ptr<Expression> Parser::parse_comparison()
{
    std::unique_ptr<Expression> left = parse_sum();
    if (!is_comparison(peek().m_kind)) {
        return left;
    }
    const Token &op = next();
    left = make_binary(op.m_kind, op.m_location, std::move(left), parse_sum());
    if (is_comparison(peek().m_kind)) {
        fail(peek().m_location, "ERR_PARSE_CHAINED_COMPARISON",
             "comparisons don't chain; join them with 'and'");
    }
    return left;
}

// Parse left-associative + and -.
std::unique_ptr<Expression> Parser::parse_sum()
{
    std::unique_ptr<Expression> left = parse_product();
    while (is(TokenKind::Plus) || is(TokenKind::Minus)) {
        const Token &op = next();
        left = make_binary(op.m_kind, op.m_location, std::move(left), parse_product());
    }
    return left;
}

// Parse left-associative *, /, mod and div.
std::unique_ptr<Expression> Parser::parse_product()
{
    using Kind = TokenKind;
    std::unique_ptr<Expression> left = parse_unary();
    while (is(Kind::Star) || is(Kind::Slash) || is(Kind::Mod) || is(Kind::Div)) {
        const Token &op = next();
        left = make_binary(op.m_kind, op.m_location, std::move(left), parse_unary());
    }
    return left;
}

// Parse unary minus, and drop unary plus.
std::unique_ptr<Expression> Parser::parse_unary()
{
    if (accept(TokenKind::Plus)) {
        return parse_unary();
    }
    if (!is(TokenKind::Minus)) {
        return parse_primary();
    }
    auto expression = make_expression(ExpressionKind::Unary, next().m_location);
    expression->m_op = TokenKind::Minus;
    expression->m_args.push_back(parse_unary());
    return expression;
}

// Parse a literal, a name, a call, a parenthesized expression or a tuple.
std::unique_ptr<Expression> Parser::parse_primary()
{
    using Kind = TokenKind;
    const Token &token = peek();
    switch (token.m_kind) {
        case Kind::Integer: {
            auto literal = make_expression(ExpressionKind::Integer, token.m_location);
            literal->m_integer = next().m_integer;
            return literal;
        }
        case Kind::Real: {
            auto literal = make_expression(ExpressionKind::Real, token.m_location);
            literal->m_real = next().m_real;
            return literal;
        }
        case Kind::Character: {
            auto literal = make_expression(ExpressionKind::Character, token.m_location);
            literal->m_integer = next().m_integer;
            return literal;
        }
        case Kind::True:
        case Kind::False: {
            auto literal = make_expression(ExpressionKind::Boolean, token.m_location);
            literal->m_boolean = next().m_kind == Kind::True;
            return literal;
        }
        case Kind::Identifier: {
            auto name = make_expression(ExpressionKind::Name, token.m_location);
            name->m_name = next().m_text;
            if (is(Kind::LeftParen)) {
                name->m_kind = ExpressionKind::Call;
                name->m_args = parse_arguments();
            }
            return name;
        }
        case Kind::LeftParen: {
            Location location = next().m_location;
            std::unique_ptr<Expression> first = parse_expression();
            if (!is(Kind::Comma)) {
                expect(Kind::RightParen, "a parenthesized expression");
                return first;
            }
            auto tuple = make_expression(ExpressionKind::Tuple, location);
            tuple->m_args.push_back(std::move(first));
            while (accept(Kind::Comma)) {
                tuple->m_args.push_back(parse_expression());
            }
            expect(Kind::RightParen, "a tuple");
            return tuple;
        }
        default:
            break;
    }
    fail(token.m_location, "ERR_PARSE_EXPECTED_EXPRESSION",
         "expected an expression, found " + describe(token));
}

// Parse ( ), or ( expression, ... ).
std::vector<std::unique_ptr<Expression>> Parser::parse_arguments()
{
    std::vector<std::unique_ptr<Expression>> args;
    expect(TokenKind::LeftParen, "an argument list");
    if (accept(TokenKind::RightParen)) {
        return args;
    }
    do {
        args.push_back(parse_expression());
    } while (accept(TokenKind::Comma));
    expect(TokenKind::RightParen, "an argument list");
    return args;
}

// Join two operands with an operator.
std::unique_ptr<Expression> Parser::make_binary(TokenKind op, const Location &location,
                                                std::unique_ptr<Expression> left,
                                                std::unique_ptr<Expression> right)
{
    auto expression = make_expression(ExpressionKind::Binary, location);
    expression->m_op = op;
    expression->m_args.push_back(std::move(left));
    expression->m_args.push_back(std::move(right));
    return expression;
}
