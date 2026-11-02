// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_INSTANCE_H
#define CSPCC_DATA_INSTANCE_H

#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes one process: its subscripts, its body and its number.
        struct DataInstance
        {
            // Subscripts of the process.
            std::vector<long long> m_indices;

            // Variant whose commands the process runs, from 1.
            int m_variant = 0;

            // Number of the process in the whole program, from 1.
            int m_pid = 0;
        };

    }
}

#endif
