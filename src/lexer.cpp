// Splits a CSP source into tokens.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "lexer.h"

#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

// Largest integer literal, Ada's Integer'Last.
constexpr long long maxInteger = 2147483647LL;

// Keywords, all lower case.
const std::map<std::string, TokenKind, std::less<>> keywords = {
    {"skip", TokenKind::Skip}, {"true", TokenKind::True}, {"false", TokenKind::False},
    {"mod", TokenKind::Mod},   {"div", TokenKind::Div},   {"and", TokenKind::And},
    {"or", TokenKind::Or},     {"not", TokenKind::Not},   {"const", TokenKind::Const},
};

// Tokens spelled with one non-ASCII character, as on the slides.
const std::map<unsigned, TokenKind> unicodeSymbols = {
    {0x2192, TokenKind::Arrow},     {0x25A1, TokenKind::Box},
    {0x2016, TokenKind::Parallel},  {0x2260, TokenKind::NotEqual},
    {0x2264, TokenKind::LessEqual}, {0x2265, TokenKind::GreaterEqual},
    {0x00AC, TokenKind::Not},       {0x2227, TokenKind::And},
    {0x2228, TokenKind::Or},
};

// Format a code point as U+XXXX.
static std::string code_point_name(unsigned codePoint)
{
    char buffer[16];
    std::snprintf(buffer, sizeof buffer, "U+%04X", codePoint);
    return buffer;
}

// Prepare to split one source.
Lexer::Lexer(std::string_view source, Diagnostics &diagnostics)
    : m_source(source)
    , m_diagnostics(diagnostics)
{}

// Split the whole source, skipping blanks and comments.
std::vector<Token> Lexer::tokenize()
{
    while (m_position < m_source.size()) {
        unsigned char c = peek();
        if (std::isspace(c)) {
            advance();
            continue;
        }
        if (skip_comment()) {
            continue;
        }
        Location location = here();
        if (std::isalpha(c)) {
            scan_identifier(location);
        } else if (std::isdigit(c)) {
            scan_number(location);
        } else if (c == '\'' || c == '"') {
            scan_character(location);
        } else if (c >= 0x80) {
            scan_symbol(location);
        } else {
            scan_punctuation(location);
        }
    }
    Token eof;
    eof.m_location = here();
    m_tokens.push_back(eof);
    return std::move(m_tokens);
}

// Name a kind of token.
std::string_view Lexer::kind_name(TokenKind kind)
{
    switch (kind) {
        case TokenKind::Eof:
            return "end of file";
        case TokenKind::Identifier:
            return "identifier";
        case TokenKind::Integer:
            return "integer";
        case TokenKind::Real:
            return "real number";
        case TokenKind::Character:
            return "character";
        case TokenKind::LeftBracket:
            return "'['";
        case TokenKind::RightBracket:
            return "']'";
        case TokenKind::LeftParen:
            return "'('";
        case TokenKind::RightParen:
            return "')'";
        case TokenKind::ColonColon:
            return "'::'";
        case TokenKind::Colon:
            return "':'";
        case TokenKind::Assign:
            return "':='";
        case TokenKind::Parallel:
            return "'‖' or '||'";
        case TokenKind::Box:
            return "'□' or '[]'";
        case TokenKind::Arrow:
            return "'→' or '->'";
        case TokenKind::Bang:
            return "'!'";
        case TokenKind::Question:
            return "'?'";
        case TokenKind::Semicolon:
            return "';'";
        case TokenKind::Comma:
            return "','";
        case TokenKind::DotDot:
            return "'..'";
        case TokenKind::Equal:
            return "'='";
        case TokenKind::NotEqual:
            return "'<>'";
        case TokenKind::Less:
            return "'<'";
        case TokenKind::LessEqual:
            return "'<='";
        case TokenKind::Greater:
            return "'>'";
        case TokenKind::GreaterEqual:
            return "'>='";
        case TokenKind::Plus:
            return "'+'";
        case TokenKind::Minus:
            return "'-'";
        case TokenKind::Star:
            return "'*'";
        case TokenKind::Slash:
            return "'/'";
        case TokenKind::Skip:
            return "'skip'";
        case TokenKind::True:
            return "'true'";
        case TokenKind::False:
            return "'false'";
        case TokenKind::Mod:
            return "'mod'";
        case TokenKind::Div:
            return "'div'";
        case TokenKind::And:
            return "'and'";
        case TokenKind::Or:
            return "'or'";
        case TokenKind::Not:
            return "'not'";
        case TokenKind::Const:
            return "'const'";
    }
    return "token";
}

// Print each token's position, kind and value.
void Lexer::dump(const std::vector<Token> &tokens, std::ostream &stream)
{
    for (const Token &token : tokens) {
        stream << token.m_location.m_line << ':' << token.m_location.m_column << ' '
               << kind_name(token.m_kind);
        if (token.m_kind == TokenKind::Identifier) {
            stream << ' ' << token.m_text;
        } else if (token.m_kind == TokenKind::Integer || token.m_kind == TokenKind::Character) {
            stream << ' ' << token.m_integer;
        } else if (token.m_kind == TokenKind::Real) {
            stream << ' ' << token.m_real;
        }
        stream << '\n';
    }
}

