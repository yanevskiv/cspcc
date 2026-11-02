// Checks a syntax tree and builds the program to translate.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CSPCC_CHECKER_H
#define CSPCC_CHECKER_H

#include "ast.h"
#include "diagnostics.h"
#include "program.h"
#include "token.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Resolves names and types, expands macros and lays out the processes of a program.
class Checker
{
public:
    // Prepare to check one program.
    explicit Checker(Diagnostics &diagnostics);

    // Check a parsed program, expanding its macros in place.
    std::unique_ptr<Program> check(Ast &ast, const std::vector<Define> &defines);

    // Name a type for messages.
    static std::string_view type_name(BaseType base);

    // Describe a message by its constructor and the types of its parts.
    static std::string message_text(std::string_view constructor, bool anonymous,
                                    const std::vector<BaseType> &types);

private:
    // Resolve the constants, the -D values overriding or adding to them.
    void check_constants(Ast &ast, const std::vector<Define> &defines);

    // Parse the value of one -D.
    Expression *parse_define(const Define &define);

    // Tell macros from processes: a top-level definition used as a command is a macro.
    std::vector<Definition *> classify_definitions(Ast &ast);

    // Collect the names used as commands.
    static void collect_macro_uses(const std::vector<std::unique_ptr<Command>> &commands,
                                   std::set<std::string> &names);

    // Group the processes into families and number every process.
    void build_families(const std::vector<Definition *> &definitions);

    // List the processes one definition stands for.
    void enumerate_instances(Family &family, Variant &variant,
                             std::set<std::vector<long long>> &taken);

    // Resolve the body of one definition.
    void resolve_variant(Family &family, Variant &variant);

    // Resolve a command list, expanding the macros it uses.
    std::vector<std::unique_ptr<Command>>
    resolve_commands(std::vector<std::unique_ptr<Command>> commands);

    // Resolve one command that isn't a macro.
    void resolve_command(Command &command);

    // Declare the names of a declaration.
    void resolve_declaration(Command &command);

    // Resolve an assignment of one value or of a tuple.
    void resolve_assignment(Command &command);

    // Resolve an input or an output and its message.
    void resolve_communication(Command &command);

    // Resolve one guarded command.
    void resolve_guarded(GuardedCommand &guard);

    // Resolve a process name to its family.
    void resolve_process(ProcessName &process);

    // Resolve an expression.
    BaseType resolve_expression(Expression &expression);

    // Resolve the target of an input or an assignment.
    BaseType resolve_target(Expression &expression);

    // Resolve the bounds of a range to constants.
    bool resolve_bounds(Range &range);

    // Compute an integer constant expression.
    std::optional<long long> fold(const Expression &expression) const;

    // Compute an integer constant expression, reporting when it isn't one.
    std::optional<long long> constant(const Expression &expression, std::string_view what);

    // Find or add the entry a family receives a message through.
    Entry *add_entry(Family &family, const Command &command);

    // Find, for each output, the entry of the receiving family it goes through.
    void match_outputs();

    // Find a name in the scopes, innermost first.
    Symbol *lookup(const std::string &name) const;

    // Create a symbol in the innermost scope.
    Symbol *declare(const std::string &name, SymbolKind kind, BaseType base,
                    const Location &location);

    // Report an unknown process name, with a hint for a near miss.
    void report_unknown_process(const ProcessName &process);

    // Check whether a value of one type can be stored in a variable of another.
    static bool assignable(BaseType to, BaseType from);

    // Destination of semantic errors and warnings.
    Diagnostics &m_diagnostics;

    // Program being built.
    std::unique_ptr<Program> m_program;

    // Scopes from the constants' outward to the innermost.
    std::vector<std::map<std::string, Symbol *>> m_scopes;

    // Macros by name.
    std::map<std::string, Definition *> m_macros;

    // Families by name.
    std::map<std::string, Family *> m_families;

    // Family whose body is being resolved.
    Family *m_family = nullptr;

    // Variant whose body is being resolved.
    Variant *m_variant = nullptr;

    // Macros being expanded, innermost last, to catch recursion.
    std::vector<std::string> m_expanding;

    // Outputs and their sending families, matched once every input is known.
    std::vector<std::pair<Command *, Family *>> m_outputs;
};

#endif
