// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/diagnostics_abort.h"

namespace cspcc
{

    // Describe the exception.
    const char *DiagnosticsAbort::what() const noexcept
    {
        return "cspcc: parse aborted";
    }

}
