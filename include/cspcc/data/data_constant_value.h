/*
 * C++ header for a constant waiting to be resolved.
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

#ifndef CSPCC_DATA_CONSTANT_VALUE_H
#define CSPCC_DATA_CONSTANT_VALUE_H

#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_location.h"

#include <string>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a constant's name and value, from a const or a -D. */
        struct DataConstantValue
        {
            /** Name of the constant. */
            std::string m_name;

            /** Position of the const, or none for a -D. */
            DataLocation m_location;

            /** Value, owned by the syntax tree or the program. */
            DataExpression *m_value = nullptr;
        };

    }
}

#endif
