/*
 * C++ file for parser unit tests.
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

// Catch2 declarations for parser checks.
#include <catch2/catch_test_macros.hpp>

// Project parser under test, with the lexer and printer it needs.
#include "cspcc/ast_printer.h"
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"

// Standard headers for capturing output.
#include <sstream>
#include <string>

// Namespace for the parser tests' helpers.
namespace
{

    // Parse a source and return its printed tree, or its diagnostics after an error.
    std::string parse_text(const std::string &source)
    {
        std::ostringstream messages;
        cspcc::DiagnosticsEngine diagnostics("test.csp", source, messages);
        cspcc::LexerTokenizer tokenizer(source, diagnostics);
        std::vector<cspcc::data::DataToken> tokens = tokenizer.tokenize();
        cspcc::ParserGrammar parser(tokens, diagnostics);
        std::unique_ptr<cspcc::data::DataAst> ast = parser.parse_program();
        if (!ast) {
            return messages.str();
        }
        std::ostringstream tree;
        cspcc::AstPrinter(tree).print(*ast);
        return tree.str();
    }

}

// Verify that a range prefix may stand in the middle of a guard, as on the slides.
TEST_CASE("Parser reads the slides' semaphore guard", "[unit][parser_grammar]")
{
    std::string tree = parse_text("S:: *[ s > 0, (i:1..100) X(i)?wait() -> s := s - 1 ]");
    CHECK(tree.find("guard (i:1..100)") != std::string::npos);
    CHECK(tree.find("when (s > 0)") != std::string::npos);
    CHECK(tree.find("input X(i) ? wait()") != std::string::npos);
}

// Verify that a parallel command's elements become processes and other definitions don't.
TEST_CASE("Parser tells processes from top-level definitions", "[unit][parser_grammar]")
{
    std::string tree = parse_text("[ A::PRODUCER || B(i:1..3)::skip ]\nPRODUCER:: *[ B(1)!0 ]");
    CHECK(tree.find("process A\n") != std::string::npos);
    CHECK(tree.find("process B(i:1..3)\n") != std::string::npos);
    CHECK(tree.find("define PRODUCER\n") != std::string::npos);
}

// Verify that an unguarded repetitive command and a tuple assignment parse.
TEST_CASE("Parser reads *[ commands ] and tuple assignments", "[unit][parser_grammar]")
{
    std::string tree = parse_text("X:: *[ (x, y) := (y, x) ]");
    CHECK(tree.find("rep\n    guard\n    do\n      assign x, y := y, x") != std::string::npos);
}

// Verify the errors the slides' typos produce.
TEST_CASE("Parser reports missing semicolons and output guards", "[unit][parser_grammar]")
{
    CHECK(parse_text("X:: [ a := 1\n b := 2 ]").find("ERR_PARSE_EXPECTED_SEMICOLON") !=
          std::string::npos);
    CHECK(parse_text("X:: *[ Bag!get(); Bag?x -> skip ]").find("ERR_PARSE_OUTPUT_GUARD") !=
          std::string::npos);
    CHECK(parse_text("X:: v : = 4").find("ERR_PARSE_COLON_EQUALS") != std::string::npos);
    CHECK(parse_text("X:: [ a::skip || b::skip ]").find("ERR_PARSE_NESTED_PARALLEL") !=
          std::string::npos);
}
