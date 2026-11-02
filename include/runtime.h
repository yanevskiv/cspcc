// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_RUNTIME_H
#define CSPCC_RUNTIME_H

#include <string_view>

// Return the spec of Csp_Runtime, the Ada package each translated program uses.
std::string_view runtime_spec();

// Return the body of Csp_Runtime.
std::string_view runtime_body();

#endif
