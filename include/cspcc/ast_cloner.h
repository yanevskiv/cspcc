// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_AST_CLONER_H
#define CSPCC_AST_CLONER_H

#include "cspcc/data/data_command.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_range.h"

#include <memory>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Copies syntax trees, leaving out what sema fills in, for expanding macros.
    class AstCloner
    {
    public:
        // Copy an expression.
        static std::unique_ptr<data::DataExpression>
        clone_expression(const data::DataExpression *expression);

        // Copy a command.
        static std::unique_ptr<data::DataCommand> clone_command(const data::DataCommand *command);

        // Copy a list of commands.
        static std::vector<std::unique_ptr<data::DataCommand>>
        clone_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands);

    private:
        // Copy a list of expressions.
        static std::vector<std::unique_ptr<data::DataExpression>>
        clone_expressions(const std::vector<std::unique_ptr<data::DataExpression>> &expressions);

        // Copy a range.
        static std::unique_ptr<data::DataRange> clone_range(const data::DataRange &range);

        // Copy a guarded command.
        static std::unique_ptr<data::DataGuardedCommand>
        clone_guarded(const data::DataGuardedCommand &guard);
    };

}

#endif
