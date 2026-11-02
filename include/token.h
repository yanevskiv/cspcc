// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_TOKEN_H
#define CSPCC_TOKEN_H

#include <string>

// Describes a position in a CSP source file.
struct Location
{
    // Line counted from 1, or 0 when there is no position.
    int m_line = 0;

    // Column in characters counted from 1.
    int m_column = 0;
};

// Describes the kind of a CSP token.
enum class TokenKind {
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

// Describes a CSP token, its spelling and its value.
struct Token
{
    // Kind of the token.
    TokenKind m_kind = TokenKind::Eof;

    // Spelling of the token in the source.
    std::string m_text;

    // Position of the token's first character.
    Location m_location;

    // Value of an integer or the code of a character.
    long long m_integer = 0;

    // Value of a real number.
    double m_real = 0;
};

#endif
