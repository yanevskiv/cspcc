// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_AST_PRINTER_H
#define CSPCC_AST_PRINTER_H

#include "cspcc/data/data_ast.h"
#include "cspcc/data/data_command.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_process_name.h"
#include "cspcc/data/data_range.h"
#include "cspcc/data/data_token_kind.h"

#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Prints a syntax tree one command per line, for --dump-ast and messages.
    class AstPrinter
    {
    public:
        // Prepare to print to a stream.
        explicit AstPrinter(std::ostream &stream);

        // Print a whole program.
        void print(const data::DataAst &ast);

        // Render an expression on one line, operations fully parenthesized.
        static std::string expression_text(const data::DataExpression *expression);

        // Render a process name on one line.
        static std::string process_text(const data::DataProcessName &process);

        // Spell an operator.
        static std::string_view op_name(data::DataTokenKind op);

    private:
        // Print a list of commands.
        void print_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands,
                            int depth);

        // Print one command and what it holds.
        void print_command(const data::DataCommand &command, int depth);

        // Render a range on one line.
        static std::string range_text(const data::DataRange &range);

        // Render a list of expressions separated by commas.
        static std::string
        list_text(const std::vector<std::unique_ptr<data::DataExpression>> &expressions);

        // Destination of the tree.
        std::ostream &m_stream;
    };

}

#endif
