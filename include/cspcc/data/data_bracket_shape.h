// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_BRACKET_SHAPE_H
#define CSPCC_DATA_BRACKET_SHAPE_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a [ ... ] command by what its top level holds.
        enum class DataBracketShape {
            Block,
            Alternative,
            Parallel,
        };

    }
}

#endif
