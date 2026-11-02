/*
 * C++ file for cspcc's parser.
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

// Project headers for the parser and the token names it reports.
#include "cspcc/parser_grammar.h"
#include "cspcc/diagnostics_abort.h"
#include "cspcc/lexer_tokenizer.h"

// Standard headers for building messages.
#include <string>
#include <utility>

// Namespace for cspcc's parser.
namespace cspcc
{

    // Namespace for the parser's helpers.
    namespace
    {

        // Name a token as a message quotes it.
        std::string describe(const data::DataToken &token)
        {
            if (token.m_kind == data::DataTokenKind::Identifier) {
                return "'" + token.m_text + "'";
            }
            return std::string(LexerTokenizer::kind_name(token.m_kind));
        }

        // Check whether a token compares two values.
        bool is_comparison(data::DataTokenKind kind)
        {
            using Kind = data::DataTokenKind;
            return kind == Kind::Equal || kind == Kind::NotEqual || kind == Kind::Less ||
                   kind == Kind::LessEqual || kind == Kind::Greater || kind == Kind::GreaterEqual;
        }

        // Make an expression of a kind at a position.
        std::unique_ptr<data::DataExpression> make_expression(data::DataExpressionKind kind,
                                                              const data::DataLocation &location)
        {
            auto expression = std::make_unique<data::DataExpression>();
            expression->m_kind = kind;
            expression->m_location = location;
            return expression;
        }

        // Make a command of a kind at a position.
        std::unique_ptr<data::DataCommand> make_command(data::DataCommandKind kind,
                                                        const data::DataLocation &location)
        {
            auto command = std::make_unique<data::DataCommand>();
            command->m_kind = kind;
            command->m_location = location;
            return command;
        }

    }

    // Prepare to parse tokens.
    ParserGrammar::ParserGrammar(const std::vector<data::DataToken> &tokens,
                                 DiagnosticsEngine &diagnostics)
        : m_tokens(tokens)
        , m_diagnostics(diagnostics)
    {
        // Empty.
    }

    // Parse constants, parallel commands and definitions until the end.
    std::unique_ptr<data::DataAst> ParserGrammar::parse_program()
    {
        auto ast = std::make_unique<data::DataAst>();
        try {
            while (!is(data::DataTokenKind::Eof)) {
                if (is(data::DataTokenKind::Const)) {
                    parse_constant(*ast);
                } else if (is(data::DataTokenKind::LeftBracket)) {
                    if (bracket_shape(0) != data::DataBracketShape::Parallel) {
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
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_lone_expression()
    {
        try {
            std::unique_ptr<data::DataExpression> expression = parse_expression();
            if (!is(data::DataTokenKind::Eof)) {
                fail(peek().m_location, "ERR_PARSE_TRAILING",
                     "unexpected " + describe(peek()) + " after the expression");
            }
            return expression;
        } catch (const DiagnosticsAbort &) {
            return nullptr;
        }
    }

    // Look at a token ahead, clamped to the Eof token.
    const data::DataToken &ParserGrammar::peek(size_t ahead) const
    {
        size_t at = m_position + ahead;
        return at < m_tokens.size() ? m_tokens[at] : m_tokens.back();
    }

    // Check the kind of a token ahead.
    bool ParserGrammar::is(data::DataTokenKind kind, size_t ahead) const
    {
        return peek(ahead).m_kind == kind;
    }

    // Move past the current token, staying on Eof.
    const data::DataToken &ParserGrammar::next()
    {
        const data::DataToken &token = peek();
        if (m_position + 1 < m_tokens.size()) {
            m_position++;
        }
        return token;
    }

    // Move past a token of a kind, if it is there.
    bool ParserGrammar::accept(data::DataTokenKind kind)
    {
        if (!is(kind)) {
            return false;
        }
        next();
        return true;
    }

    // Move past a required token.
    const data::DataToken &ParserGrammar::expect(data::DataTokenKind kind, std::string_view context)
    {
        if (!is(kind)) {
            fail(peek().m_location, "ERR_PARSE_EXPECTED",
                 "expected " + std::string(LexerTokenizer::kind_name(kind)) + " in " +
                     std::string(context) + ", found " + describe(peek()));
        }
        return next();
    }

    // Report the error and unwind to parse_program.
    void ParserGrammar::fail(const data::DataLocation &location, std::string_view code,
                             std::string_view message)
    {
        m_diagnostics.error(location, code, message);
        throw DiagnosticsAbort();
    }

    // Count parentheses from a '(' to the one that closes it.
    size_t ParserGrammar::after_parens(size_t ahead) const
    {
        int depth = 0;
        for (size_t i = ahead; !is(data::DataTokenKind::Eof, i); i++) {
            if (is(data::DataTokenKind::LeftParen, i)) {
                depth++;
            } else if (is(data::DataTokenKind::RightParen, i) && --depth == 0) {
                return i + 1;
            }
        }
        return 0;
    }

    // Scan a bracket's top level for '::', '||' or an arrow.
    data::DataBracketShape ParserGrammar::bracket_shape(size_t ahead) const
    {
        using Kind = data::DataTokenKind;
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
                return data::DataBracketShape::Alternative;
            } else if (depth == 0 && (kind == Kind::ColonColon || kind == Kind::Parallel)) {
                return data::DataBracketShape::Parallel;
            }
        }
        return data::DataBracketShape::Block;
    }

    // Check for NAME:: or NAME(...)::.
    bool ParserGrammar::starts_definition(size_t ahead) const
    {
        if (!is(data::DataTokenKind::Identifier, ahead)) {
            return false;
        }
        if (is(data::DataTokenKind::ColonColon, ahead + 1)) {
            return true;
        }
        if (!is(data::DataTokenKind::LeftParen, ahead + 1)) {
            return false;
        }
        size_t after = after_parens(ahead + 1);
        return after && is(data::DataTokenKind::ColonColon, after);
    }

    // Check for X? X! X(...)? or X(...)!.
    bool ParserGrammar::starts_communication(size_t ahead, data::DataTokenKind &kind) const
    {
        if (!is(data::DataTokenKind::Identifier, ahead)) {
            return false;
        }
        size_t after = ahead + 1;
        if (is(data::DataTokenKind::LeftParen, after)) {
            after = after_parens(after);
            if (!after) {
                return false;
            }
        }
        kind = peek(after).m_kind;
        return kind == data::DataTokenKind::Question || kind == data::DataTokenKind::Bang;
    }

    // Check for x: or x, y, z: that isn't ': ='.
    bool ParserGrammar::starts_declaration(size_t ahead) const
    {
        if (!is(data::DataTokenKind::Identifier, ahead)) {
            return false;
        }
        size_t at = ahead + 1;
        while (is(data::DataTokenKind::Comma, at) && is(data::DataTokenKind::Identifier, at + 1)) {
            at += 2;
        }
        return is(data::DataTokenKind::Colon, at) && !is(data::DataTokenKind::Equal, at + 1);
    }

    // Check for (i: at the start of a guard element.
    bool ParserGrammar::starts_range_prefix(size_t ahead) const
    {
        return is(data::DataTokenKind::LeftParen, ahead) &&
               is(data::DataTokenKind::Identifier, ahead + 1) &&
               is(data::DataTokenKind::Colon, ahead + 2) &&
               !is(data::DataTokenKind::Equal, ahead + 3);
    }

    // Parse const NAME = value, with an optional semicolon.
    void ParserGrammar::parse_constant(data::DataAst &ast)
    {
        next();
        auto constant = std::make_unique<data::DataConstantDefinition>();
        const data::DataToken &name = expect(data::DataTokenKind::Identifier, "a constant");
        constant->m_name = name.m_text;
        constant->m_location = name.m_location;
        if (is(data::DataTokenKind::Assign)) {
            fail(peek().m_location, "ERR_PARSE_EXPECTED",
                 "a constant is written const NAME = value");
        }
        expect(data::DataTokenKind::Equal, "a constant");
        constant->m_value = parse_expression();
        accept(data::DataTokenKind::Semicolon);
        ast.m_constants.push_back(std::move(constant));
    }

    // Parse [ definition || definition ... ].
    void ParserGrammar::parse_parallel(data::DataAst &ast)
    {
        expect(data::DataTokenKind::LeftBracket, "a parallel command");
        do {
            ast.m_definitions.push_back(parse_definition(true));
        } while (accept(data::DataTokenKind::Parallel));
        expect(data::DataTokenKind::RightBracket, "a parallel command");
        accept(data::DataTokenKind::Semicolon);
    }

    // Parse NAME(subscripts) :: commands.
    std::unique_ptr<data::DataDefinition> ParserGrammar::parse_definition(bool inParallel)
    {
        auto definition = std::make_unique<data::DataDefinition>();
        const data::DataToken &name = expect(data::DataTokenKind::Identifier, "a process label");
        definition->m_name = name.m_text;
        definition->m_location = name.m_location;
        definition->m_in_parallel = inParallel;
        if (accept(data::DataTokenKind::LeftParen)) {
            definition->m_has_subscripts = true;
            do {
                definition->m_subscripts.push_back(parse_label_subscript());
            } while (accept(data::DataTokenKind::Comma));
            expect(data::DataTokenKind::RightParen, "a process label");
        }
        expect(data::DataTokenKind::ColonColon, "a process label");
        definition->m_body = parse_commands(inParallel ? data::DataListContext::Element
                                                       : data::DataListContext::TopLevel);
        return definition;
    }

    // Parse i:low..high or a fixed value.
    data::DataLabelSubscript ParserGrammar::parse_label_subscript()
    {
        data::DataLabelSubscript subscript;
        if (is(data::DataTokenKind::Identifier) && is(data::DataTokenKind::Colon, 1)) {
            subscript.m_range = parse_range();
        } else {
            subscript.m_value = parse_expression();
        }
        return subscript;
    }

    // Parse i:low..high.
    std::unique_ptr<data::DataRange> ParserGrammar::parse_range()
    {
        auto range = std::make_unique<data::DataRange>();
        const data::DataToken &variable = expect(data::DataTokenKind::Identifier, "a range");
        range->m_variable = variable.m_text;
        range->m_location = variable.m_location;
        expect(data::DataTokenKind::Colon, "a range");
        range->m_low = parse_expression();
        expect(data::DataTokenKind::DotDot, "a range");
        range->m_high = parse_expression();
        return range;
    }

    // Parse commands separated by semicolons, a trailing one allowed.
    std::vector<std::unique_ptr<data::DataCommand>>
    ParserGrammar::parse_commands(data::DataListContext context)
    {
        std::vector<std::unique_ptr<data::DataCommand>> commands;
        commands.push_back(parse_command());
        while (accept(data::DataTokenKind::Semicolon)) {
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
    bool ParserGrammar::at_list_end(data::DataListContext context) const
    {
        using Kind = data::DataTokenKind;
        if (is(Kind::Eof) || is(Kind::RightBracket) || is(Kind::Box) || is(Kind::Parallel)) {
            return true;
        }
        if (context != data::DataListContext::TopLevel) {
            return false;
        }
        return is(Kind::Const) || starts_definition(0) ||
               (is(Kind::LeftBracket) && bracket_shape(0) == data::DataBracketShape::Parallel);
    }

    // Parse one command by its first token.
    std::unique_ptr<data::DataCommand> ParserGrammar::parse_command()
    {
        using Kind = data::DataTokenKind;
        data::DataLocation location = peek().m_location;
        if (accept(Kind::Skip)) {
            return make_command(data::DataCommandKind::Skip, location);
        }
        if (is(Kind::Star) && is(Kind::LeftBracket, 1)) {
            bool guarded = bracket_shape(1) == data::DataBracketShape::Alternative;
            next();
            next();
            auto command = make_command(data::DataCommandKind::Repetitive, location);
            parse_guarded_commands(*command, guarded);
            expect(Kind::RightBracket, "a repetitive command");
            return command;
        }
        if (is(Kind::LeftBracket)) {
            data::DataBracketShape shape = bracket_shape(0);
            if (shape == data::DataBracketShape::Parallel) {
                fail(location, "ERR_PARSE_NESTED_PARALLEL",
                     "a parallel command inside a process isn't supported; put the processes "
                     "in the top-level one");
            }
            next();
            if (shape == data::DataBracketShape::Alternative) {
                auto command = make_command(data::DataCommandKind::Alternative, location);
                parse_guarded_commands(*command, true);
                expect(Kind::RightBracket, "an alternative command");
                return command;
            }
            auto command = make_command(data::DataCommandKind::Block, location);
            command->m_body = parse_commands(data::DataListContext::Block);
            expect(Kind::RightBracket, "a block");
            return command;
        }
        if (is(Kind::LeftParen)) {
            auto command = make_command(data::DataCommandKind::Assign, location);
            std::unique_ptr<data::DataExpression> targets = parse_primary();
            expect(Kind::Assign, "a tuple assignment");
            std::unique_ptr<data::DataExpression> values = parse_expression();
            if (targets->m_kind == data::DataExpressionKind::Tuple) {
                command->m_targets = std::move(targets->m_args);
            } else {
                command->m_targets.push_back(std::move(targets));
            }
            if (values->m_kind == data::DataExpressionKind::Tuple) {
                command->m_values = std::move(values->m_args);
            } else {
                command->m_values.push_back(std::move(values));
            }
            return command;
        }
        if (is(Kind::Identifier)) {
            return parse_named_command();
        }
        fail(location, "ERR_PARSE_EXPECTED_COMMAND",
             "expected a command, found " + describe(peek()));
    }

    // Parse a declaration, an input, an output, an assignment, a macro or a hook.
    std::unique_ptr<data::DataCommand> ParserGrammar::parse_named_command()
    {
        using Kind = data::DataTokenKind;
        Kind communication = Kind::Eof;
        if (starts_declaration(0)) {
            return parse_declaration();
        }
        if (starts_communication(0, communication)) {
            return parse_communication();
        }
        const data::DataToken &name = next();
        if (is(Kind::Colon) && is(Kind::Equal, 1)) {
            fail(peek().m_location, "ERR_PARSE_COLON_EQUALS",
                 "':' and '=' written apart; an assignment is written ':='");
        }
        std::unique_ptr<data::DataExpression> target =
            make_expression(data::DataExpressionKind::Name, name.m_location);
        target->m_name = name.m_text;
        if (is(Kind::LeftParen)) {
            target->m_kind = data::DataExpressionKind::Call;
            target->m_args = parse_arguments();
        }
        if (accept(Kind::Assign)) {
            auto command = make_command(data::DataCommandKind::Assign, name.m_location);
            command->m_targets.push_back(std::move(target));
            command->m_values.push_back(parse_expression());
            return command;
        }
        auto command = make_command(data::DataCommandKind::Macro, name.m_location);
        command->m_name = name.m_text;
        command->m_has_parens = target->m_kind == data::DataExpressionKind::Call;
        command->m_args = std::move(target->m_args);
        return command;
    }

    // Parse x, y: type.
    std::unique_ptr<data::DataCommand> ParserGrammar::parse_declaration()
    {
        auto command = make_command(data::DataCommandKind::Declare, peek().m_location);
        do {
            const data::DataToken &name = expect(data::DataTokenKind::Identifier, "a declaration");
            command->m_names.push_back(name.m_text);
            command->m_name_locations.push_back(name.m_location);
        } while (accept(data::DataTokenKind::Comma));
        expect(data::DataTokenKind::Colon, "a declaration");
        parse_type(command->m_type);
        return command;
    }

    // Parse (low..high, ...) name.
    void ParserGrammar::parse_type(data::DataTypeSpecifier &type)
    {
        if (accept(data::DataTokenKind::LeftParen)) {
            do {
                type.m_lows.push_back(parse_expression());
                expect(data::DataTokenKind::DotDot, "an array's bounds");
                type.m_highs.push_back(parse_expression());
            } while (accept(data::DataTokenKind::Comma));
            expect(data::DataTokenKind::RightParen, "an array's bounds");
        }
        const data::DataToken &name = expect(data::DataTokenKind::Identifier, "a type");
        type.m_name = name.m_text;
        type.m_location = name.m_location;
    }

    // Parse a process name, ? or !, and the message.
    std::unique_ptr<data::DataCommand> ParserGrammar::parse_communication()
    {
        const data::DataToken &name = next();
        auto command = make_command(data::DataCommandKind::Input, name.m_location);
        command->m_process.m_name = name.m_text;
        command->m_process.m_location = name.m_location;
        if (is(data::DataTokenKind::LeftParen)) {
            command->m_process.m_subscripts = parse_arguments();
        }
        if (next().m_kind == data::DataTokenKind::Bang) {
            command->m_kind = data::DataCommandKind::Output;
        }
        command->m_message = parse_expression();
        return command;
    }

    // Parse guarded commands separated by boxes, or one unguarded list.
    void ParserGrammar::parse_guarded_commands(data::DataCommand &command, bool guarded)
    {
        if (!guarded) {
            auto forever = std::make_unique<data::DataGuardedCommand>();
            forever->m_location = peek().m_location;
            forever->m_body = parse_commands(data::DataListContext::Block);
            command.m_guards.push_back(std::move(forever));
            return;
        }
        do {
            command.m_guards.push_back(parse_guarded_command());
        } while (accept(data::DataTokenKind::Box));
    }

    // Parse range prefixes, conditions, declarations and a last input, then the arrow.
    std::unique_ptr<data::DataGuardedCommand> ParserGrammar::parse_guarded_command()
    {
        using Kind = data::DataTokenKind;
        auto guard = std::make_unique<data::DataGuardedCommand>();
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
            data::DataGuardElement element;
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
                         "the input must be the last element of a guard, found " +
                             describe(peek()));
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
        guard->m_body = parse_commands(data::DataListContext::Guarded);
        return guard;
    }

    // Parse an expression.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_expression()
    {
        return parse_or();
    }

    // Parse left-associative or.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_or()
    {
        std::unique_ptr<data::DataExpression> left = parse_and();
        while (is(data::DataTokenKind::Or)) {
            data::DataLocation location = next().m_location;
            left = make_binary(data::DataTokenKind::Or, location, std::move(left), parse_and());
        }
        return left;
    }

    // Parse left-associative and.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_and()
    {
        std::unique_ptr<data::DataExpression> left = parse_not();
        while (is(data::DataTokenKind::And)) {
            data::DataLocation location = next().m_location;
            left = make_binary(data::DataTokenKind::And, location, std::move(left), parse_not());
        }
        return left;
    }

    // Parse not, which binds looser than a comparison: not a = b is not (a = b).
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_not()
    {
        if (!is(data::DataTokenKind::Not)) {
            return parse_comparison();
        }
        auto expression = make_expression(data::DataExpressionKind::Unary, next().m_location);
        expression->m_op = data::DataTokenKind::Not;
        expression->m_args.push_back(parse_not());
        return expression;
    }

    // Parse one comparison; comparisons don't chain.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_comparison()
    {
        std::unique_ptr<data::DataExpression> left = parse_sum();
        if (!is_comparison(peek().m_kind)) {
            return left;
        }
        const data::DataToken &op = next();
        left = make_binary(op.m_kind, op.m_location, std::move(left), parse_sum());
        if (is_comparison(peek().m_kind)) {
            fail(peek().m_location, "ERR_PARSE_CHAINED_COMPARISON",
                 "comparisons don't chain; join them with 'and'");
        }
        return left;
    }

    // Parse left-associative + and -.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_sum()
    {
        std::unique_ptr<data::DataExpression> left = parse_product();
        while (is(data::DataTokenKind::Plus) || is(data::DataTokenKind::Minus)) {
            const data::DataToken &op = next();
            left = make_binary(op.m_kind, op.m_location, std::move(left), parse_product());
        }
        return left;
    }

    // Parse left-associative *, /, mod and div.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_product()
    {
        using Kind = data::DataTokenKind;
        std::unique_ptr<data::DataExpression> left = parse_unary();
        while (is(Kind::Star) || is(Kind::Slash) || is(Kind::Mod) || is(Kind::Div)) {
            const data::DataToken &op = next();
            left = make_binary(op.m_kind, op.m_location, std::move(left), parse_unary());
        }
        return left;
    }

    // Parse unary minus, and drop unary plus.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_unary()
    {
        if (accept(data::DataTokenKind::Plus)) {
            return parse_unary();
        }
        if (!is(data::DataTokenKind::Minus)) {
            return parse_primary();
        }
        auto expression = make_expression(data::DataExpressionKind::Unary, next().m_location);
        expression->m_op = data::DataTokenKind::Minus;
        expression->m_args.push_back(parse_unary());
        return expression;
    }

    // Parse a literal, a name, a call, a parenthesized expression or a tuple.
    std::unique_ptr<data::DataExpression> ParserGrammar::parse_primary()
    {
        using Kind = data::DataTokenKind;
        const data::DataToken &token = peek();
        switch (token.m_kind) {
            case Kind::Integer: {
                auto literal = make_expression(data::DataExpressionKind::Integer, token.m_location);
                literal->m_integer = next().m_integer;
                return literal;
            }
            case Kind::Real: {
                auto literal = make_expression(data::DataExpressionKind::Real, token.m_location);
                literal->m_real = next().m_real;
                return literal;
            }
            case Kind::Character: {
                auto literal =
                    make_expression(data::DataExpressionKind::Character, token.m_location);
                literal->m_integer = next().m_integer;
                return literal;
            }
            case Kind::True:
            case Kind::False: {
                auto literal = make_expression(data::DataExpressionKind::Boolean, token.m_location);
                literal->m_boolean = next().m_kind == Kind::True;
                return literal;
            }
            case Kind::Identifier: {
                auto name = make_expression(data::DataExpressionKind::Name, token.m_location);
                name->m_name = next().m_text;
                if (is(Kind::LeftParen)) {
                    name->m_kind = data::DataExpressionKind::Call;
                    name->m_args = parse_arguments();
                }
                return name;
            }
            case Kind::LeftParen: {
                data::DataLocation location = next().m_location;
                std::unique_ptr<data::DataExpression> first = parse_expression();
                if (!is(Kind::Comma)) {
                    expect(Kind::RightParen, "a parenthesized expression");
                    return first;
                }
                auto tuple = make_expression(data::DataExpressionKind::Tuple, location);
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
    std::vector<std::unique_ptr<data::DataExpression>> ParserGrammar::parse_arguments()
    {
        std::vector<std::unique_ptr<data::DataExpression>> args;
        expect(data::DataTokenKind::LeftParen, "an argument list");
        if (accept(data::DataTokenKind::RightParen)) {
            return args;
        }
        do {
            args.push_back(parse_expression());
        } while (accept(data::DataTokenKind::Comma));
        expect(data::DataTokenKind::RightParen, "an argument list");
        return args;
    }

    // Join two operands with an operator.
    std::unique_ptr<data::DataExpression>
    ParserGrammar::make_binary(data::DataTokenKind op, const data::DataLocation &location,
                               std::unique_ptr<data::DataExpression> left,
                               std::unique_ptr<data::DataExpression> right)
    {
        auto expression = make_expression(data::DataExpressionKind::Binary, location);
        expression->m_op = op;
        expression->m_args.push_back(std::move(left));
        expression->m_args.push_back(std::move(right));
        return expression;
    }

}
