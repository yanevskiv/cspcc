// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/emit_process_writer.h"

#include <utility>

namespace cspcc
{

    namespace
    {

        // Join texts with a separator.
        std::string join(const std::vector<std::string> &items, std::string_view separator)
        {
            std::string text;
            for (size_t i = 0; i < items.size(); i++) {
                text += (i ? std::string(separator) : "") + items[i];
            }
            return text;
        }

        // Count the alternatives a guard's ranges unroll into.
        long long unrolled(const data::DataGuardedCommand &guard)
        {
            long long count = 1;
            for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
                count *= range->m_last - range->m_first + 1;
            }
            return count;
        }

    }

    // Keep the task body's names and the source's name.
    EmitProcessWriter::EmitProcessWriter(const data::DataFamily &family,
                                         const data::DataVariant &variant, EmitNameTable &names,
                                         std::string source)
        : m_family(family)
        , m_variant(variant)
        , m_names(names)
        , m_source(std::move(source))
    {
    }

    // Name the indices and locals, write the body, then put the declarations above it.
    std::string EmitProcessWriter::write(int indent)
    {
        for (data::DataSymbol *symbol : m_variant.m_index_symbols) {
            symbol->m_ada = m_names.make(symbol->m_name);
            m_declarations.push_back(symbol->m_ada + " : constant Integer := " + m_family.m_ada +
                                     "_Idx (Id, " + std::to_string(symbol->m_index_position) +
                                     ");");
        }
        for (data::DataSymbol *symbol : m_variant.m_locals) {
            symbol->m_ada = m_names.make(symbol->m_name);
            std::string type(EmitExpressionWriter::ada_type(symbol->m_base));
            std::string value(EmitExpressionWriter::default_value(symbol->m_base));
            if (!symbol->m_dimensions.empty()) {
                std::vector<std::string> bounds;
                for (const std::pair<long long, long long> &dimension : symbol->m_dimensions) {
                    bounds.push_back(std::to_string(dimension.first) + " .. " +
                                     std::to_string(dimension.second));
                    value = "(others => " + value + ")";
                }
                type = "array (" + join(bounds, ", ") + ") of " + type;
            }
            m_declarations.push_back(symbol->m_ada + " : " + type + " := " + value + ";");
        }
        m_indent = indent + 1;
        emit_commands(m_variant.m_definition->m_body);
        std::string pad(indent * 3, ' ');
        std::string text = pad + "declare\n";
        for (const std::string &declaration : m_declarations) {
            text += pad + "   " + declaration + "\n";
        }
        return text + pad + "begin\n" + m_body + pad + "end;\n";
    }

    // Append a line.
    void EmitProcessWriter::line(std::string_view text)
    {
        m_body += std::string(m_indent * 3, ' ');
        m_body += text;
        m_body += '\n';
    }

    // Declare a local.
    std::string EmitProcessWriter::local(std::string_view base, std::string_view declaration)
    {
        std::string name = m_names.make(base);
        m_declarations.push_back(name + " : " + std::string(declaration));
        return name;
    }

    // Declare Peer on first use.
    const std::string &EmitProcessWriter::peer()
    {
        if (m_peer.empty()) {
            m_peer = local("Peer", "Pid := 1;");
        }
        return m_peer;
    }

    // Write each command, then null if none wrote a statement.
    void EmitProcessWriter::emit_commands(
        const std::vector<std::unique_ptr<data::DataCommand>> &commands)
    {
        size_t before = m_body.size();
        for (const std::unique_ptr<data::DataCommand> &command : commands) {
            emit_command(*command);
        }
        if (m_body.size() == before) {
            line("null;");
        }
    }

    // Write a command by its kind; declarations are already above the block.
    void EmitProcessWriter::emit_command(const data::DataCommand &command)
    {
        switch (command.m_kind) {
            case data::DataCommandKind::Skip:
                line("null;");
                return;
            case data::DataCommandKind::Declare:
                return;
            case data::DataCommandKind::Assign:
                emit_assignment(command);
                return;
            case data::DataCommandKind::Input:
                emit_input(command);
                return;
            case data::DataCommandKind::Output:
                emit_output(command);
                return;
            case data::DataCommandKind::Macro:
                emit_hook(command);
                return;
            case data::DataCommandKind::Block:
                emit_commands(command.m_body);
                return;
            case data::DataCommandKind::Alternative:
            case data::DataCommandKind::Repetitive:
                emit_alternative(command);
                return;
        }
    }

    // Assign one value, or a tuple's values all at once through temporaries.
    void EmitProcessWriter::emit_assignment(const data::DataCommand &command)
    {
        if (command.m_targets.size() == 1) {
            const data::DataExpression &target = *command.m_targets[0];
            line(m_expressions.render(target) +
                 " := " + m_expressions.render(*command.m_values[0], target.m_type) + ";");
            return;
        }
        std::vector<std::string> temporaries;
        for (size_t i = 0; i < command.m_values.size(); i++) {
            data::DataTypeBase base = command.m_targets[i]->m_type;
            temporaries.push_back(local(
                "Tmp", std::string(EmitExpressionWriter::ada_type(base)) +
                           " := " + std::string(EmitExpressionWriter::default_value(base)) + ";"));
            line(temporaries.back() + " := " + m_expressions.render(*command.m_values[i], base) +
                 ";");
        }
        for (size_t i = 0; i < command.m_targets.size(); i++) {
            line(m_expressions.render(*command.m_targets[i]) + " := " + temporaries[i] + ";");
        }
    }

    // Wait for the input, failing if its source terminates first.
    void EmitProcessWriter::emit_input(const data::DataCommand &command)
    {
        const std::string &source = peer();
        line(source + " := " + process_id(command.m_process) + ";");
        line("R.Waiting (Id, \"input from \" & R.Name_Of (" + source + ") & \" at " +
             location(command.m_location) + "\");");
        line("loop");
        m_indent++;
        line("if R.Is_Done (" + source + ") then");
        line("   R.Fail (Id, \"input from terminated process \" & R.Name_Of (" + source + "));");
        line("end if;");
        line("select");
        m_indent++;
        emit_accept(command, source);
        line("exit;");
        m_indent--;
        line("or");
        line("   delay R.Poll;");
        line("end select;");
        m_indent--;
        line("end loop;");
        line("R.Running (Id);");
    }

    // Call the receiver's entry, failing if the receiver has terminated.
    void EmitProcessWriter::emit_output(const data::DataCommand &command)
    {
        const std::string &target = peer();
        const data::DataEntry &entry = *command.m_entry;
        std::vector<std::string> args;
        for (size_t i = 0; i < command.m_parts.size(); i++) {
            args.push_back(m_expressions.render(*command.m_parts[i], entry.m_types[i]));
        }
        std::string call = command.m_process.m_family->m_ada + "_T (" + target + ")." +
                           entry.m_ada + " (Id)" +
                           (args.empty() ? "" : " (" + join(args, ", ") + ")");
        line(target + " := " + process_id(command.m_process) + ";");
        line("R.Waiting (Id, \"output to \" & R.Name_Of (" + target + ") & \" at " +
             location(command.m_location) + "\");");
        line("begin");
        line("   " + call + ";");
        line("exception");
        line("   when Tasking_Error =>");
        line("      R.Fail (Id, \"output to terminated process \" & R.Name_Of (" + target + "));");
        line("end;");
        line("R.Running (Id);");
    }

    // Call R.Hook with the hook's name and the values of its arguments.
    void EmitProcessWriter::emit_hook(const data::DataCommand &command)
    {
        std::string text = "\"" + command.m_name;
        if (command.m_has_parens) {
            std::vector<std::string> images;
            for (const std::unique_ptr<data::DataExpression> &arg : command.m_args) {
                images.push_back("R.Img (" + m_expressions.render(*arg) + ")");
            }
            text += images.empty() ? "()\"" : "(\" & " + join(images, " & \", \" & ") + " & \")\"";
        } else {
            text += "\"";
        }
        line("R.Hook (Id, " + text + ");");
    }

    // Unroll the guards and evaluate them; choose at random among the true guards whose input
    // has a caller waiting, or else wait for the first input; then run the commands.
    void EmitProcessWriter::emit_alternative(const data::DataCommand &command)
    {
        bool repetitive = command.m_kind == data::DataCommandKind::Repetitive;
        const auto &guards = command.m_guards;
        if (guards.size() == 1 && !guards[0]->m_input && guards[0]->m_ranges.empty()) {
            emit_plain_alternative(command);
            return;
        }
        int number = ++m_alternatives;
        std::string suffix = "_" + std::to_string(number);
        std::vector<long long> bases;
        long long total = 0;
        std::string inputMask;
        for (const std::unique_ptr<data::DataGuardedCommand> &guard : guards) {
            long long count = unrolled(*guard);
            bases.push_back(total + 1);
            std::string choice =
                count == 1 ? std::to_string(total + 1)
                           : std::to_string(total + 1) + " .. " + std::to_string(total + count);
            bool input = guard->m_input != nullptr;
            inputMask +=
                (inputMask.empty() ? "" : ", ") + choice + " => " + (input ? "True" : "False");
            total += count;
        }
        std::string range = "(1 .. " + std::to_string(total) + ")";
        std::string open =
            local("Open" + suffix, "R.Bool_Array " + range + " := (others => False);");
        std::string sources = local("Src" + suffix, "R.Pid_Array " + range + " := (others => 1);");
        std::string inputs =
            local("Input" + suffix, "constant R.Bool_Array " + range + " := (" + inputMask + ");");
        std::string selected = local("Sel" + suffix, "Natural := 0;");
        std::string slot = local("Slot" + suffix, "Positive := 1;");
        std::string label = repetitive ? m_names.make("Rep" + suffix) : "";
        if (repetitive) {
            line(label + " : loop");
            m_indent++;
        }
        for (size_t g = 0; g < guards.size(); g++) {
            const data::DataGuardedCommand &guard = *guards[g];
            int loops = 0;
            std::string offset = std::to_string(bases[g]);
            long long stride = unrolled(guard);
            for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
                long long count = range->m_last - range->m_first + 1;
                stride /= count;
                std::string counter = m_names.make("L" + suffix);
                line("for " + counter + " in " + EmitExpressionWriter::number(range->m_first) +
                     " .. " + EmitExpressionWriter::number(range->m_last) + " loop");
                m_indent++;
                line(range->m_symbol->m_ada + " := " + counter + ";");
                offset += " + (" + counter + " - " + EmitExpressionWriter::number(range->m_first) +
                          ") * " + std::to_string(stride);
                loops++;
            }
            line(slot + " := " + offset + ";");
            std::string test = condition(guard);
            if (!test.empty()) {
                line("if " + test + " then");
                m_indent++;
            }
            if (guard.m_input) {
                line(sources + " (" + slot + ") := " + process_id(guard.m_input->m_process) + ";");
                line(open + " (" + slot + ") := not R.Is_Done (" + sources + " (" + slot + "));");
            } else {
                line(open + " (" + slot + ") := True;");
            }
            if (!test.empty()) {
                m_indent--;
                line("else");
                line("   " + open + " (" + slot + ") := False;");
                line("end if;");
            }
            for (; loops > 0; loops--) {
                m_indent--;
                line("end loop;");
            }
        }
        bool anyInput = false;
        for (const std::unique_ptr<data::DataGuardedCommand> &guard : guards) {
            anyInput = anyInput || guard->m_input;
        }
        auto accepts = [&](auto when) {
            bool first = true;
            for (size_t g = 0; g < guards.size(); g++) {
                const data::DataGuardedCommand &guard = *guards[g];
                if (!guard.m_input) {
                    continue;
                }
                long long count = unrolled(guard);
                for (long long k = 0; k < count; k++) {
                    long long rest = k;
                    long long stride = count;
                    for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
                        long long size = range->m_last - range->m_first + 1;
                        stride /= size;
                        m_expressions.substitute(
                            range->m_symbol,
                            "(" + std::to_string(range->m_first + rest / stride) + ")");
                        rest %= stride;
                    }
                    std::string index = std::to_string(bases[g] + k);
                    if (!first) {
                        line("or");
                    }
                    first = false;
                    line("when " + when(index) + " =>");
                    m_indent++;
                    emit_accept(*guard.m_input, sources + " (" + index + ")");
                    line(selected + " := " + index + ";");
                    m_indent--;
                }
                for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
                    m_expressions.forget(range->m_symbol);
                }
            }
        };
        if (!anyInput) {
            line(selected + " := R.Pick (" + open + ", " + open + ");");
        } else {
            std::string ready =
                local("Ready" + suffix, "R.Bool_Array " + range + " := (others => False);");
            std::string chosen = local("Chosen" + suffix, "Natural := 0;");
            line(ready + " := " + open + ";");
            for (size_t g = 0; g < guards.size(); g++) {
                if (!guards[g]->m_input) {
                    continue;
                }
                const std::string &entry = guards[g]->m_input->m_entry->m_ada;
                for (long long k = 0; k < unrolled(*guards[g]); k++) {
                    std::string index = std::to_string(bases[g] + k);
                    line(ready + " (" + index + ") := " + ready + " (" + index + ") and then " +
                         entry + " (" + sources + " (" + index + "))'Count > 0;");
                }
            }
            line(selected + " := R.Pick (" + ready + ", " + ready + ");");
            line("if " + selected + " /= 0 and then " + inputs + " (" + selected + ") then");
            m_indent++;
            line(chosen + " := " + selected + ";");
            line(selected + " := 0;");
            line("select");
            m_indent++;
            accepts([&](const std::string &index) {
                return chosen + " = " + index;
            });
            m_indent--;
            line("end select;");
            m_indent--;
            line("elsif " + selected + " = 0 and then R.Any (" + open + ", " + inputs + ") then");
            m_indent++;
            line("R.Waiting (Id, \"alternative command at " + location(command.m_location) +
                 "\");");
            line("loop");
            m_indent++;
            line("select");
            m_indent++;
            accepts([&](const std::string &index) {
                return open + " (" + index + ")";
            });
            m_indent--;
            line("or");
            line("   delay R.Poll;");
            line("   R.Close_Done (" + open + ", " + inputs + ", " + sources + ");");
            line("end select;");
            line("exit when " + selected + " /= 0 or else not R.Any (" + open + ", " + inputs +
                 ");");
            m_indent--;
            line("end loop;");
            line("R.Running (Id);");
            m_indent--;
            line("end if;");
        }
        if (repetitive) {
            line("exit " + label + " when " + selected + " = 0;");
        } else {
            line("if " + selected + " = 0 then");
            line("   R.Fail (Id, \"no guard of the alternative command at " +
                 location(command.m_location) + " is true\");");
            line("end if;");
        }
        line("case " + selected + " is");
        m_indent++;
        for (size_t g = 0; g < guards.size(); g++) {
            const data::DataGuardedCommand &guard = *guards[g];
            long long count = unrolled(guard);
            line("when " +
                 (count == 1
                      ? std::to_string(bases[g])
                      : std::to_string(bases[g]) + " .. " + std::to_string(bases[g] + count - 1)) +
                 " =>");
            m_indent++;
            long long stride = count;
            for (const std::unique_ptr<data::DataRange> &range : guard.m_ranges) {
                long long size = range->m_last - range->m_first + 1;
                stride /= size;
                line(range->m_symbol->m_ada +
                     " := " + EmitExpressionWriter::number(range->m_first) + " + ((" + selected +
                     " - " + std::to_string(bases[g]) + ") / " + std::to_string(stride) + ") mod " +
                     std::to_string(size) + ";");
            }
            emit_commands(guard.m_body);
            m_indent--;
        }
        line("when others =>");
        line("   null;");
        m_indent--;
        line("end case;");
        if (repetitive) {
            m_indent--;
            line("end loop " + label + ";");
        }
    }

    // Write [ b -> c ] as if b then c, and *[ b -> c ] as a loop that exits when b is false.
    void EmitProcessWriter::emit_plain_alternative(const data::DataCommand &command)
    {
        const data::DataGuardedCommand &guard = *command.m_guards[0];
        std::string test = condition(guard);
        if (command.m_kind == data::DataCommandKind::Repetitive) {
            std::string label = m_names.make("Rep_" + std::to_string(++m_alternatives));
            line(label + " : loop");
            m_indent++;
            if (!test.empty()) {
                line("exit " + label + " when not (" + test + ");");
            }
            emit_commands(guard.m_body);
            m_indent--;
            line("end loop " + label + ";");
            return;
        }
        line("if " + (test.empty() ? std::string("True") : test) + " then");
        m_indent++;
        emit_commands(guard.m_body);
        m_indent--;
        line("else");
        line("   R.Fail (Id, \"no guard of the alternative command at " +
             location(command.m_location) + " is true\");");
        line("end if;");
    }

    // Accept from a source, store the parts in the targets and trace the message.
    void EmitProcessWriter::emit_accept(const data::DataCommand &input, std::string_view source)
    {
        const data::DataEntry &entry = *input.m_entry;
        std::vector<std::string> parameters;
        std::vector<std::string> names;
        for (size_t i = 0; i < entry.m_types.size(); i++) {
            names.push_back("Arg_" + std::to_string(i + 1));
            parameters.push_back(names.back() + " : " +
                                 std::string(EmitExpressionWriter::ada_type(entry.m_types[i])));
        }
        line("accept " + entry.m_ada + " (" + std::string(source) + ")" +
             (parameters.empty() ? "" : " (" + join(parameters, "; ") + ")") + " do");
        m_indent++;
        for (size_t i = 0; i < input.m_parts.size(); i++) {
            line(m_expressions.render(*input.m_parts[i]) + " := " + names[i] + ";");
        }
        line("if R.Tracing then");
        line("   R.Comm (" + std::string(source) + ", Id, " + message_image(input, names) + ");");
        line("end if;");
        m_indent--;
        line("end " + entry.m_ada + ";");
    }

    // Join a guard's conditions.
    std::string EmitProcessWriter::condition(const data::DataGuardedCommand &guard)
    {
        std::vector<std::string> tests;
        for (const data::DataGuardElement &element : guard.m_elements) {
            if (element.m_condition) {
                tests.push_back(m_expressions.render(*element.m_condition));
            }
        }
        return join(tests, " and then ");
    }

    // Render has(1, 2), (1, 2) or 1 as an Ada String expression.
    std::string EmitProcessWriter::message_image(const data::DataCommand &command,
                                                 const std::vector<std::string> &values)
    {
        std::vector<std::string> images;
        for (const std::string &value : values) {
            images.push_back("R.Img (" + value + ")");
        }
        std::string parts = join(images, " & \", \" & ");
        if (!command.m_anonymous) {
            return images.empty() ? "\"" + command.m_constructor + "()\""
                                  : "\"" + command.m_constructor + "(\" & " + parts + " & \")\"";
        }
        if (images.size() == 1) {
            return parts;
        }
        return "\"(\" & " + parts + " & \")\"";
    }

    // Call Family_Id with the process's subscripts.
    std::string EmitProcessWriter::process_id(const data::DataProcessName &process)
    {
        std::vector<std::string> args = {"Id"};
        for (const std::unique_ptr<data::DataExpression> &subscript : process.m_subscripts) {
            args.push_back(m_expressions.render(*subscript));
        }
        return process.m_family->m_ada + "_Id (" + join(args, ", ") + ")";
    }

    // Spell a position as file:line:column, ready to go inside an Ada string.
    std::string EmitProcessWriter::location(const data::DataLocation &location) const
    {
        std::string text;
        for (char c : m_source) {
            text += c == '"' ? std::string("\"\"") : std::string(1, c);
        }
        return text + ":" + std::to_string(location.m_line) + ":" +
               std::to_string(location.m_column);
    }

}
