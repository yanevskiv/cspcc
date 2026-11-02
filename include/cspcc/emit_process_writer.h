/*
 * C++ header for the Ada body of one translated CSP process definition.
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

#ifndef CSPCC_EMIT_PROCESS_WRITER_H
#define CSPCC_EMIT_PROCESS_WRITER_H

#include "cspcc/data/data_command.h"
#include "cspcc/data/data_entry.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_family.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_process_name.h"
#include "cspcc/data/data_symbol.h"
#include "cspcc/data/data_type_base.h"
#include "cspcc/data/data_variant.h"
#include "cspcc/emit_expression_writer.h"
#include "cspcc/emit_name_table.h"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Writes the Ada block that runs one variant of a process family. */
    class EmitProcessWriter
    {
    public:
        /**
         * Prepare to write one variant.
         *
         * @param family Family the variant belongs to, its Ada names already given.
         * @param variant Variant to write.
         * @param names Names taken in the task body so far, which the writer adds to, since
         *              loop names must be distinct across a whole body.
         * @param source Name of the CSP file, for locations in messages.
         */
        EmitProcessWriter(const data::DataFamily &family, const data::DataVariant &variant,
                          EmitNameTable &names, std::string source);

        /**
         * Write the variant as a declare block.
         *
         * @param indent Indentation of the block, in Ada levels of three spaces.
         * @return The block's text.
         */
        std::string write(int indent);

    private:
        /**
         * Append a line to the body at the current indentation.
         *
         * @param text Line to append.
         */
        void line(std::string_view text);

        /**
         * Declare a local of the block.
         *
         * @param base Name to start from.
         * @param declaration Text after the name and its colon.
         * @return The name taken.
         */
        std::string local(std::string_view base, std::string_view declaration);

        /**
         * Return the Peer local, declaring it on first use.
         *
         * @return Its name.
         */
        const std::string &peer();

        /**
         * Write a command list, or null when it writes nothing.
         *
         * @param commands Commands to write.
         */
        void emit_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands);

        /**
         * Write one command.
         *
         * @param command Command to write.
         */
        void emit_command(const data::DataCommand &command);

        /**
         * Write an assignment, a tuple through temporaries.
         *
         * @param command Assignment to write.
         */
        void emit_assignment(const data::DataCommand &command);

        /**
         * Write an input command outside a guard.
         *
         * @param command Input to write.
         */
        void emit_input(const data::DataCommand &command);

        /**
         * Write an output command.
         *
         * @param command Output to write.
         */
        void emit_output(const data::DataCommand &command);

        /**
         * Write a call of a hook.
         *
         * @param command Hook to write.
         */
        void emit_hook(const data::DataCommand &command);

        /**
         * Write an alternative or repetitive command.
         *
         * @param command Command to write.
         */
        void emit_alternative(const data::DataCommand &command);

        /**
         * Write an alternative or repetitive command of one guard with no input or range.
         *
         * @param command Command to write.
         */
        void emit_plain_alternative(const data::DataCommand &command);

        /**
         * Write the accept statement of an input.
         *
         * @param input Input to accept.
         * @param source Ada expression naming the sending process.
         */
        void emit_accept(const data::DataCommand &input, std::string_view source);

        /**
         * Write the conditions of a guard joined by and then.
         *
         * @param guard Guard whose conditions to join.
         * @return The condition, or empty when there is none.
         */
        std::string condition(const data::DataGuardedCommand &guard);

        /**
         * Render the message of an input or output for traces.
         *
         * @param command Input or output.
         * @param values Ada expressions of the message's parts.
         * @return An Ada String expression.
         */
        std::string message_image(const data::DataCommand &command,
                                  const std::vector<std::string> &values);

        /**
         * Render the number of the process a command names.
         *
         * @param process Process name.
         * @return A call of the family's _Id function.
         */
        std::string process_id(const data::DataProcessName &process);

        /**
         * Render a location for the inside of an Ada string literal.
         *
         * @param location Position in the CSP file.
         * @return Text such as copy.csp:3:10, its quotes doubled.
         */
        std::string location(const data::DataLocation &location) const;

        /** Family the variant belongs to. */
        const data::DataFamily &m_family;

        /** Variant being written. */
        const data::DataVariant &m_variant;

        /** Names taken in the task body, the block's included. */
        EmitNameTable &m_names;

        /** Name of the CSP file. */
        std::string m_source;

        /** Declarations of the block's locals. */
        std::vector<std::string> m_declarations;

        /** Statements of the block. */
        std::string m_body;

        /** Indentation of the next statement, in Ada levels. */
        int m_indent = 0;

        /** Renderer of expressions, with bound variables replaced inside unrolled alternatives. */
        EmitExpressionWriter m_expressions;

        /** Name of the Peer local, empty until it is needed. */
        std::string m_peer;

        /** Number of alternative commands written, for naming their locals. */
        int m_alternatives = 0;
    };

}

#endif
