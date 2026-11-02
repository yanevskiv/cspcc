// Tests of the Ada writer.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "test.h"

TEST_CASE("Ada names avoid reserved words and clashes of case", "[emitter]")
{
    NameTable names;
    CHECK(names.make("in") == "in_2");
    CHECK(names.make("N") == "N");
    CHECK(names.make("n") == "n_2");
    CHECK(NameTable::sanitize("_a__b_") == "a_b");
}

TEST_CASE("Literals are spelled as Ada spells them", "[emitter]")
{
    CHECK(ExpressionWriter::real_literal(3) == "3.0");
    CHECK(ExpressionWriter::real_literal(0.5) == "0.5");
    CHECK(ExpressionWriter::real_literal(1e20) == "1.0E+20");
    CHECK(ExpressionWriter::number(-2) == "(-2)");
}

TEST_CASE("Each message a family receives is an entry family", "[emitter]")
{
    Translation result =
        translate("[ P(i:1..2)::S!wait() || S::*[ (i:1..2) P(i)?wait() -> skip ] ]");
    REQUIRE(result.m_ok);
    CHECK(contains(result.m_ada, "task type S_Task (Id : Pid) is"));
    CHECK(contains(result.m_ada, "entry C_wait (Pid);"));
    CHECK(contains(result.m_ada, "S_T (Peer).C_wait (Id);"));
    CHECK(contains(result.m_ada, "accept C_wait (Src_1 (2)) do"));
}

TEST_CASE("Unit names come from file names", "[emitter]")
{
    CHECK(AdaWriter::unit_from("Dining-Philosophers") == "dining_philosophers");
    CHECK(AdaWriter::unit_from("01_copy") == "csp_01_copy");
    CHECK(AdaWriter::unit_from("select") == "select_main");
    CHECK_FALSE(AdaWriter::valid_unit("csp_runtime"));
}
