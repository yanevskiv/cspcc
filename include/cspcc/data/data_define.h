// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_DEFINE_H
#define CSPCC_DATA_DEFINE_H

#include <string>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a -D NAME=VALUE option.
        struct DataDefine
        {
            // Name of the constant.
            std::string m_name;

            // Value as written.
            std::string m_value;
        };

    }
}

#endif
