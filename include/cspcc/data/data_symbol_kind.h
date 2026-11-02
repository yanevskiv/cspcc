// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_SYMBOL_KIND_H
#define CSPCC_DATA_SYMBOL_KIND_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes what a name stands for.
        enum class DataSymbolKind {
            Constant,
            Variable,
            Index,
            Bound,
        };

    }
}

#endif
