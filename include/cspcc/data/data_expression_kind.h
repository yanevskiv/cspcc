// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_EXPRESSION_KIND_H
#define CSPCC_DATA_EXPRESSION_KIND_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes the kind of an expression or of a target.
        enum class DataExpressionKind {
            Integer,
            Real,
            Character,
            Boolean,
            Name,
            Call,
            Unary,
            Binary,
            Tuple,
        };

    }
}

#endif
