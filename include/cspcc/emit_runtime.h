// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMIT_RUNTIME_H
#define CSPCC_EMIT_RUNTIME_H

#include <string_view>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Holds the sources of the Csp_Runtime package, embedded at build time.
    class EmitRuntime
    {
    public:
        // Return the package's spec.
        static std::string_view spec();

        // Return the package's body.
        static std::string_view body();
    };

}

#endif
