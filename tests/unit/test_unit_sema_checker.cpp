/*
 * C++ file for semantic check unit tests.
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

// Catch2 declarations for semantic checks.
#include <catch2/catch_test_macros.hpp>

// Project checker under test, with the lexer and parser it needs.
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"
#include "cspcc/sema_checker.h"

// Standard headers for capturing diagnostics.
#include <sstream>
#include <string>

// Namespace for the semantic tests' helpers.
namespace
{

    // Hold a checked program with the tree it points into, and the diagnostics.
    struct Checked
    {
        std::unique_ptr<cspcc::data::DataAst> m_ast;
        std::unique_ptr<cspcc::data::DataProgram> m_program;
        std::string m_messages;
    };

    // Lex, parse and check a source.
    Checked check(const std::string &source,
                  const std::vector<cspcc::data::DataDefine> &defines = {})
    {
        Checked result;
        std::ostringstream messages;
        cspcc::DiagnosticsEngine diagnostics("test.csp", source, messages);
        cspcc::LexerTokenizer tokenizer(source, diagnostics);
        std::vector<cspcc::data::DataToken> tokens = tokenizer.tokenize();
        result.m_ast = cspcc::ParserGrammar(tokens, diagnostics).parse_program();
        if (result.m_ast) {
            result.m_program = cspcc::SemaChecker(diagnostics).check(*result.m_ast, defines);
        }
        result.m_messages = messages.str();
        return result;
    }

}

// Verify that label ranges number the processes family by family.
TEST_CASE("Sema numbers the processes of each family", "[unit][sema_checker]")
{
    Checked result = check("const N = 3\n"
                           "[ P(i:1..N)::Q!go() || Q::*[ (i:1..N) P(i)?go() -> skip ] ]");
    REQUIRE(result.m_program);
    REQUIRE(result.m_program->m_families.size() == 2);
    CHECK(result.m_program->m_total == 4);
    CHECK(result.m_program->m_families[0]->m_first == 1);
    CHECK(result.m_program->m_families[0]->m_last == 3);
    CHECK(result.m_program->m_families[1]->m_first == 4);
    CHECK(result.m_program->m_families[1]->m_entries.size() == 1);
}

// Verify that a -D value replaces a const's value.
TEST_CASE("Sema lets -D override a constant", "[unit][sema_checker]")
{
    Checked result = check("const N = 3\n[ P(i:1..N)::skip ]", {{"N", "7"}});
    REQUIRE(result.m_program);
    CHECK(result.m_program->m_total == 7);
}

// Verify that a top-level definition used as a command is a macro and the rest are processes.
TEST_CASE("Sema expands macros and keeps hooks", "[unit][sema_checker]")
{
    Checked result = check("X:: [ n: integer; SET; eat() ]\nSET:: [ n := 1 ]");
    REQUIRE(result.m_program);
    REQUIRE(result.m_program->m_families.size() == 1);
    const auto &body = result.m_program->m_families[0]->m_variants[0]->m_definition->m_body;
    REQUIRE(body.size() == 1);
    REQUIRE(body[0]->m_body.size() == 3);
    CHECK(body[0]->m_body[1]->m_kind == cspcc::data::DataCommandKind::Assign);
    CHECK(body[0]->m_body[2]->m_is_hook);
}

// Verify the hints for the slides' typos and mismatched messages.
TEST_CASE("Sema reports unknown processes and unmatched outputs", "[unit][sema_checker]")
{
    Checked unknown = check("[ Fork(i:0..1)::skip || P::fork(0)!in() ]");
    CHECK(unknown.m_messages.find("ERR_SEMA_UNKNOWN_PROCESS") != std::string::npos);
    CHECK(unknown.m_messages.find("did you mean 'Fork'?") != std::string::npos);
    Checked unmatched = check("[ A::B!has(1) || B::[ c: character; A?has(c) ] ]");
    CHECK(unmatched.m_messages.find("ERR_SEMA_NO_MATCHING_INPUT") != std::string::npos);
}
