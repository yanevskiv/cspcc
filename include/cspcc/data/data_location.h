// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_LOCATION_H
#define CSPCC_DATA_LOCATION_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a position in a CSP source file.
        struct DataLocation
        {
            // Line counted from 1, or 0 when there is no position.
            int m_line = 0;

            // Column in characters counted from 1.
            int m_column = 0;
        };

    }
}

#endif
