/*
 * C++ header for cspcc's lexer.
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

#ifndef CSPCC_LEXER_TOKENIZER_H
#define CSPCC_LEXER_TOKENIZER_H

#include "cspcc/data/data_location.h"
#include "cspcc/data/data_token.h"
#include "cspcc/data/data_token_kind.h"
#include "cspcc/diagnostics_engine.h"

#include <cstddef>
#include <ostream>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Splits CSP source into tokens, with the slides' symbols and their ASCII spellings. */
    class LexerTokenizer
    {
    public:
        /**
         * Prepare to split one source.
         *
         * @param source Text to split, which must outlive the tokenizer.
         * @param diagnostics Destination of lexical errors.
         */
        LexerTokenizer(std::string_view source, DiagnosticsEngine &diagnostics);

        /**
         * Split the whole source.
         *
         * @return Tokens, the last one of kind Eof.
         */
        std::vector<data::DataToken> tokenize();

        /**
         * Name a kind of token for diagnostics.
         *
         * @param kind Kind to name.
         * @return Name such as identifier or '::'.
         */
        static std::string_view kind_name(data::DataTokenKind kind);

        /**
         * Print tokens one per line, for --dump-tokens.
         *
         * @param tokens Tokens to print.
         * @param stream Destination of the listing.
         */
        static void dump(const std::vector<data::DataToken> &tokens, std::ostream &stream);

    private:
        /**
         * Look at a byte ahead of the current position.
         *
         * @param ahead Distance from the current position.
         * @return The byte, or 0 past the end.
         */
        unsigned char peek(size_t ahead = 0) const;

        /**
         * Move past bytes, counting lines and characters.
         *
         * @param count Number of bytes.
         */
        void advance(size_t count = 1);

        /**
         * Return the current position.
         *
         * @return Line and column of the next character.
         */
        data::DataLocation here() const;

        /**
         * Add a token spelled by the bytes at the current position and move past them.
         *
         * @param kind Kind of the token.
         * @param location Position of the token.
         * @param count Number of bytes it spans.
         */
        void push(data::DataTokenKind kind, const data::DataLocation &location, size_t count);

        /**
         * Decode the UTF-8 character at the current position.
         *
         * @param codePoint Destination of the character's code point.
         * @param length Destination of the character's length in bytes.
         * @return Whether the bytes are well-formed UTF-8.
         */
        bool decode(unsigned &codePoint, size_t &length) const;

        /**
         * Scan an identifier or a keyword.
         *
         * @param location Position of its first character.
         */
        void scan_identifier(const data::DataLocation &location);

        /**
         * Scan an integer or a real number.
         *
         * @param location Position of its first digit.
         */
        void scan_number(const data::DataLocation &location);

        /**
         * Scan a character literal or a one-character string.
         *
         * @param location Position of its opening quote.
         */
        void scan_character(const data::DataLocation &location);

        /**
         * Scan a non-ASCII character: one of CSP's symbols, or an error.
         *
         * @param location Position of the character.
         */
        void scan_symbol(const data::DataLocation &location);

        /**
         * Scan an ASCII operator or punctuation mark.
         *
         * @param location Position of its first character.
         */
        void scan_punctuation(const data::DataLocation &location);

        /**
         * Skip a comment, if one starts at the current position.
         *
         * @return Whether a comment was skipped.
         */
        bool skip_comment();

        /** Text being split. */
        std::string_view m_source;

        /** Destination of lexical errors. */
        DiagnosticsEngine &m_diagnostics;

        /** Offset of the next byte. */
        size_t m_position = 0;

        /** Line of the next character. */
        int m_line = 1;

        /** Column of the next character. */
        int m_column = 1;

        /** Tokens scanned so far. */
        std::vector<data::DataToken> m_tokens;
    };

}

#endif
