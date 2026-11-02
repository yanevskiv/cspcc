// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_TOKEN_KIND_H
#define CSPCC_DATA_TOKEN_KIND_H

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        // Describes the kind of a CSP token.
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
