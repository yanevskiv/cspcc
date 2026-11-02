// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_LIST_CONTEXT_H
#define CSPCC_DATA_LIST_CONTEXT_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes where a command list appears, which decides what ends it.
        enum class DataListContext {
            TopLevel,
            Element,
            Guarded,
            Block,
        };

    }
}

#endif
