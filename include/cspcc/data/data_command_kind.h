// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_COMMAND_KIND_H
#define CSPCC_DATA_COMMAND_KIND_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes the kind of a command.
        enum class DataCommandKind {
            Skip,
            Assign,
            Declare,
            Input,
            Output,
            Macro,
            Block,
            Alternative,
            Repetitive,
        };

    }
}

#endif
