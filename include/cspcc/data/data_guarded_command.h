/*
 * C++ header for a CSP guarded command.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * cspcc  is  free  software;  you  can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * cspcc  is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with cspcc; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CSPCC_DATA_GUARDED_COMMAND_H
#define CSPCC_DATA_GUARDED_COMMAND_H

#include "cspcc/data/data_guard_element.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_range.h"

#include <memory>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        struct DataCommand;

        /** Describes a guarded command: (ranges) elements; input -> commands. */
        struct DataGuardedCommand
        {
            /** Position of the guard. */
            DataLocation m_location;

            /** Ranges the guarded command is repeated over. */
            std::vector<std::unique_ptr<DataRange>> m_ranges;

            /** Conditions and declarations of the guard. */
            std::vector<DataGuardElement> m_elements;

            /** Input that ends the guard, if any. */
            std::unique_ptr<DataCommand> m_input;

            /** Commands run when the guard is chosen. */
            std::vector<std::unique_ptr<DataCommand>> m_body;
        };

    }
}

#endif
