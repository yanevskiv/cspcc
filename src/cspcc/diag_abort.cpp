/*
 * C++ file for the exception that stops a parse.
 *
 * Copyright (C) 2026 Ivan Janevski
 *
 * cspcc  is  free  software;  you  can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free  Software  Foundation;  either  version 3 of the License, or (at
 * your option) any later version.
 *
 * cspcc  is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS  FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You  should  have  received  a copy of the GNU General Public License
 * along with cspcc; if not, see <https://www.gnu.org/licenses/>.
 */

// Project header for the exception.
#include "cspcc/diag_abort.h"

// Namespace for cspcc's diagnostics.
namespace cspcc
{

    // Describe the exception.
    const char *DiagAbort::what() const noexcept
    {
        return "cspcc: parse aborted";
    }

}
