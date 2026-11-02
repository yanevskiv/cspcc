// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_LABEL_SUBSCRIPT_H
#define CSPCC_DATA_LABEL_SUBSCRIPT_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_range.h"

#include <memory>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a label subscript: a fixed value or a range.
        struct DataLabelSubscript
        {
            // Fixed value, when the subscript is one.
            std::unique_ptr<DataExpression> m_value;

            // Range, when the subscript is one.
            std::unique_ptr<DataRange> m_range;
        };

    }
}

#endif
