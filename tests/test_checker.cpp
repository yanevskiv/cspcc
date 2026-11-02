// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "test.h"

// A checked program with the tree it points into, and the messages.
struct Checked
{
    std::unique_ptr<Ast> m_ast;
    std::unique_ptr<Program> m_program;
    std::string m_messages;
};

// Lex, parse and check a source.
static Checked check(const std::string &source, const std::vector<Define> &defines = {})
{
    Checked result;
    std::ostringstream messages;
    Diagnostics diagnostics("test.csp", source, messages);
    std::vector<Token> tokens = Lexer(source, diagnostics).tokenize();
    result.m_ast = Parser(tokens, diagnostics).parse_program();
    if (result.m_ast) {
        result.m_program = Checker(diagnostics).check(*result.m_ast, defines);
    }
    result.m_messages = messages.str();
    return result;
}

TEST_CASE("Label ranges number the processes family by family", "[checker]")
{
    Checked result =
        check("const N = 3\n[ P(i:1..N)::Q!go() || Q::*[ (i:1..N) P(i)?go() -> skip ] ]");
    REQUIRE(result.m_program);
    REQUIRE(result.m_program->m_families.size() == 2);
    CHECK(result.m_program->m_total == 4);
    CHECK(result.m_program->m_families[0]->m_first == 1);
    CHECK(result.m_program->m_families[0]->m_last == 3);
    CHECK(result.m_program->m_families[1]->m_first == 4);
    CHECK(result.m_program->m_families[1]->m_entries.size() == 1);
}

TEST_CASE("-D overrides a constant", "[checker]")
{
    Checked result = check("const N = 3\n[ P(i:1..N)::skip ]", {{"N", "7"}});
    REQUIRE(result.m_program);
    CHECK(result.m_program->m_total == 7);
    Checked notes = check("N :: 3\n[ (i:1..N) P(i) :: skip ]", {{"N", "5"}});
    REQUIRE(notes.m_program);
    CHECK(notes.m_program->m_total == 5);
}

TEST_CASE("A definition used as a command is a macro, an undefined one a hook", "[checker]")
{
    Checked result = check("X:: [ n: integer; SET; eat() ]\nSET:: [ n := 1 ]");
    REQUIRE(result.m_program);
    REQUIRE(result.m_program->m_families.size() == 1);
    const auto &body = result.m_program->m_families[0]->m_variants[0]->m_definition->m_body;
    REQUIRE(body.size() == 1);
    REQUIRE(body[0]->m_body.size() == 3);
    CHECK(body[0]->m_body[1]->m_kind == CommandKind::Assign);
    CHECK(body[0]->m_body[2]->m_is_hook);
}

TEST_CASE("Unknown processes get a hint, unmatched outputs a warning", "[checker]")
{
    Checked unknown = check("[ Fork(i:0..1)::skip || P::fork(0)!in() ]");
    CHECK(contains(unknown.m_messages, "ERR_SEMA_UNKNOWN_PROCESS"));
    CHECK(contains(unknown.m_messages, "did you mean 'Fork'?"));
    Checked unmatched = check("[ A::B!has(1) || B::[ c: character; A?has(c) ] ]");
    CHECK(contains(unmatched.m_messages, "WARN_SEMA_NO_MATCHING_INPUT"));
}

TEST_CASE("Types in any case, as the notes write them", "[checker]")
{
    Checked result = check("X :: [ a : Integer; b : REAL; c : Boolean; d : character ]");
    REQUIRE(result.m_program);
    CHECK_FALSE(contains(result.m_messages, "WARN_SEMA_UNKNOWN_TYPE"));
}

TEST_CASE("The definitions of a family have the same number of subscripts", "[checker]")
{
    const char *source = R"csp(
[ M(i:1..2):: skip || M(0, 0):: skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_ARITY]"));
}

TEST_CASE("Each process is defined once", "[checker]")
{
    const char *source = R"csp(
[ S(i:0..3):: skip || S(i:3..5):: skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_DUPLICATE_PROCESS]"));
}

TEST_CASE("A macro can't use itself", "[checker]")
{
    const char *source = R"csp(
X:: LOOP
LOOP:: [ skip; LOOP ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_MACRO_RECURSIVE]"));
}

TEST_CASE("A program of macros alone has no processes", "[checker]")
{
    const char *source = R"csp(
const N = 1
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_NO_PROCESSES]"));
}

TEST_CASE("A process's index is a constant", "[checker]")
{
    const char *source = R"csp(
[ X(i:1..2):: i := 3 ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_NOT_ASSIGNABLE]"));
}

TEST_CASE("A range's bounds are constants, so its processes and alternatives are known",
          "[checker]")
{
    const char *source = R"csp(
[ fac(i:1..LIMIT):: skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_NOT_CONSTANT]"));
}

TEST_CASE("A signal is written with parentheses: wait(), not wait", "[checker]")
{
    const char *source = R"csp(
[ A:: B!go() || B:: A?go ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_UNDECLARED]"));
}

TEST_CASE("Types must match", "[checker]")
{
    const char *source = R"csp(
X:: [ n: integer; n := 'a' ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_TYPE]"));
}

TEST_CASE("A name is declared before it is used", "[checker]")
{
    const char *source = R"csp(
X:: [ n := 1 ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_UNDECLARED]"));
}

TEST_CASE("The slides' FORK names Pfil instead of Phil", "[checker]")
{
    const char *source = R"csp(
[ Phil(i:0..1):: Fork(i)!in() || Fork(i:0..1):: Pfil(i)?in() ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_SEMA_UNKNOWN_PROCESS]"));
}
