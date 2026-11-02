/*
 * C++ header for one CSP token.
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

#ifndef CSPCC_DATA_TOKEN_H
#define CSPCC_DATA_TOKEN_H

#include "cspcc/data/data_location.h"
#include "cspcc/data/data_token_kind.h"

#include <string>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes a CSP token, its spelling and its value. */
        struct DataToken
        {
            /** Kind of the token. */
            DataTokenKind m_kind = DataTokenKind::Eof;

            /** Spelling of the token in the source. */
            std::string m_text;

            /** Position of the token's first character. */
            DataLocation m_location;

            /** Value of an integer or the code of a character. */
            long long m_integer = 0;

            /** Value of a real number. */
            double m_real = 0;
        };

    }
}

#endif
