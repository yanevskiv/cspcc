/*
 * C++ header for a parsed CSP program.
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

#ifndef CSPCC_DATA_AST_H
#define CSPCC_DATA_AST_H

#include "cspcc/data/data_constant_definition.h"
#include "cspcc/data/data_definition.h"

#include <memory>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a parsed program: its constants and its definitions. */
        struct DataAst
        {
            /** Constants in source order. */
            std::vector<std::unique_ptr<DataConstantDefinition>> m_constants;

            /** Process and macro definitions in source order. */
            std::vector<std::unique_ptr<DataDefinition>> m_definitions;
        };

    }
}

#endif
