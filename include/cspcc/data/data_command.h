// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DATA_COMMAND_H
#define CSPCC_DATA_COMMAND_H

#include "cspcc/data/data_command_kind.h"
#include "cspcc/data/data_expression.h"
#include "cspcc/data/data_guard_element.h"
#include "cspcc/data/data_guarded_command.h"
#include "cspcc/data/data_location.h"
#include "cspcc/data/data_process_name.h"
#include "cspcc/data/data_type_specifier.h"

#include <memory>
#include <string>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Namespace for cspcc data declarations.
    namespace data
    {
        struct DataEntry;
        struct DataSymbol;

        // Describes a command and, after sema, its resolved message.
        struct DataCommand
        {
            // Kind of the command.
            DataCommandKind m_kind = DataCommandKind::Skip;

            // Position of the command.
            DataLocation m_location;

            // Targets of an assignment, one or a tuple.
            std::vector<std::unique_ptr<DataExpression>> m_targets;

            // Values of an assignment, one or a tuple.
            std::vector<std::unique_ptr<DataExpression>> m_values;

            // Names a declaration declares.
            std::vector<std::string> m_names;

            // Position of each declared name.
            std::vector<DataLocation> m_name_locations;

            // Type a declaration gives its names.
            DataTypeSpecifier m_type;

            // Symbols a declaration creates.
            std::vector<DataSymbol *> m_symbols;

            // Process an input reads from or an output writes to.
            DataProcessName m_process;

            // Message of an input or an output as written.
            std::unique_ptr<DataExpression> m_message;

            // Name of a macro or a hook.
            std::string m_name;

            // Arguments of a hook.
            std::vector<std::unique_ptr<DataExpression>> m_args;

            // Whether a macro or a hook was written with parentheses.
            bool m_has_parens = false;

            // Commands of a block.
            std::vector<std::unique_ptr<DataCommand>> m_body;

            // Guarded commands of an alternative or a repetitive command.
            std::vector<std::unique_ptr<DataGuardedCommand>> m_guards;

            // Constructor of the message, empty when it has none.
            std::string m_constructor;

            // Whether the message is a plain value or tuple, with no constructor.
            bool m_anonymous = true;

            // Values or targets the message carries.
            std::vector<DataExpression *> m_parts;

            // Entry of the receiving process the message travels through.
            DataEntry *m_entry = nullptr;

            // Whether a macro command calls a hook.
            bool m_is_hook = false;
        };

    }
}

#endif
