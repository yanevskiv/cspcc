// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_PROCESS_NAME_H
#define CSPCC_DATA_PROCESS_NAME_H

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
        struct DataFamily;

        // Describes a process name such as X, X(i) or M(i, j - 1).
        struct DataProcessName
        {
            // Name of the process or of its array.
            std::string m_name;

            // Position of the name.
            DataLocation m_location;

            // Subscripts of an element of a process array.
            std::vector<std::unique_ptr<DataExpression>> m_subscripts;

            // Family sema resolves the name to.
            DataFamily *m_family = nullptr;
        };

    }
}

#endif
