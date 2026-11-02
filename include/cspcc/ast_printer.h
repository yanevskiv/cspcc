/*
 * C++ header for printing CSP syntax trees.
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

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Prints a syntax tree one command per line, for --dump-ast and messages. */
    class AstPrinter
    {
    public:
        /**
         * Prepare to print to a stream.
         *
         * @param stream Destination of the tree.
         */
        explicit AstPrinter(std::ostream &stream);

        /**
         * Print a whole program.
         *
         * @param ast Program to print.
         */
        void print(const data::DataAst &ast);

        /**
         * Render an expression on one line, operations fully parenthesized.
         *
         * @param expression Expression to render, or null.
         * @return The text.
         */
        static std::string expression_text(const data::DataExpression *expression);

        /**
         * Render a process name on one line.
         *
         * @param process Process name to render.
         * @return The text, such as Fork((i + 1)).
         */
        static std::string process_text(const data::DataProcessName &process);

        /**
         * Spell an operator.
         *
         * @param op Operator.
         * @return Its CSP spelling.
         */
        static std::string_view op_name(data::DataTokenKind op);

    private:
        /**
         * Print a list of commands.
         *
         * @param commands Commands to print.
         * @param depth Indentation level.
         */
        void print_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands,
                            int depth);

        /**
         * Print one command and what it holds.
         *
         * @param command Command to print.
         * @param depth Indentation level.
         */
        void print_command(const data::DataCommand &command, int depth);

        /**
         * Render a range on one line.
         *
         * @param range Range to render.
         * @return The text, such as i:1..n.
         */
        static std::string range_text(const data::DataRange &range);

        /**
         * Render a list of expressions separated by commas.
         *
         * @param expressions Expressions to render.
         * @return The text.
         */
        static std::string
        list_text(const std::vector<std::unique_ptr<data::DataExpression>> &expressions);

        /** Destination of the tree. */
        std::ostream &m_stream;
    };

}

#endif
