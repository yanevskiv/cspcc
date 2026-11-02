/*
 * C++ file for Ada emitter unit tests.
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

// Catch2 declarations for emitter checks.
#include <catch2/catch_test_macros.hpp>

// Project emitter under test, with the stages before it.
#include "cspcc/emit_ada_writer.h"
#include "cspcc/emit_expression_writer.h"
#include "cspcc/emit_name_table.h"
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"
#include "cspcc/sema_checker.h"

// Standard headers for capturing output.
#include <sstream>
#include <string>

// Namespace for the emitter tests' helpers.
namespace
{

    // Translate a source to Ada, or return the diagnostics.
    std::string translate(const std::string &source)
    {
        std::ostringstream messages;
        cspcc::DiagnosticsEngine diagnostics("test.csp", source, messages);
        cspcc::LexerTokenizer tokenizer(source, diagnostics);
        std::vector<cspcc::data::DataToken> tokens = tokenizer.tokenize();
        std::unique_ptr<cspcc::data::DataAst> ast =
            cspcc::ParserGrammar(tokens, diagnostics).parse_program();
        if (!ast) {
            return messages.str();
        }
        std::unique_ptr<cspcc::data::DataProgram> program =
            cspcc::SemaChecker(diagnostics).check(*ast, {});
        if (!program) {
            return messages.str();
        }
        return cspcc::EmitAdaWriter(*program, "Test", "test.csp").write();
    }

}

// Verify that CSP names that are Ada words or differ only in case get distinct Ada names.
TEST_CASE("Name table avoids reserved words and case clashes", "[unit][emit_name_table]")
{
    cspcc::EmitNameTable names;
    CHECK(names.make("in") == "in_2");
    CHECK(names.make("N") == "N");
    CHECK(names.make("n") == "n_2");
    CHECK(cspcc::EmitNameTable::sanitize("_a__b_") == "a_b");
}

// Verify Ada's spelling of reals and negative numbers.
TEST_CASE("Expression writer spells literals as Ada does", "[unit][emit_expression_writer]")
{
    CHECK(cspcc::EmitExpressionWriter::real_literal(3) == "3.0");
    CHECK(cspcc::EmitExpressionWriter::real_literal(0.5) == "0.5");
    CHECK(cspcc::EmitExpressionWriter::real_literal(1e20) == "1.0E+20");
    CHECK(cspcc::EmitExpressionWriter::number(-2) == "(-2)");
}

// Verify that a family gets a task type with an entry family per message kind.
TEST_CASE("Ada writer gives each received message an entry", "[unit][emit_ada_writer]")
{
    std::string ada = translate("[ P(i:1..2)::S!wait() || S::*[ (i:1..2) P(i)?wait() -> skip ] ]");
    CHECK(ada.find("task type S_Task (Id : Pid) is") != std::string::npos);
    CHECK(ada.find("entry C_wait (Pid);") != std::string::npos);
    CHECK(ada.find("S_T (Peer).C_wait (Id);") != std::string::npos);
    CHECK(ada.find("accept C_wait (Src_1 (2)) do") != std::string::npos);
}

// Verify the unit names made from file names.
TEST_CASE("Ada writer makes unit names from file names", "[unit][emit_ada_writer]")
{
    CHECK(cspcc::EmitAdaWriter::unit_from("Dining-Philosophers") == "dining_philosophers");
    CHECK(cspcc::EmitAdaWriter::unit_from("01_copy") == "csp_01_copy");
    CHECK(cspcc::EmitAdaWriter::unit_from("select") == "select_main");
    CHECK_FALSE(cspcc::EmitAdaWriter::valid_unit("csp_runtime"));
}
