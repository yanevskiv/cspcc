/*
 * C++ header for deep copies of CSP syntax trees.
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

#ifndef CSPCC_AST_CLONER_H
#define CSPCC_AST_CLONER_H

#include "cspcc/data/data_command.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_range.h"

#include <memory>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Copies syntax trees, leaving out what sema fills in, for expanding macros. */
    class AstCloner
    {
    public:
        /**
         * Copy an expression.
         *
         * @param expression Expression to copy, or null.
         * @return The copy, or null.
         */
        static std::unique_ptr<data::DataExpression>
        clone_expression(const data::DataExpression *expression);

        /**
         * Copy a command.
         *
         * @param command Command to copy, or null.
         * @return The copy, or null.
         */
        static std::unique_ptr<data::DataCommand> clone_command(const data::DataCommand *command);

        /**
         * Copy a list of commands.
         *
         * @param commands Commands to copy.
         * @return The copies.
         */
        static std::vector<std::unique_ptr<data::DataCommand>>
        clone_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands);

    private:
        /**
         * Copy a list of expressions.
         *
         * @param expressions Expressions to copy.
         * @return The copies.
         */
        static std::vector<std::unique_ptr<data::DataExpression>>
        clone_expressions(const std::vector<std::unique_ptr<data::DataExpression>> &expressions);

        /**
         * Copy a range.
         *
         * @param range Range to copy.
         * @return The copy.
         */
        static std::unique_ptr<data::DataRange> clone_range(const data::DataRange &range);

        /**
         * Copy a guarded command.
         *
         * @param guard Guarded command to copy.
         * @return The copy.
         */
        static std::unique_ptr<data::DataGuardedCommand>
        clone_guarded(const data::DataGuardedCommand &guard);
    };

}

#endif
