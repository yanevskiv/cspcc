// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "test.h"

// Parse a source, keeping the messages.
static std::unique_ptr<Ast> parse(const std::string &source, std::string *messages = nullptr)
{
    std::ostringstream stream;
    Diagnostics diagnostics("test.csp", source, stream);
    std::vector<Token> tokens = Lexer(source, diagnostics).tokenize();
    std::unique_ptr<Ast> ast = Parser(tokens, diagnostics).parse_program();
    if (messages) {
        *messages = stream.str();
    }
    return ast;
}

TEST_CASE("The slides' semaphore guard", "[parser]")
{
    std::unique_ptr<Ast> ast = parse("S:: *[ s > 0, (i:1..100) X(i)?wait() -> s := s - 1 ]");
    REQUIRE(ast);
    const Command &loop = *ast->m_definitions[0]->m_body[0];
    REQUIRE(loop.m_kind == CommandKind::Repetitive);
    const GuardedCommand &guard = *loop.m_guards[0];
    REQUIRE(guard.m_ranges.size() == 1);
    CHECK(guard.m_ranges[0]->m_variable == "i");
    REQUIRE(guard.m_elements.size() == 1);
    CHECK(guard.m_elements[0].m_condition);
    REQUIRE(guard.m_input);
    CHECK(guard.m_input->m_kind == CommandKind::Input);
    CHECK(guard.m_input->m_process.m_name == "X");
    CHECK(guard.m_body[0]->m_kind == CommandKind::Assign);
}

TEST_CASE("A parallel command's elements are processes, other definitions aren't", "[parser]")
{
    std::unique_ptr<Ast> ast = parse("[ A::PRODUCER || B(i:1..3)::skip ]\nPRODUCER:: *[ B(1)!0 ]");
    REQUIRE(ast);
    REQUIRE(ast->m_definitions.size() == 3);
    CHECK(ast->m_definitions[0]->m_in_parallel);
    CHECK(ast->m_definitions[1]->m_subscripts[0].m_range->m_variable == "i");
    CHECK_FALSE(ast->m_definitions[2]->m_in_parallel);
}

TEST_CASE("*[ commands ] and tuple assignments", "[parser]")
{
    std::unique_ptr<Ast> ast = parse("X:: *[ (x, y) := (y, x) ]");
    REQUIRE(ast);
    const Command &loop = *ast->m_definitions[0]->m_body[0];
    REQUIRE(loop.m_kind == CommandKind::Repetitive);
    REQUIRE(loop.m_guards.size() == 1);
    CHECK(loop.m_guards[0]->m_elements.empty());
    CHECK(loop.m_guards[0]->m_body[0]->m_targets.size() == 2);
}

TEST_CASE("The notes' constants and labels", "[parser]")
{
    std::unique_ptr<Ast> ast =
        parse("N :: 3\n[ (i:1..N) Reader(i) :: skip || Producer(1..N) :: skip ]");
    REQUIRE(ast);
    REQUIRE(ast->m_constants.size() == 1);
    CHECK(ast->m_constants[0]->m_name == "N");
    REQUIRE(ast->m_definitions.size() == 2);
    CHECK(ast->m_definitions[0]->m_name == "Reader");
    CHECK(ast->m_definitions[0]->m_subscripts[0].m_range->m_variable == "i");
    CHECK(ast->m_definitions[1]->m_subscripts[0].m_range);
}

TEST_CASE("The slides' typos", "[parser]")
{
    std::string messages;
    parse("X:: [ a := 1\n b := 2 ]", &messages);
    CHECK(contains(messages, "ERR_PARSE_EXPECTED_SEMICOLON"));
    parse("X:: *[ Bag!get(); Bag?x -> skip ]", &messages);
    CHECK(contains(messages, "ERR_PARSE_OUTPUT_GUARD"));
    parse("X:: v : = 4", &messages);
    CHECK(contains(messages, "ERR_PARSE_COLON_EQUALS"));
}

TEST_CASE("Comparisons don't chain", "[parser]")
{
    const char *source = R"csp(
X:: [ b: boolean; b := 1 < 2 < 3 ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_CHAINED_COMPARISON]"));
}

TEST_CASE("The slides' ROOM writes v : = 4", "[parser]")
{
    const char *source = R"csp(
ROOM:: [ v: integer; v : = 4 ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_COLON_EQUALS]"));
}

TEST_CASE("The input ends a guard", "[parser]")
{
    const char *source = R"csp(
X:: *[ n: integer; Y?n; n > 0 -> skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_INPUT_NOT_LAST]"));
}

TEST_CASE("Commands are separated by semicolons, as the slides' CENTER forgets once", "[parser]")
{
    const char *source = R"csp(
X:: [ x: integer; x := 1
      x := 2 ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_EXPECTED_SEMICOLON]"));
}

TEST_CASE("A parallel command only stands at the top level", "[parser]")
{
    const char *source = R"csp(
X:: [ a::skip || b::skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_NESTED_PARALLEL]"));
}

TEST_CASE("CSP's guards end with an input, never an output, as the slides' NODE has", "[parser]")
{
    const char *source = R"csp(
NODE:: *[ Bag!getTask(); Bag?getData(left, right) -> skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_OUTPUT_GUARD]"));
}

TEST_CASE("A range before a label names a subscript of the label", "[parser]")
{
    const char *source = R"csp(
[ (i:1..3) Worker(j) :: skip || Boss :: skip ]
)csp";
    Translation result = translate(source);
    CHECK_FALSE(result.m_ok);
    CHECK(contains(result.m_messages, "[ERR_PARSE_UNUSED_RANGE]"));
}
