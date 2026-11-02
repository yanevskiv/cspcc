/*
 * C++ header for cspcc's semantic checks.
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

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Resolves names and types, expands macros and lays out the processes of a program. */
    class SemaChecker
    {
    public:
        /**
         * Prepare to check one program.
         *
         * @param diagnostics Destination of semantic errors and warnings.
         */
        explicit SemaChecker(DiagnosticsEngine &diagnostics);

        /**
         * Check a parsed program, expanding its macros in place.
         *
         * @param ast Program to check, which must outlive the result.
         * @param defines Constants given on the command line.
         * @return The checked program, or null after errors.
         */
        std::unique_ptr<data::DataProgram> check(data::DataAst &ast,
                                                 const std::vector<data::DataDefine> &defines);

        /**
         * Name a type for messages.
         *
         * @param base Type to name.
         * @return Its CSP name, such as integer.
         */
        static std::string_view type_name(data::DataTypeBase base);

        /**
         * Describe a message by its constructor and the types of its parts.
         *
         * @param constructor Constructor, empty when the message has none.
         * @param anonymous Whether the message has no constructor.
         * @param types Types of the parts.
         * @return Text such as has(integer), or (integer, real).
         */
        static std::string message_text(std::string_view constructor, bool anonymous,
                                        const std::vector<data::DataTypeBase> &types);

    private:
        /**
         * Resolve the constants, the -D values overriding or adding to them.
         *
         * @param ast Program whose constants to resolve.
         * @param defines Constants given on the command line.
         */
        void check_constants(data::DataAst &ast, const std::vector<data::DataDefine> &defines);

        /**
         * Parse the value of one -D.
         *
         * @param define Name and text of the value.
         * @return The value, or null after an error.
         */
        data::DataExpression *parse_define(const data::DataDefine &define);

        /**
         * Tell macros from processes: a top-level definition used as a command is a macro.
         *
         * @param ast Program whose definitions to sort.
         * @return The definitions that are processes.
         */
        std::vector<data::DataDefinition *> classify_definitions(data::DataAst &ast);

        /**
         * Collect the names used as commands.
         *
         * @param commands Commands to search, nested ones included.
         * @param names Destination of the names.
         */
        static void
        collect_macro_uses(const std::vector<std::unique_ptr<data::DataCommand>> &commands,
                           std::set<std::string> &names);

        /**
         * Group the processes into families and number every process.
         *
         * @param definitions Definitions that are processes.
         */
        void build_families(const std::vector<data::DataDefinition *> &definitions);

        /**
         * List the processes one definition stands for.
         *
         * @param family Family the definition belongs to.
         * @param variant Variant made from the definition.
         * @param taken Subscripts already defined in the family.
         */
        void enumerate_instances(data::DataFamily &family, data::DataVariant &variant,
                                 std::set<std::vector<long long>> &taken);

        /**
         * Resolve the body of one definition.
         *
         * @param family Family the definition belongs to.
         * @param variant Variant made from the definition.
         */
        void resolve_variant(data::DataFamily &family, data::DataVariant &variant);

        /**
         * Resolve a command list, expanding the macros it uses.
         *
         * @param commands Commands to resolve.
         * @return The commands with each macro replaced by its body.
         */
        std::vector<std::unique_ptr<data::DataCommand>>
        resolve_commands(std::vector<std::unique_ptr<data::DataCommand>> commands);

        /**
         * Resolve one command that isn't a macro.
         *
         * @param command Command to resolve.
         */
        void resolve_command(data::DataCommand &command);

        /**
         * Declare the names of a declaration.
         *
         * @param command Declaration to resolve.
         */
        void resolve_declaration(data::DataCommand &command);

        /**
         * Resolve an assignment of one value or of a tuple.
         *
         * @param command Assignment to resolve.
         */
        void resolve_assignment(data::DataCommand &command);

        /**
         * Resolve an input or an output and its message.
         *
         * @param command Input or output to resolve.
         */
        void resolve_communication(data::DataCommand &command);

        /**
         * Resolve one guarded command.
         *
         * @param guard Guarded command to resolve.
         */
        void resolve_guarded(data::DataGuardedCommand &guard);

        /**
         * Resolve a process name to its family.
         *
         * @param process Process name to resolve.
         */
        void resolve_process(data::DataProcessName &process);

        /**
         * Resolve an expression.
         *
         * @param expression Expression to resolve.
         * @return Its type, None after an error.
         */
        data::DataTypeBase resolve_expression(data::DataExpression &expression);

        /**
         * Resolve the target of an input or an assignment.
         *
         * @param expression Target to resolve.
         * @return Its type, None after an error.
         */
        data::DataTypeBase resolve_target(data::DataExpression &expression);

        /**
         * Resolve the bounds of a range to constants.
         *
         * @param range Range to resolve.
         * @return Whether both bounds are constant.
         */
        bool resolve_bounds(data::DataRange &range);

        /**
         * Compute an integer constant expression.
         *
         * @param expression Expression to compute.
         * @return Its value, or nothing when it isn't constant.
         */
        std::optional<long long> fold(const data::DataExpression &expression) const;

        /**
         * Compute an integer constant expression, reporting when it isn't one.
         *
         * @param expression Expression to compute.
         * @param what What the value is, for the message.
         * @return Its value, or nothing after an error.
         */
        std::optional<long long> constant(const data::DataExpression &expression,
                                          std::string_view what);

        /**
         * Find or add the entry a family receives a message through.
         *
         * @param family Receiving family.
         * @param command Input whose message is resolved.
         * @return The entry.
         */
        data::DataEntry *add_entry(data::DataFamily &family, const data::DataCommand &command);

        /**
         * Find, for each output, the entry of the receiving family it goes through.
         */
        void match_outputs();

        /**
         * Find a name in the scopes, innermost first.
         *
         * @param name Name to find.
         * @return Its symbol, or null.
         */
        data::DataSymbol *lookup(const std::string &name) const;

        /**
         * Create a symbol in the innermost scope.
         *
         * @param name Name of the symbol.
         * @param kind What the name stands for.
         * @param base Its type.
         * @param location Position of its declaration.
         * @return The symbol.
         */
        data::DataSymbol *declare(const std::string &name, data::DataSymbolKind kind,
                                  data::DataTypeBase base, const data::DataLocation &location);

        /**
         * Report an unknown process name, with a hint for a near miss.
         *
         * @param process Process name that matches no family.
         */
        void report_unknown_process(const data::DataProcessName &process);

        /**
         * Check whether a value of one type can be stored in a variable of another.
         *
         * @param to Type of the variable.
         * @param from Type of the value.
         * @return Whether the types match, an integer going into a real.
         */
        static bool assignable(data::DataTypeBase to, data::DataTypeBase from);

        /** Destination of semantic errors and warnings. */
        DiagnosticsEngine &m_diagnostics;

        /** Program being built. */
        std::unique_ptr<data::DataProgram> m_program;

        /** Scopes from the constants' outward to the innermost. */
        std::vector<std::map<std::string, data::DataSymbol *>> m_scopes;

        /** Macros by name. */
        std::map<std::string, data::DataDefinition *> m_macros;

        /** Families by name. */
        std::map<std::string, data::DataFamily *> m_families;

        /** Family whose body is being resolved. */
        data::DataFamily *m_family = nullptr;

        /** Variant whose body is being resolved. */
        data::DataVariant *m_variant = nullptr;

        /** Macros being expanded, innermost last, to catch recursion. */
        std::vector<std::string> m_expanding;

        /** Outputs and their sending families, matched once every input is known. */
        std::vector<std::pair<data::DataCommand *, data::DataFamily *>> m_outputs;
    };

}

#endif