// Look at a byte ahead.
unsigned char Lexer::peek(size_t ahead) const
{
    size_t at = m_position + ahead;
    return at < m_source.size() ? static_cast<unsigned char>(m_source[at]) : 0;
}

// Move past bytes, counting a column per UTF-8 character.
void Lexer::advance(size_t count)
{
    for (size_t i = 0; i < count && m_position < m_source.size(); i++, m_position++) {
        unsigned char c = m_source[m_position];
        if (c == '\n') {
            m_line++;
            m_column = 1;
        } else if ((c & 0xC0) != 0x80) {
            m_column++;
        }
    }
}

// Return the current position.
Location Lexer::here() const
{
    Location location;
    location.m_line = m_line;
    location.m_column = m_column;
    return location;
}

// Add a token and move past its bytes.
void Lexer::push(TokenKind kind, const Location &location, size_t count)
{
    Token token;
    token.m_kind = kind;
    token.m_location = location;
    token.m_text = std::string(m_source.substr(m_position, count));
    m_tokens.push_back(token);
    advance(count);
}

// Decode one UTF-8 character.
bool Lexer::decode(unsigned &codePoint, size_t &length) const
{
    unsigned char c = peek();
    if (c < 0x80) {
        codePoint = c;
        length = 1;
        return true;
    }
    if ((c & 0xE0) == 0xC0) {
        length = 2;
        codePoint = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        length = 3;
        codePoint = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        length = 4;
        codePoint = c & 0x07;
    } else {
        length = 1;
        return false;
    }
    for (size_t i = 1; i < length; i++) {
        unsigned char next = peek(i);
        if ((next & 0xC0) != 0x80) {
            length = i;
            return false;
        }
        codePoint = (codePoint << 6) | (next & 0x3F);
    }
    return true;
}

// Scan an identifier or a keyword, rejecting a non-ASCII letter glued to it.
void Lexer::scan_identifier(const Location &location)
{
    size_t count = 0;
    while (std::isalnum(peek(count)) || peek(count) == '_') {
        count++;
    }
    std::string word(m_source.substr(m_position, count));
    auto keyword = keywords.find(word);
    push(keyword == keywords.end() ? TokenKind::Identifier : keyword->second, location, count);
    if (peek() >= 0x80) {
        unsigned codePoint = 0;
        size_t length = 1;
        decode(codePoint, length);
        m_diagnostics.error(here(), "ERR_LEX_NON_ASCII_IDENT",
                            "a non-ASCII character (" + code_point_name(codePoint) +
                                ") right after '" + word + "'");
        m_diagnostics.note(here(), "identifiers are ASCII letters, digits and '_'");
        advance(length);
    }
}

// Scan a number; '..' after digits ends an integer, so 1..5 is a range.
void Lexer::scan_number(const Location &location)
{
    size_t count = 0;
    while (std::isdigit(peek(count))) {
        count++;
    }
    bool real = false;
    if (peek(count) == '.' && std::isdigit(peek(count + 1))) {
        real = true;
        count++;
        while (std::isdigit(peek(count))) {
            count++;
        }
    }
    unsigned char sign = peek(count + 1);
    if ((peek(count) == 'e' || peek(count) == 'E') &&
        (std::isdigit(sign) || ((sign == '+' || sign == '-') && std::isdigit(peek(count + 2))))) {
        real = true;
        count += 2;
        while (std::isdigit(peek(count))) {
            count++;
        }
    }
    std::string text(m_source.substr(m_position, count));
    push(real ? TokenKind::Real : TokenKind::Integer, location, count);
    Token &token = m_tokens.back();
    if (real) {
        token.m_real = std::strtod(text.c_str(), nullptr);
    } else {
        long long value = 0;
        auto [end, status] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (status != std::errc() || end != text.data() + text.size() || value > maxInteger) {
            m_diagnostics.error(location, "ERR_LEX_INT_RANGE",
                                "the integer " + text + " is larger than 2147483647");
            value = 0;
        }
        token.m_integer = value;
    }
    if (std::isalpha(peek()) || peek() == '_') {
        m_diagnostics.error(here(), "ERR_LEX_BAD_NUMBER",
                            "a letter right after the number " + text);
        while (std::isalnum(peek()) || peek() == '_') {
            advance();
        }
    }
}

