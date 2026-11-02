/*
 * C++ header for an element of a CSP guard.
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

#ifndef CSPCC_DATA_GUARD_ELEMENT_H
#define CSPCC_DATA_GUARD_ELEMENT_H

#include "cspcc/data/data_expression.h"

#include <memory>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        struct DataCommand;

        /** Describes an element of a guard: a condition or a declaration. */
        struct DataGuardElement
        {
            /** Boolean condition, when the element is one. */
            std::unique_ptr<DataExpression> m_condition;

            /** Declaration, when the element is one. */
            std::unique_ptr<DataCommand> m_declaration;
        };

    }
}

#endif
