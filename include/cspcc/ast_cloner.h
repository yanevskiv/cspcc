// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_AST_CLONER_H
#define CSPCC_AST_CLONER_H

#include "cspcc/ast.h"

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
        static std::unique_ptr<Expression>
        clone_expression(const Expression *expression);

        // Copy a command.
        static std::unique_ptr<Command> clone_command(const Command *command);

        // Copy a list of commands.
        static std::vector<std::unique_ptr<Command>>
        clone_commands(const std::vector<std::unique_ptr<Command>> &commands);

    private:
        // Copy a list of expressions.
        static std::vector<std::unique_ptr<Expression>>
        clone_expressions(const std::vector<std::unique_ptr<Expression>> &expressions);

        // Copy a range.
        static std::unique_ptr<Range> clone_range(const Range &range);

        // Copy a guarded command.
        static std::unique_ptr<GuardedCommand>
        clone_guarded(const GuardedCommand &guard);
    };

}

#endif
