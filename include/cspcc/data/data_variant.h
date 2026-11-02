// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_VARIANT_H
#define CSPCC_DATA_VARIANT_H

#include "cspcc/data/data_definition.h"
#include "cspcc/data/data_symbol.h"

#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes one definition of a family, such as M(i:1..3, 0)::WEST.
        struct DataVariant
        {
            // Definition the variant comes from.
            DataDefinition *m_definition = nullptr;

            // Symbols of the label's ranges.
            std::vector<DataSymbol *> m_index_symbols;

            // Variables and bound variables declared in the body.
            std::vector<DataSymbol *> m_locals;
        };

    }
}

#endif