// Scan 'c', or "c" as on the slides; ''' is the quote itself.
void Lexer::scan_character(const Location &location)
{
    unsigned char quote = peek();
    if (quote == '\'' && peek(1) == '\'' && peek(2) == '\'') {
        push(TokenKind::Character, location, 3);
        m_tokens.back().m_integer = '\'';
        return;
    }
    size_t count = 1;
    while (peek(count) && peek(count) != quote && peek(count) != '\n') {
        count++;
    }
    if (peek(count) != quote) {
        m_diagnostics.error(location, "ERR_LEX_UNTERMINATED",
                            quote == '\'' ? "a character literal with no closing quote"
                                          : "a string with no closing quote");
        advance(count);
        return;
    }
    std::string body(m_source.substr(m_position + 1, count - 1));
    if (body.size() != 1 || static_cast<unsigned char>(body[0]) >= 0x80) {
        if (quote == '"') {
            m_diagnostics.error(location, "ERR_LEX_STRING",
                                "strings aren't values in CSP; only a one-character string such as "
                                "\"A\" is, as a character");
        } else {
            m_diagnostics.error(location, "ERR_LEX_BAD_CHAR_LITERAL",
                                "a character literal holds one ASCII character");
        }
        advance(count + 1);
        return;
    }
    push(TokenKind::Character, location, count + 1);
    m_tokens.back().m_integer = static_cast<unsigned char>(body[0]);
}

// Scan one of the slides' symbols, or report the character, with a hint for look-alikes.
void Lexer::scan_symbol(const Location &location)
{
    unsigned codePoint = 0;
    size_t length = 1;
    if (!decode(codePoint, length)) {
        m_diagnostics.error(location, "ERR_LEX_BAD_UTF8", "a byte that isn't UTF-8");
        advance(length);
        return;
    }
    auto symbol = unicodeSymbols.find(codePoint);
    if (symbol != unicodeSymbols.end()) {
        push(symbol->second, location, length);
        return;
    }
    m_diagnostics.error(location, "ERR_LEX_BAD_CHAR",
                        "unexpected character " + std::string(m_source.substr(m_position, length)) +
                            " (" + code_point_name(codePoint) + ")");
    if ((codePoint >= 0x400 && codePoint <= 0x4FF) || codePoint < 0x250) {
        m_diagnostics.note(location, "identifiers are ASCII; a Cyrillic or accented letter looks "
                                     "like a Latin one but isn't");
    }
    advance(length);
}

// Scan an ASCII operator, preferring the longest spelling.
void Lexer::scan_punctuation(const Location &location)
{
    using Kind = TokenKind;
    unsigned char c = peek();
    unsigned char next = peek(1);
    switch (c) {
        case '[':
            next == ']' ? push(Kind::Box, location, 2) : push(Kind::LeftBracket, location, 1);
            return;
        case ']':
            push(Kind::RightBracket, location, 1);
            return;
        case '(':
            push(Kind::LeftParen, location, 1);
            return;
        case ')':
            push(Kind::RightParen, location, 1);
            return;
        case ':':
            if (next == ':') {
                push(Kind::ColonColon, location, 2);
            } else if (next == '=') {
                push(Kind::Assign, location, 2);
            } else {
                push(Kind::Colon, location, 1);
            }
            return;
        case '|':
            next == '|' ? push(Kind::Parallel, location, 2) : push(Kind::Box, location, 1);
            return;
        case '-':
            next == '>' ? push(Kind::Arrow, location, 2) : push(Kind::Minus, location, 1);
            return;
        case '!':
            push(Kind::Bang, location, 1);
            return;
        case '?':
            push(Kind::Question, location, 1);
            return;
        case ';':
            push(Kind::Semicolon, location, 1);
            return;
        case ',':
            push(Kind::Comma, location, 1);
            return;
        case '=':
            push(Kind::Equal, location, 1);
            return;
        case '<':
            if (next == '=') {
                push(Kind::LessEqual, location, 2);
            } else if (next == '>') {
                push(Kind::NotEqual, location, 2);
            } else {
                push(Kind::Less, location, 1);
            }
            return;
        case '>':
            next == '=' ? push(Kind::GreaterEqual, location, 2) : push(Kind::Greater, location, 1);
            return;
        case '+':
            push(Kind::Plus, location, 1);
            return;
        case '*':
            push(Kind::Star, location, 1);
            return;
        case '/':
            next == '=' ? push(Kind::NotEqual, location, 2) : push(Kind::Slash, location, 1);
            return;
        case '.':
            if (next == '.') {
                push(Kind::DotDot, location, 2);
                return;
            }
            break;
        default:
            break;
    }
    m_diagnostics.error(location, "ERR_LEX_BAD_CHAR",
                        std::string("unexpected character '") + static_cast<char>(c) + "'");
    advance();
}

// Skip a -- or // comment up to the end of its line, or a /* ... */ comment.
bool Lexer::skip_comment()
{
    if (peek() == '/' && peek(1) == '*') {
        Location location = here();
        advance(2);
        while (peek() && !(peek() == '*' && peek(1) == '/')) {
            advance();
        }
        if (!peek()) {
            m_diagnostics.error(location, "ERR_LEX_UNTERMINATED", "a /* comment with no */");
            return true;
        }
        advance(2);
        return true;
    }
    bool dashes = peek() == '-' && peek(1) == '-';
    bool slashes = peek() == '/' && peek(1) == '/';
    if (!dashes && !slashes) {
        return false;
    }
    while (peek() && peek() != '\n') {
        advance();
    }
    return true;
}
