/*
 * C++ header for cspcc's parser.
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

#ifndef CSPCC_PARSER_GRAMMAR_H
#define CSPCC_PARSER_GRAMMAR_H

#include "cspcc/data/data_ast.h"
#include "cspcc/data/data_bracket_shape.h"
#include "cspcc/data/data_command.h"
#include "cspcc/data/data_definition.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_label_subscript.h"
#include "cspcc/data/data_list_context.h"
#include "cspcc/data/data_range.h"
#include "cspcc/data/data_token.h"
#include "cspcc/data/data_type_specifier.h"
#include "cspcc/diagnostics_engine.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Parses CSP tokens into a syntax tree, by recursive descent. */
    class ParserGrammar
    {
    public:
        /**
         * Prepare to parse tokens.
         *
         * @param tokens Tokens ending with one of kind Eof, which must outlive the parser.
         * @param diagnostics Destination of syntax errors.
         */
        ParserGrammar(const std::vector<data::DataToken> &tokens, DiagnosticsEngine &diagnostics);

        /**
         * Parse a whole program.
         *
         * @return The program, or null after a syntax error.
         */
        std::unique_ptr<data::DataAst> parse_program();

        /**
         * Parse one expression with nothing after it, such as the value of -D.
         *
         * @return The expression, or null after a syntax error.
         */
        std::unique_ptr<data::DataExpression> parse_lone_expression();

    private:
        /**
         * Look at a token ahead of the current one.
         *
         * @param ahead Distance from the current token.
         * @return The token, or the Eof token past the end.
         */
        const data::DataToken &peek(size_t ahead = 0) const;

        /**
         * Check the kind of a token ahead.
         *
         * @param kind Kind to compare with.
         * @param ahead Distance from the current token.
         * @return Whether the token has that kind.
         */
        bool is(data::DataTokenKind kind, size_t ahead = 0) const;

        /**
         * Move past the current token.
         *
         * @return The token moved past.
         */
        const data::DataToken &next();

        /**
         * Move past the current token if it has a kind.
         *
         * @param kind Kind to look for.
         * @return Whether the token was there.
         */
        bool accept(data::DataTokenKind kind);

        /**
         * Move past a token that must have a kind, or stop with an error.
         *
         * @param kind Kind required.
         * @param context What the token belongs to, for the message.
         * @return The token moved past.
         */
        const data::DataToken &expect(data::DataTokenKind kind, std::string_view context);

        /**
         * Report an error and stop the parse.
         *
         * @param location Position of the error.
         * @param code Code of the error.
         * @param message Description of the error.
         */
        [[noreturn]] void fail(const data::DataLocation &location, std::string_view code,
                               std::string_view message);

        /**
         * Find the token after the parenthesis that closes one.
         *
         * @param ahead Distance of a '(' token from the current one.
         * @return Distance of the token after the matching ')', or 0 when there is none.
         */
        size_t after_parens(size_t ahead) const;

        /**
         * Tell what a [ ... ] command holds at its top level.
         *
         * @param ahead Distance of the '[' token from the current one.
         * @return Parallel for '::' or '||', Alternative for an arrow, else Block.
         */
        data::DataBracketShape bracket_shape(size_t ahead) const;

        /**
         * Check whether a label and '::' start at a token.
         *
         * @param ahead Distance of the token from the current one.
         * @return Whether a definition starts there.
         */
        bool starts_definition(size_t ahead) const;

        /**
         * Check whether an input or an output starts at a token.
         *
         * @param ahead Distance of the token from the current one.
         * @param kind Destination of Question or Bang.
         * @return Whether X? or X(...)? or the same with ! starts there.
         */
        bool starts_communication(size_t ahead, data::DataTokenKind &kind) const;

        /**
         * Check whether a declaration, x, y: type, starts at a token.
         *
         * @param ahead Distance of the token from the current one.
         * @return Whether a declaration starts there.
         */
        bool starts_declaration(size_t ahead) const;

        /**
         * Check whether a guard's range prefix, (i:1..n), starts at a token.
         *
         * @param ahead Distance of the token from the current one.
         * @return Whether a range prefix starts there.
         */
        bool starts_range_prefix(size_t ahead) const;

        /**
         * Parse const NAME = value.
         *
         * @param ast Program to add the constant to.
         */
        void parse_constant(data::DataAst &ast);

        /**
         * Parse a parallel command, [ A::... || B(i:1..n)::... ].
         *
         * @param ast Program to add its processes to.
         */
        void parse_parallel(data::DataAst &ast);

        /**
         * Parse label :: commands.
         *
         * @param inParallel Whether the definition is an element of a parallel command.
         * @return The definition.
         */
        std::unique_ptr<data::DataDefinition> parse_definition(bool inParallel);

        /**
         * Parse a label subscript: i:low..high or a value.
         *
         * @return The subscript.
         */
        data::DataLabelSubscript parse_label_subscript();

        /**
         * Parse i:low..high.
         *
         * @return The range.
         */
        std::unique_ptr<data::DataRange> parse_range();

        /**
         * Parse commands separated by semicolons.
         *
         * @param context Where the list appears, which decides what ends it.
         * @return The commands.
         */
        std::vector<std::unique_ptr<data::DataCommand>>
        parse_commands(data::DataListContext context);

        /**
         * Check whether the current token ends a command list.
         *
         * @param context Where the list appears.
         * @return Whether the list ends here.
         */
        bool at_list_end(data::DataListContext context) const;

        /**
         * Parse one command.
         *
         * @return The command.
         */
        std::unique_ptr<data::DataCommand> parse_command();

        /**
         * Parse a command that starts with an identifier.
         *
         * @return The command.
         */
        std::unique_ptr<data::DataCommand> parse_named_command();

        /**
         * Parse x, y: type.
         *
         * @return The declaration.
         */
        std::unique_ptr<data::DataCommand> parse_declaration();

        /**
         * Parse a type, (low..high, ...) name.
         *
         * @param type Destination of the type.
         */
        void parse_type(data::DataTypeSpecifier &type);

        /**
         * Parse X(...)?target or X(...)!value.
         *
         * @return The input or the output.
         */
        std::unique_ptr<data::DataCommand> parse_communication();

        /**
         * Parse the guarded commands of [ ... ] or *[ ... ] after the '['.
         *
         * @param command Alternative or repetitive command to fill.
         * @param guarded Whether an arrow follows; *[ commands ] without one repeats forever.
         */
        void parse_guarded_commands(data::DataCommand &command, bool guarded);

        /**
         * Parse (ranges) elements; input -> commands.
         *
         * @return The guarded command.
         */
        std::unique_ptr<data::DataGuardedCommand> parse_guarded_command();

        /**
         * Parse an expression.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_expression();

        /**
         * Parse operands joined by or.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_or();

        /**
         * Parse operands joined by and.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_and();

        /**
         * Parse not, or a comparison.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_not();

        /**
         * Parse a comparison, or a sum.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_comparison();

        /**
         * Parse terms joined by + and -.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_sum();

        /**
         * Parse factors joined by *, /, mod and div.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_product();

        /**
         * Parse a signed factor.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_unary();

        /**
         * Parse a literal, a name, name(...), (expression) or a tuple.
         *
         * @return The expression.
         */
        std::unique_ptr<data::DataExpression> parse_primary();

        /**
         * Parse ( expression, ... ) after a name.
         *
         * @return The expressions, none for ().
         */
        std::vector<std::unique_ptr<data::DataExpression>> parse_arguments();

        /**
         * Join two operands with an operator.
         *
         * @param op Operator.
         * @param location Position of the operator.
         * @param left Left operand.
         * @param right Right operand.
         * @return The binary expression.
         */
        static std::unique_ptr<data::DataExpression>
        make_binary(data::DataTokenKind op, const data::DataLocation &location,
                    std::unique_ptr<data::DataExpression> left,
                    std::unique_ptr<data::DataExpression> right);

        /** Tokens being parsed. */
        const std::vector<data::DataToken> &m_tokens;

        /** Destination of syntax errors. */
        DiagnosticsEngine &m_diagnostics;

        /** Index of the current token. */
        size_t m_position = 0;
    };

}

#endif
