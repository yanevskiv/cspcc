// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

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

// Namespace for cspcc declarations.
namespace cspcc
{
    // Parses CSP tokens into a syntax tree, by recursive descent.
    class ParserGrammar
    {
    public:
        // Prepare to parse tokens.
        ParserGrammar(const std::vector<data::DataToken> &tokens, DiagnosticsEngine &diagnostics);

        // Parse a whole program.
        std::unique_ptr<data::DataAst> parse_program();

        // Parse one expression with nothing after it, such as the value of -D.
        std::unique_ptr<data::DataExpression> parse_lone_expression();

    private:
        // Look at a token ahead of the current one.
        const data::DataToken &peek(size_t ahead = 0) const;

        // Check the kind of a token ahead.
        bool is(data::DataTokenKind kind, size_t ahead = 0) const;

        // Move past the current token.
        const data::DataToken &next();

        // Move past the current token if it has a kind.
        bool accept(data::DataTokenKind kind);

        // Move past a token that must have a kind, or stop with an error.
        const data::DataToken &expect(data::DataTokenKind kind, std::string_view context);

        // Report an error and stop the parse.
        [[noreturn]] void fail(const data::DataLocation &location, std::string_view code,
                               std::string_view message);

        // Find the token after the parenthesis that closes one.
        size_t after_parens(size_t ahead) const;

        // Tell what a [ ... ] command holds at its top level.
        data::DataBracketShape bracket_shape(size_t ahead) const;

        // Check whether a label and '::' start at a token.
        bool starts_definition(size_t ahead) const;

        // Check whether an input or an output starts at a token.
        bool starts_communication(size_t ahead, data::DataTokenKind &kind) const;

        // Check whether a declaration, x, y: type, starts at a token.
        bool starts_declaration(size_t ahead) const;

        // Check whether a guard's range prefix, (i:1..n), starts at a token.
        bool starts_range_prefix(size_t ahead) const;

        // Parse const NAME = value.
        void parse_constant(data::DataAst &ast);

        // Parse a parallel command, [ A::... || B(i:1..n)::... ].
        void parse_parallel(data::DataAst &ast);

        // Parse label :: commands.
        std::unique_ptr<data::DataDefinition> parse_definition(bool inParallel);

        // Parse a label subscript: i:low..high or a value.
        data::DataLabelSubscript parse_label_subscript();

        // Parse i:low..high.
        std::unique_ptr<data::DataRange> parse_range();

        // Parse commands separated by semicolons.
        std::vector<std::unique_ptr<data::DataCommand>>
        parse_commands(data::DataListContext context);

        // Check whether the current token ends a command list.
        bool at_list_end(data::DataListContext context) const;

        // Parse one command.
        std::unique_ptr<data::DataCommand> parse_command();

        // Parse a command that starts with an identifier.
        std::unique_ptr<data::DataCommand> parse_named_command();

        // Parse x, y: type.
        std::unique_ptr<data::DataCommand> parse_declaration();

        // Parse a type, (low..high, ...) name.
        void parse_type(data::DataTypeSpecifier &type);

        // Parse X(...)?target or X(...)!value.
        std::unique_ptr<data::DataCommand> parse_communication();

        // Parse the guarded commands of [ ... ] or *[ ... ] after the '['.
        void parse_guarded_commands(data::DataCommand &command, bool guarded);

        // Parse (ranges) elements; input -> commands.
        std::unique_ptr<data::DataGuardedCommand> parse_guarded_command();

        // Parse an expression.
        std::unique_ptr<data::DataExpression> parse_expression();

        // Parse operands joined by or.
        std::unique_ptr<data::DataExpression> parse_or();

        // Parse operands joined by and.
        std::unique_ptr<data::DataExpression> parse_and();

        // Parse not, or a comparison.
        std::unique_ptr<data::DataExpression> parse_not();

        // Parse a comparison, or a sum.
        std::unique_ptr<data::DataExpression> parse_comparison();

        // Parse terms joined by + and -.
        std::unique_ptr<data::DataExpression> parse_sum();

        // Parse factors joined by *, /, mod and div.
        std::unique_ptr<data::DataExpression> parse_product();

        // Parse a signed factor.
        std::unique_ptr<data::DataExpression> parse_unary();

        // Parse a literal, a name, name(...), (expression) or a tuple.
        std::unique_ptr<data::DataExpression> parse_primary();

        // Parse ( expression, ... ) after a name.
        std::vector<std::unique_ptr<data::DataExpression>> parse_arguments();

        // Join two operands with an operator.
        static std::unique_ptr<data::DataExpression>
        make_binary(data::DataTokenKind op, const data::DataLocation &location,
                    std::unique_ptr<data::DataExpression> left,
                    std::unique_ptr<data::DataExpression> right);

        // Tokens being parsed.
        const std::vector<data::DataToken> &m_tokens;

        // Destination of syntax errors.
        DiagnosticsEngine &m_diagnostics;

        // Index of the current token.
        size_t m_position = 0;
    };

}

#endif
