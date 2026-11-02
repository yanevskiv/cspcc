// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_CONSTANT_VALUE_H
#define CSPCC_DATA_CONSTANT_VALUE_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"

#include <string>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a constant's name and value, from a const or a -D.
        struct DataConstantValue
        {
            // Name of the constant.
            std::string m_name;

            // Position of the const, or none for a -D.
            DataLocation m_location;

            // Value, owned by the syntax tree or the program.
            DataExpression *m_value = nullptr;
        };

    }
}

#endif
