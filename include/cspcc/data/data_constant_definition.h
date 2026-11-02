// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_CONSTANT_DEFINITION_H
#define CSPCC_DATA_CONSTANT_DEFINITION_H

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
        // Describes a named constant.
        struct DataConstantDefinition
        {
            // Name of the constant.
            std::string m_name;

            // Position of the name.
            DataLocation m_location;

            // Value as written.
            std::unique_ptr<DataExpression> m_value;
        };

    }
}

#endif
