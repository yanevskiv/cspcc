// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMITTER_H
#define CSPCC_EMITTER_H

#include "ast.h"
#include "program.h"
#include "token.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Hands out Ada names that clash neither with Ada's reserved words nor with each other.
class NameTable
{
public:
    // Start with Ada's reserved words taken.
    NameTable();

    // Take a name as it is.
    void reserve(std::string_view name);

    // Check whether a name is taken, ignoring case as Ada does.
    bool taken(std::string_view name) const;

    // Take a name made from a CSP name, adding _2, _3 and so on until it is free.
    std::string make(std::string_view base);

    // Turn a CSP name into a valid Ada identifier.
    static std::string sanitize(std::string_view name);

private:
    // Names taken, in lower case.
    std::set<std::string> m_taken;
};

// Renders checked expressions as Ada, fully parenthesized.
class ExpressionWriter
{
public:
    // Render an expression.
    std::string render(const Expression &expression, BaseType want = BaseType::None) const;

    // Render a symbol as fixed text from now on, such as a bound variable as a constant.
    void substitute(const Symbol *symbol, std::string text);

    // Render a symbol by its Ada name again.
    void forget(const Symbol *symbol);

    // Name the Ada type of a CSP type.
    static std::string_view ada_type(BaseType base);

    // Give the initial value of a variable of a type.
    static std::string_view default_value(BaseType base);

    // Render a real number as an Ada literal.
    static std::string real_literal(double value);

    // Spell an integer, parenthesizing a negative one.
    static std::string number(long long value);

private:
    // Texts standing for symbols, such as bound variables in unrolled alternatives.
    std::map<const Symbol *, std::string> m_substitutions;
};

// Writes the Ada block that runs one variant of a process family.
class ProcessWriter
{
public:
    // Prepare to write one variant.
    ProcessWriter(const Family &family, const Variant &variant, NameTable &names,
                  std::string source);

    // Write the variant as a declare block.
    std::string write(int indent);

private:
    // Append a line to the body at the current indentation.
    void line(std::string_view text);

    // Declare a local of the block.
    std::string local(std::string_view base, std::string_view declaration);

    // Return the Peer local, declaring it on first use.
    const std::string &peer();

    // Write a command list, or null when it writes nothing.
    void emit_commands(const std::vector<std::unique_ptr<Command>> &commands);

    // Write one command.
    void emit_command(const Command &command);

    // Write an assignment, a tuple through temporaries.
    void emit_assignment(const Command &command);

    // Write an input command outside a guard.
    void emit_input(const Command &command);

    // Write an output command.
    void emit_output(const Command &command);

    // Write a call of a hook.
    void emit_hook(const Command &command);

    // Write an alternative or repetitive command.
    void emit_alternative(const Command &command);

    // Write an alternative or repetitive command of one guard with no input or range.
    void emit_plain_alternative(const Command &command);

    // Write the accept statement of an input.
    void emit_accept(const Command &input, std::string_view source);

    // Write the conditions of a guard joined by and then.
    std::string condition(const GuardedCommand &guard);

    // Render the message of an input or output for traces.
    std::string message_image(const Command &command, const std::vector<std::string> &values);

    // Render the number of the process a command names.
    std::string process_id(const ProcessName &process);

    // Render a location for the inside of an Ada string literal.
    std::string location(const Location &location) const;

    // Family the variant belongs to.
    const Family &m_family;

    // Variant being written.
    const Variant &m_variant;

    // Names taken in the task body, the block's included.
    NameTable &m_names;

    // Name of the CSP file.
    std::string m_source;

    // Declarations of the block's locals.
    std::vector<std::string> m_declarations;

    // Statements of the block.
    std::string m_body;

    // Indentation of the next statement, in Ada levels.
    int m_indent = 0;

    // Renderer of expressions, with bound variables replaced inside unrolled alternatives.
    ExpressionWriter m_expressions;

    // Name of the Peer local, empty until it is needed.
    std::string m_peer;

    // Number of alternative commands written, for naming their locals.
    int m_alternatives = 0;
};

// Writes a checked program as one Ada procedure with a task type per process family.
class AdaWriter
{
public:
    // Prepare to write a program.
    AdaWriter(Program &program, std::string unit, std::string source);

    // Write the whole procedure.
    std::string write();

    // Check whether a name can be the Ada main procedure's file name.
    static bool valid_unit(std::string_view name);

    // Make a unit name from any file name.
    static std::string unit_from(std::string_view name);

private:
    // Give Ada names to the constants, the families and their entries.
    void name_everything();

    // Append a line at the current indentation.
    void line(std::string_view text);

    // Write the constants.
    void write_constants();

    // Write a family's task type, its table of processes and its lookups.
    void write_family_spec(const Family &family);

    // Write a family's task body.
    void write_family_body(const Family &family);

    // Write the aggregate mapping subscripts to process numbers.
    std::string map_aggregate(const Family &family,
                              const std::map<std::vector<long long>, int> &pids,
                              std::vector<long long> &prefix) const;

    // Write the main procedure's statements.
    void write_main();

    // Program being written.
    Program &m_program;

    // Name of the Ada main procedure.
    std::string m_unit;

    // Name of the CSP file.
    std::string m_source;

    // Names taken at the level of the main procedure.
    NameTable m_names;

    // Text written so far.
    std::string m_text;

    // Indentation of the next line, in Ada levels.
    int m_indent = 0;
};

#endif
