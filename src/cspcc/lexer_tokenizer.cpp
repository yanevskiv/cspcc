/*
 * C++ file for cspcc's lexer.
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

// Project header for the lexer.
#include "cspcc/lexer_tokenizer.h"

// Standard headers for scanning text and numbers.
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

// Namespace for cspcc's lexer.
namespace cspcc
{

    // Namespace for the lexer's spelling tables.
    namespace
    {

        // Largest integer literal, Ada's Integer'Last.
        constexpr long long lexerIntMax = 2147483647LL;

        // Keywords, all lower case.
        const std::map<std::string, data::DataTokenKind, std::less<>> lexerKeywords = {
            {"skip", data::DataTokenKind::Skip},   {"true", data::DataTokenKind::True},
            {"false", data::DataTokenKind::False}, {"mod", data::DataTokenKind::Mod},
            {"div", data::DataTokenKind::Div},     {"and", data::DataTokenKind::And},
            {"or", data::DataTokenKind::Or},       {"not", data::DataTokenKind::Not},
            {"const", data::DataTokenKind::Const},
        };

        // Tokens spelled with one non-ASCII character, as on the slides.
        const std::map<unsigned, data::DataTokenKind> lexerSymbols = {
            {0x2192, data::DataTokenKind::Arrow},     {0x25A1, data::DataTokenKind::Box},
            {0x2016, data::DataTokenKind::Parallel},  {0x2260, data::DataTokenKind::NotEqual},
            {0x2264, data::DataTokenKind::LessEqual}, {0x2265, data::DataTokenKind::GreaterEqual},
            {0x00AC, data::DataTokenKind::Not},       {0x2227, data::DataTokenKind::And},
            {0x2228, data::DataTokenKind::Or},
        };

        // Format a code point as U+XXXX.
        std::string code_point_name(unsigned codePoint)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof buffer, "U+%04X", codePoint);
            return buffer;
        }

    }

    // Prepare to split one source.
    LexerTokenizer::LexerTokenizer(std::string_view source, DiagnosticsEngine &diagnostics)
        : m_source(source)
        , m_diagnostics(diagnostics)
    {
        // Empty.
    }

    // Split the whole source, skipping blanks and comments.
    std::vector<data::DataToken> LexerTokenizer::tokenize()
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
            data::DataLocation location = here();
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
        data::DataToken eof;
        eof.m_location = here();
        m_tokens.push_back(eof);
        return std::move(m_tokens);
    }

    // Name a kind of token.
    std::string_view LexerTokenizer::kind_name(data::DataTokenKind kind)
    {
        switch (kind) {
            case data::DataTokenKind::Eof:
                return "end of file";
            case data::DataTokenKind::Identifier:
                return "identifier";
            case data::DataTokenKind::Integer:
                return "integer";
            case data::DataTokenKind::Real:
                return "real number";
            case data::DataTokenKind::Character:
                return "character";
            case data::DataTokenKind::LeftBracket:
                return "'['";
            case data::DataTokenKind::RightBracket:
                return "']'";
            case data::DataTokenKind::LeftParen:
                return "'('";
            case data::DataTokenKind::RightParen:
                return "')'";
            case data::DataTokenKind::ColonColon:
                return "'::'";
            case data::DataTokenKind::Colon:
                return "':'";
            case data::DataTokenKind::Assign:
                return "':='";
            case data::DataTokenKind::Parallel:
                return "'‖' or '||'";
            case data::DataTokenKind::Box:
                return "'□' or '[]'";
            case data::DataTokenKind::Arrow:
                return "'→' or '->'";
            case data::DataTokenKind::Bang:
                return "'!'";
            case data::DataTokenKind::Question:
                return "'?'";
            case data::DataTokenKind::Semicolon:
                return "';'";
            case data::DataTokenKind::Comma:
                return "','";
            case data::DataTokenKind::DotDot:
                return "'..'";
            case data::DataTokenKind::Equal:
                return "'='";
            case data::DataTokenKind::NotEqual:
                return "'<>'";
            case data::DataTokenKind::Less:
                return "'<'";
            case data::DataTokenKind::LessEqual:
                return "'<='";
            case data::DataTokenKind::Greater:
                return "'>'";
            case data::DataTokenKind::GreaterEqual:
                return "'>='";
            case data::DataTokenKind::Plus:
                return "'+'";
            case data::DataTokenKind::Minus:
                return "'-'";
            case data::DataTokenKind::Star:
                return "'*'";
            case data::DataTokenKind::Slash:
                return "'/'";
            case data::DataTokenKind::Skip:
                return "'skip'";
            case data::DataTokenKind::True:
                return "'true'";
            case data::DataTokenKind::False:
                return "'false'";
            case data::DataTokenKind::Mod:
                return "'mod'";
            case data::DataTokenKind::Div:
                return "'div'";
            case data::DataTokenKind::And:
                return "'and'";
            case data::DataTokenKind::Or:
                return "'or'";
            case data::DataTokenKind::Not:
                return "'not'";
            case data::DataTokenKind::Const:
                return "'const'";
        }
        return "token";
    }

    // Print each token's position, kind and value.
    void LexerTokenizer::dump(const std::vector<data::DataToken> &tokens, std::ostream &stream)
    {
        for (const data::DataToken &token : tokens) {
            stream << token.m_location.m_line << ':' << token.m_location.m_column << ' '
                   << kind_name(token.m_kind);
            if (token.m_kind == data::DataTokenKind::Identifier) {
                stream << ' ' << token.m_text;
            } else if (token.m_kind == data::DataTokenKind::Integer ||
                       token.m_kind == data::DataTokenKind::Character) {
                stream << ' ' << token.m_integer;
            } else if (token.m_kind == data::DataTokenKind::Real) {
                stream << ' ' << token.m_real;
            }
            stream << '\n';
        }
    }

    // Look at a byte ahead.
    unsigned char LexerTokenizer::peek(size_t ahead) const
    {
        size_t at = m_position + ahead;
        return at < m_source.size() ? static_cast<unsigned char>(m_source[at]) : 0;
    }

    // Move past bytes, counting a column per UTF-8 character.
    void LexerTokenizer::advance(size_t count)
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
    data::DataLocation LexerTokenizer::here() const
    {
        data::DataLocation location;
        location.m_line = m_line;
        location.m_column = m_column;
        return location;
    }

    // Add a token and move past its bytes.
    void LexerTokenizer::push(data::DataTokenKind kind, const data::DataLocation &location,
                              size_t count)
    {
        data::DataToken token;
        token.m_kind = kind;
        token.m_location = location;
        token.m_text = std::string(m_source.substr(m_position, count));
        m_tokens.push_back(token);
        advance(count);
    }

    // Decode one UTF-8 character.
    bool LexerTokenizer::decode(unsigned &codePoint, size_t &length) const
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
    void LexerTokenizer::scan_identifier(const data::DataLocation &location)
    {
        size_t count = 0;
        while (std::isalnum(peek(count)) || peek(count) == '_') {
            count++;
        }
        std::string word(m_source.substr(m_position, count));
        auto keyword = lexerKeywords.find(word);
        push(keyword == lexerKeywords.end() ? data::DataTokenKind::Identifier : keyword->second,
             location, count);
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
    void LexerTokenizer::scan_number(const data::DataLocation &location)
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
            (std::isdigit(sign) ||
             ((sign == '+' || sign == '-') && std::isdigit(peek(count + 2))))) {
            real = true;
            count += 2;
            while (std::isdigit(peek(count))) {
                count++;
            }
        }
        std::string text(m_source.substr(m_position, count));
        push(real ? data::DataTokenKind::Real : data::DataTokenKind::Integer, location, count);
        data::DataToken &token = m_tokens.back();
        if (real) {
            token.m_real = std::strtod(text.c_str(), nullptr);
        } else {
            long long value = 0;
            auto [end, status] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (status != std::errc() || end != text.data() + text.size() || value > lexerIntMax) {
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
    void LexerTokenizer::scan_character(const data::DataLocation &location)
    {
        unsigned char quote = peek();
        if (quote == '\'' && peek(1) == '\'' && peek(2) == '\'') {
            push(data::DataTokenKind::Character, location, 3);
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
                m_diagnostics.error(
                    location, "ERR_LEX_STRING",
                    "strings aren't values in CSP; only a one-character string such as "
                    "\"A\" is, as a character");
            } else {
                m_diagnostics.error(location, "ERR_LEX_BAD_CHAR_LITERAL",
                                    "a character literal holds one ASCII character");
            }
            advance(count + 1);
            return;
        }
        push(data::DataTokenKind::Character, location, count + 1);
        m_tokens.back().m_integer = static_cast<unsigned char>(body[0]);
    }

    // Scan one of the slides' symbols, or report the character, with a hint for look-alikes.
    void LexerTokenizer::scan_symbol(const data::DataLocation &location)
    {
        unsigned codePoint = 0;
        size_t length = 1;
        if (!decode(codePoint, length)) {
            m_diagnostics.error(location, "ERR_LEX_BAD_UTF8", "a byte that isn't UTF-8");
            advance(length);
            return;
        }
        auto symbol = lexerSymbols.find(codePoint);
        if (symbol != lexerSymbols.end()) {
            push(symbol->second, location, length);
            return;
        }
        m_diagnostics.error(location, "ERR_LEX_BAD_CHAR",
                            "unexpected character " +
                                std::string(m_source.substr(m_position, length)) + " (" +
                                code_point_name(codePoint) + ")");
        if ((codePoint >= 0x400 && codePoint <= 0x4FF) || codePoint < 0x250) {
            m_diagnostics.note(location,
                               "identifiers are ASCII; a Cyrillic or accented letter looks "
                               "like a Latin one but isn't");
        }
        advance(length);
    }

    // Scan an ASCII operator, preferring the longest spelling.
    void LexerTokenizer::scan_punctuation(const data::DataLocation &location)
    {
        using Kind = data::DataTokenKind;
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
                next == '=' ? push(Kind::GreaterEqual, location, 2)
                            : push(Kind::Greater, location, 1);
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

    // Skip a -- or // comment up to the end of its line.
    bool LexerTokenizer::skip_comment()
    {
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

}
