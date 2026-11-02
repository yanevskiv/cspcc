// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DIAGNOSTICS_ABORT_H
#define CSPCC_DIAGNOSTICS_ABORT_H

#include <exception>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Signals an error, already reported, that the parser can't recover from.
    class DiagnosticsAbort : public std::exception
    {
    public:
        // Describe the exception.
        const char *what() const noexcept override;
    };

}

#endif
