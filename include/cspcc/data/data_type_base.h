// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_TYPE_BASE_H
#define CSPCC_DATA_TYPE_BASE_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a scalar type, or the element type of an array.
        enum class DataTypeBase {
            None,
            Integer,
            Real,
            Character,
            Boolean,
        };

    }
}

#endif
