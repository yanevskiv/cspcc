// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_AST_H
#define CSPCC_DATA_AST_H

#include "cspcc/data/data_constant_definition.h"
#include "cspcc/data/data_definition.h"

#include <memory>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a parsed program: its constants and its definitions.
        struct DataAst
        {
            // Constants in source order.
            std::vector<std::unique_ptr<DataConstantDefinition>> m_constants;

            // Process and macro definitions in source order.
            std::vector<std::unique_ptr<DataDefinition>> m_definitions;
        };

    }
}

#endif
