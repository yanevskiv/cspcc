/*
 * C++ header for the exception that stops a parse.
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

#ifndef CSPCC_DIAG_ABORT_H
#define CSPCC_DIAG_ABORT_H

#include <exception>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Signals an error, already reported, that the parser can't recover from. */
    class DiagAbort : public std::exception
    {
    public:
        /**
         * Describe the exception.
         *
         * @return A fixed description.
         */
        const char *what() const noexcept override;
    };

}

#endif
