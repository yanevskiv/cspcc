// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_RANGE_H
#define CSPCC_DATA_RANGE_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"

#include <memory>
#include <string>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        struct DataSymbol;

        // Describes a range in a process label or a guard.
        struct DataRange
        {
            // Name of the bound variable.
            std::string m_variable;

            // Position of the bound variable.
            DataLocation m_location;

            // Lower bound as written.
            std::unique_ptr<DataExpression> m_low;

            // Upper bound as written.
            std::unique_ptr<DataExpression> m_high;

            // Lower bound sema computes.
            long long m_first = 0;

            // Upper bound sema computes.
            long long m_last = 0;

            // Symbol of the bound variable.
            DataSymbol *m_symbol = nullptr;
        };

    }
}

#endif
