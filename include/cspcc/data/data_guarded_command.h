// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_GUARDED_COMMAND_H
#define CSPCC_DATA_GUARDED_COMMAND_H

#include "cspcc/data/data_guard_element.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_range.h"

#include <memory>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        struct DataCommand;

        // Describes a guarded command: (ranges) elements; input -> commands.
        struct DataGuardedCommand
        {
            // Position of the guard.
            DataLocation m_location;

            // Ranges the guarded command is repeated over.
            std::vector<std::unique_ptr<DataRange>> m_ranges;

            // Conditions and declarations of the guard.
            std::vector<DataGuardElement> m_elements;

            // Input that ends the guard, if any.
            std::unique_ptr<DataCommand> m_input;

            // Commands run when the guard is chosen.
            std::vector<std::unique_ptr<DataCommand>> m_body;
        };

    }
}

#endif
