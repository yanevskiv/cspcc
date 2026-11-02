// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_GUARD_ELEMENT_H
#define CSPCC_DATA_GUARD_ELEMENT_H

#include "cspcc/data/data_expression.h"

#include <memory>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        struct DataCommand;

        // Describes an element of a guard: a condition or a declaration.
        struct DataGuardElement
        {
            // Boolean condition, when the element is one.
            std::unique_ptr<DataExpression> m_condition;

            // Declaration, when the element is one.
            std::unique_ptr<DataCommand> m_declaration;
        };

    }
}

#endif
