/*
 * C++ header for one definition of a CSP process family.
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

#ifndef CSPCC_DATA_VARIANT_H
#define CSPCC_DATA_VARIANT_H

#include "cspcc/data/data_definition.h"
#include "cspcc/data/data_symbol.h"

#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes one definition of a family, such as M(i:1..3, 0)::WEST. */
        struct DataVariant
        {
            /** Definition the variant comes from. */
            DataDefinition *m_definition = nullptr;

            /** Symbols of the label's ranges. */
            std::vector<DataSymbol *> m_index_symbols;

            /** Variables and bound variables declared in the body. */
            std::vector<DataSymbol *> m_locals;
        };

    }
}

#endif
