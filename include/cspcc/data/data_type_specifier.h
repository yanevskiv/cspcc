// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_TYPE_SPECIFIER_H
#define CSPCC_DATA_TYPE_SPECIFIER_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"

#include <memory>
#include <string>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a declared type: a name and, for an array, its bounds.
        struct DataTypeSpecifier
        {
            // Name of the scalar type.
            std::string m_name;

            // Position of the type's name.
            DataLocation m_location;

            // Lower bound of each dimension.
            std::vector<std::unique_ptr<DataExpression>> m_lows;

            // Upper bound of each dimension.
            std::vector<std::unique_ptr<DataExpression>> m_highs;
        };

    }
}

#endif
