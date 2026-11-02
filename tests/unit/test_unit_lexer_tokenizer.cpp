/*
 * C++ file for lexer unit tests.
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

// Catch2 declarations for lexer checks.
#include <catch2/catch_test_macros.hpp>

// Project lexer under test.
#include "cspcc/lexer_tokenizer.h"

// Standard headers for capturing diagnostics.
#include <sstream>
#include <string>
#include <vector>

// Namespace for the lexer tests' helpers.
namespace
{

    // Split a source and return the kinds of its tokens.
    std::vector<cspcc::data::DataTokenKind> kinds_of(const std::string &source,
                                                     std::string *messages = nullptr)
    {
        std::ostringstream stream;
        cspcc::DiagnosticsEngine diagnostics("test.csp", source, stream);
        cspcc::LexerTokenizer tokenizer(source, diagnostics);
        std::vector<cspcc::data::DataTokenKind> kinds;
        for (const cspcc::data::DataToken &token : tokenizer.tokenize()) {
            kinds.push_back(token.m_kind);
        }
        if (messages) {
            *messages = stream.str();
        }
        return kinds;
    }

}

// Verify that the slides' symbols and their ASCII spellings give the same tokens.
TEST_CASE("Lexer reads both spellings of CSP's symbols", "[unit][lexer_tokenizer]")
{
    using Kind = cspcc::data::DataTokenKind;
    std::vector<Kind> expected = {Kind::Arrow,    Kind::Box,       Kind::Parallel,
                                  Kind::NotEqual, Kind::LessEqual, Kind::GreaterEqual,
                                  Kind::Eof};
    CHECK(kinds_of("→ □ ‖ ≠ ≤ ≥") == expected);
    CHECK(kinds_of("-> [] || <> <= >=") == expected);
    CHECK(kinds_of("| /=")[0] == Kind::Box);
    CHECK(kinds_of("| /=")[1] == Kind::NotEqual);
}

// Verify that a range's dots aren't read as a real number's point.
TEST_CASE("Lexer splits 1..5 into a range", "[unit][lexer_tokenizer]")
{
    using Kind = cspcc::data::DataTokenKind;
    CHECK(kinds_of("1..5") ==
          std::vector<Kind>{Kind::Integer, Kind::DotDot, Kind::Integer, Kind::Eof});
    CHECK(kinds_of("1.5")[0] == Kind::Real);
}

// Verify keywords, comments, labels and the slides' one-character strings.
TEST_CASE("Lexer reads keywords, comments and characters", "[unit][lexer_tokenizer]")
{
    using Kind = cspcc::data::DataTokenKind;
    CHECK(kinds_of("X::*[c: character; west?c] -- comment\nskip") ==
          std::vector<Kind>{Kind::Identifier, Kind::ColonColon, Kind::Star, Kind::LeftBracket,
                            Kind::Identifier, Kind::Colon, Kind::Identifier, Kind::Semicolon,
                            Kind::Identifier, Kind::Question, Kind::Identifier, Kind::RightBracket,
                            Kind::Skip, Kind::Eof});
    CHECK(kinds_of("'*' \"A\" ''''") ==
          std::vector<Kind>{Kind::Character, Kind::Character, Kind::Character, Kind::Eof});
}

// Verify that a Cyrillic look-alike is an error with a hint.
TEST_CASE("Lexer rejects a Cyrillic letter", "[unit][lexer_tokenizer]")
{
    std::string messages;
    kinds_of("console(ј-1)", &messages);
    CHECK(messages.find("ERR_LEX_BAD_CHAR") != std::string::npos);
    CHECK(messages.find("Cyrillic") != std::string::npos);
}
