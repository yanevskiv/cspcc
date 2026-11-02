// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_PROGRAM_H
#define CSPCC_DATA_PROGRAM_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_family.h"
#include "cspcc/data/data_symbol.h"

#include <memory>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a checked program: its constants and its process families.
        struct DataProgram
        {
            // Constants in the order they are defined.
            std::vector<DataSymbol *> m_constants;

            // Process families in source order.
            std::vector<std::unique_ptr<DataFamily>> m_families;

            // Every symbol of the program.
            std::vector<std::unique_ptr<DataSymbol>> m_symbols;

            // Expressions sema creates, such as the values of -D.
            std::vector<std::unique_ptr<DataExpression>> m_expressions;

            // Number of processes.
            int m_total = 0;
        };

    }
}

#endif
