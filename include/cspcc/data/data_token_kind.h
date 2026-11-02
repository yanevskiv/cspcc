/*
 * C++ header for the kinds of CSP tokens.
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

#ifndef CSPCC_DATA_TOKEN_KIND_H
#define CSPCC_DATA_TOKEN_KIND_H

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Namespace for cspcc data declarations. */
    namespace data
    {
        /** Describes the kind of a CSP token. */
        enum class DataTokenKind {
            Eof,
            Identifier,
            Integer,
            Real,
            Character,
            LeftBracket,
            RightBracket,
            LeftParen,
            RightParen,
            ColonColon,
            Colon,
            Assign,
            Parallel,
            Box,
            Arrow,
            Bang,
            Question,
            Semicolon,
            Comma,
            DotDot,
            Equal,
            NotEqual,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
            Plus,
            Minus,
            Star,
            Slash,
            Skip,
            True,
            False,
            Mod,
            Div,
            And,
            Or,
            Not,
            Const,
        };

    }
}

#endif
