// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "test.h"

// Split a source and return the kinds of its tokens.
static std::vector<TokenKind> kinds_of(const std::string &source, std::string *messages = nullptr)
{
    std::ostringstream stream;
    Diagnostics diagnostics("test.csp", source, stream);
    std::vector<TokenKind> kinds;
    for (const Token &token : Lexer(source, diagnostics).tokenize()) {
        kinds.push_back(token.m_kind);
    }
    if (messages) {
        *messages = stream.str();
    }
    return kinds;
}

TEST_CASE("The slides' symbols and their ASCII spellings give the same tokens", "[lexer]")
{
    using Kind = TokenKind;
    std::vector<Kind> expected = {Kind::Arrow,    Kind::Box,       Kind::Parallel,
                                  Kind::NotEqual, Kind::LessEqual, Kind::GreaterEqual,
                                  Kind::Eof};
    CHECK(kinds_of("→ □ ‖ ≠ ≤ ≥") == expected);
    CHECK(kinds_of("-> [] || <> <= >=") == expected);
    CHECK(kinds_of("| /=") == std::vector<Kind>{Kind::Box, Kind::NotEqual, Kind::Eof});
}

TEST_CASE("1..5 is a range, not a real", "[lexer]")
{
    using Kind = TokenKind;
    CHECK(kinds_of("1..5") ==
          std::vector<Kind>{Kind::Integer, Kind::DotDot, Kind::Integer, Kind::Eof});
    CHECK(kinds_of("1.5")[0] == Kind::Real);
}

TEST_CASE("Keywords, comments and characters", "[lexer]")
{
    using Kind = TokenKind;
    CHECK(kinds_of("X::*[c: character; west?c] -- comment\nskip") ==
          std::vector<Kind>{Kind::Identifier, Kind::ColonColon, Kind::Star, Kind::LeftBracket,
                            Kind::Identifier, Kind::Colon, Kind::Identifier, Kind::Semicolon,
                            Kind::Identifier, Kind::Question, Kind::Identifier, Kind::RightBracket,
                            Kind::Skip, Kind::Eof});
    CHECK(kinds_of("/* a\n comment */ skip // and another") ==
          std::vector<Kind>{Kind::Skip, Kind::Eof});
    CHECK(kinds_of("'*' \"A\" ''''") ==
          std::vector<Kind>{Kind::Character, Kind::Character, Kind::Character, Kind::Eof});
}

TEST_CASE("A Cyrillic look-alike is an error with a hint", "[lexer]")
{
    std::string messages;
    kinds_of("console(ј-1)", &messages);
    CHECK(contains(messages, "ERR_LEX_BAD_CHAR"));
    CHECK(contains(messages, "Cyrillic"));
}

TEST_CASE("A Cyrillic letter that looks Latin, as in the slides' console(ј-1)", "[lexer]")
{
    const char *source = R"csp(
X:: console(ј - 1)!'A'
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_LEX_BAD_CHAR]"));
}

TEST_CASE("Only a one-character string is a value", "[lexer]")
{
    const char *source = R"csp(
X:: Y!"AB"
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_LEX_STRING]"));
}

TEST_CASE("A /* comment ends with */", "[lexer]")
{
    const char *source = R"csp(
X :: skip /* never closed
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_LEX_UNTERMINATED]"));
}
