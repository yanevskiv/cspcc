// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_SEMA_CHECKER_H
#define CSPCC_SEMA_CHECKER_H

#include "cspcc/data/data_ast.h"
#include "cspcc/data/data_command.h"
#include "cspcc/data/data_constant_value.h"
#include "cspcc/data/data_define.h"
#include "cspcc/data/data_definition.h"
#include "cspcc/data/data_entry.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_family.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_process_name.h"
#include "cspcc/data/data_program.h"
#include "cspcc/data/data_symbol.h"
#include "cspcc/data/data_type_base.h"
#include "cspcc/data/data_variant.h"
#include "cspcc/diagnostics_engine.h"

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Resolves names and types, expands macros and lays out the processes of a program.
    class SemaChecker
    {
    public:
        // Prepare to check one program.
        explicit SemaChecker(DiagnosticsEngine &diagnostics);

        // Check a parsed program, expanding its macros in place.
        std::unique_ptr<data::DataProgram> check(data::DataAst &ast,
                                                 const std::vector<data::DataDefine> &defines);

        // Name a type for messages.
        static std::string_view type_name(data::DataTypeBase base);

        // Describe a message by its constructor and the types of its parts.
        static std::string message_text(std::string_view constructor, bool anonymous,
                                        const std::vector<data::DataTypeBase> &types);

    private:
        // Resolve the constants, the -D values overriding or adding to them.
        void check_constants(data::DataAst &ast, const std::vector<data::DataDefine> &defines);

        // Parse the value of one -D.
        data::DataExpression *parse_define(const data::DataDefine &define);

        // Tell macros from processes: a top-level definition used as a command is a macro.
        std::vector<data::DataDefinition *> classify_definitions(data::DataAst &ast);

        // Collect the names used as commands.
        static void
        collect_macro_uses(const std::vector<std::unique_ptr<data::DataCommand>> &commands,
                           std::set<std::string> &names);

        // Group the processes into families and number every process.
        void build_families(const std::vector<data::DataDefinition *> &definitions);

        // List the processes one definition stands for.
        void enumerate_instances(data::DataFamily &family, data::DataVariant &variant,
                                 std::set<std::vector<long long>> &taken);

        // Resolve the body of one definition.
        void resolve_variant(data::DataFamily &family, data::DataVariant &variant);

        // Resolve a command list, expanding the macros it uses.
        std::vector<std::unique_ptr<data::DataCommand>>
        resolve_commands(std::vector<std::unique_ptr<data::DataCommand>> commands);

        // Resolve one command that isn't a macro.
        void resolve_command(data::DataCommand &command);

        // Declare the names of a declaration.
        void resolve_declaration(data::DataCommand &command);

        // Resolve an assignment of one value or of a tuple.
        void resolve_assignment(data::DataCommand &command);

        // Resolve an input or an output and its message.
        void resolve_communication(data::DataCommand &command);

        // Resolve one guarded command.
        void resolve_guarded(data::DataGuardedCommand &guard);

        // Resolve a process name to its family.
        void resolve_process(data::DataProcessName &process);

        // Resolve an expression.
        data::DataTypeBase resolve_expression(data::DataExpression &expression);

        // Resolve the target of an input or an assignment.
        data::DataTypeBase resolve_target(data::DataExpression &expression);

        // Resolve the bounds of a range to constants.
        bool resolve_bounds(data::DataRange &range);

        // Compute an integer constant expression.
        std::optional<long long> fold(const data::DataExpression &expression) const;

        // Compute an integer constant expression, reporting when it isn't one.
        std::optional<long long> constant(const data::DataExpression &expression,
                                          std::string_view what);

        // Find or add the entry a family receives a message through.
        data::DataEntry *add_entry(data::DataFamily &family, const data::DataCommand &command);

        // Find, for each output, the entry of the receiving family it goes through.
        void match_outputs();

        // Find a name in the scopes, innermost first.
        data::DataSymbol *lookup(const std::string &name) const;

        // Create a symbol in the innermost scope.
        data::DataSymbol *declare(const std::string &name, data::DataSymbolKind kind,
                                  data::DataTypeBase base, const data::DataLocation &location);

        // Report an unknown process name, with a hint for a near miss.
        void report_unknown_process(const data::DataProcessName &process);

        // Check whether a value of one type can be stored in a variable of another.
        static bool assignable(data::DataTypeBase to, data::DataTypeBase from);

        // Destination of semantic errors and warnings.
        DiagnosticsEngine &m_diagnostics;

        // Program being built.
        std::unique_ptr<data::DataProgram> m_program;

        // Scopes from the constants' outward to the innermost.
        std::vector<std::map<std::string, data::DataSymbol *>> m_scopes;

        // Macros by name.
        std::map<std::string, data::DataDefinition *> m_macros;

        // Families by name.
        std::map<std::string, data::DataFamily *> m_families;

        // Family whose body is being resolved.
        data::DataFamily *m_family = nullptr;

        // Variant whose body is being resolved.
        data::DataVariant *m_variant = nullptr;

        // Macros being expanded, innermost last, to catch recursion.
        std::vector<std::string> m_expanding;

        // Outputs and their sending families, matched once every input is known.
        std::vector<std::pair<data::DataCommand *, data::DataFamily *>> m_outputs;
    };

}

#endif
