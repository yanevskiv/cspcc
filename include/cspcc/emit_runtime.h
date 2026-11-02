/*
 * C++ header for the Ada runtime cspcc writes next to its output.
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

#ifndef CSPCC_EMIT_RUNTIME_H
#define CSPCC_EMIT_RUNTIME_H

#include <string_view>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Holds the sources of the Csp_Runtime package, embedded at build time. */
    class EmitRuntime
    {
    public:
        /**
         * Return the package's spec.
         *
         * @return Text of csp_runtime.ads.
         */
        static std::string_view spec();

        /**
         * Return the package's body.
         *
         * @return Text of csp_runtime.adb.
         */
        static std::string_view body();
    };

}

#endif
