// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

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

// Namespace for cspcc declarations.
namespace cspcc
{
    // Writes the Ada block that runs one variant of a process family.
    class EmitProcessWriter
    {
    public:
        // Prepare to write one variant.
        EmitProcessWriter(const data::DataFamily &family, const data::DataVariant &variant,
                          EmitNameTable &names, std::string source);

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
        void emit_commands(const std::vector<std::unique_ptr<data::DataCommand>> &commands);

        // Write one command.
        void emit_command(const data::DataCommand &command);

        // Write an assignment, a tuple through temporaries.
        void emit_assignment(const data::DataCommand &command);

        // Write an input command outside a guard.
        void emit_input(const data::DataCommand &command);

        // Write an output command.
        void emit_output(const data::DataCommand &command);

        // Write a call of a hook.
        void emit_hook(const data::DataCommand &command);

        // Write an alternative or repetitive command.
        void emit_alternative(const data::DataCommand &command);

        // Write an alternative or repetitive command of one guard with no input or range.
        void emit_plain_alternative(const data::DataCommand &command);

        // Write the accept statement of an input.
        void emit_accept(const data::DataCommand &input, std::string_view source);

        // Write the conditions of a guard joined by and then.
        std::string condition(const data::DataGuardedCommand &guard);

        // Render the message of an input or output for traces.
        std::string message_image(const data::DataCommand &command,
                                  const std::vector<std::string> &values);

        // Render the number of the process a command names.
        std::string process_id(const data::DataProcessName &process);

        // Render a location for the inside of an Ada string literal.
        std::string location(const data::DataLocation &location) const;

        // Family the variant belongs to.
        const data::DataFamily &m_family;

        // Variant being written.
        const data::DataVariant &m_variant;

        // Names taken in the task body, the block's included.
        EmitNameTable &m_names;

        // Name of the CSP file.
        std::string m_source;

        // Declarations of the block's locals.
        std::vector<std::string> m_declarations;

        // Statements of the block.
        std::string m_body;

        // Indentation of the next statement, in Ada levels.
        int m_indent = 0;

        // Renderer of expressions, with bound variables replaced inside unrolled alternatives.
        EmitExpressionWriter m_expressions;

        // Name of the Peer local, empty until it is needed.
        std::string m_peer;

        // Number of alternative commands written, for naming their locals.
        int m_alternatives = 0;
    };

}

#endif
