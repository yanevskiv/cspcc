// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_TOKEN_H
#define CSPCC_DATA_TOKEN_H

#include "cspcc/data/data_location.h"
#include "cspcc/data/data_token_kind.h"

#include <string>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes a CSP token, its spelling and its value.
        struct DataToken
        {
            // Kind of the token.
            DataTokenKind m_kind = DataTokenKind::Eof;

            // Spelling of the token in the source.
            std::string m_text;

            // Position of the token's first character.
            DataLocation m_location;

            // Value of an integer or the code of a character.
            long long m_integer = 0;

            // Value of a real number.
            double m_real = 0;
        };

    }
}

#endif
