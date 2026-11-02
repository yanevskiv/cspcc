// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_ENTRY_H
#define CSPCC_DATA_ENTRY_H

#include "cspcc/data/data_type_base.h"

#include <string>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes one kind of message a process family receives.
        struct DataEntry
        {
            // Constructor of the message, empty when it has none.
            std::string m_constructor;

            // Whether the message is a plain value or tuple.
            bool m_anonymous = true;

            // Types of the message's parts.
            std::vector<DataTypeBase> m_types;

            // Name of the Ada entry.
            std::string m_ada;
        };

    }
}

#endif
