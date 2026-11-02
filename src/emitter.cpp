// Writes a checked program as an Ada main procedure.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "emitter.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <utility>
#include <vector>

// Ada 2022's reserved words.
constexpr std::string_view reservedWords[] = {
    "abort",     "abs",        "abstract",  "accept",    "access", "aliased",  "all",
    "and",       "array",      "at",        "begin",     "body",   "case",     "constant",
    "declare",   "delay",      "delta",     "digits",    "do",     "else",     "elsif",
    "end",       "entry",      "exception", "exit",      "for",    "function", "generic",
    "goto",      "if",         "in",        "interface", "is",     "limited",  "loop",
    "mod",       "new",        "not",       "null",      "of",     "or",       "others",
    "out",       "overriding", "package",   "parallel",  "pragma", "private",  "procedure",
    "protected", "raise",      "range",     "record",    "rem",    "renames",  "requeue",
    "return",    "reverse",    "select",    "separate",  "some",   "subtype",  "synchronized",
    "tagged",    "task",       "terminate", "then",      "type",   "until",    "use",
    "when",      "while",      "with",      "xor",
};

// Fold a name to lower case.
static std::string lower(std::string_view text)
{
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

// Take the reserved words.
NameTable::NameTable()
{
    for (std::string_view word : reservedWords) {
        reserve(word);
    }
}

// Take a name.
void NameTable::reserve(std::string_view name)
{
    m_taken.insert(lower(name));
}

// Check a name.
bool NameTable::taken(std::string_view name) const
{
    return m_taken.count(lower(name)) != 0;
}

// Take the sanitized name, or the first free numbered one.
std::string NameTable::make(std::string_view base)
{
    std::string name = sanitize(base);
    std::string candidate = name;
    for (int n = 2; taken(candidate); n++) {
        candidate = name + "_" + std::to_string(n);
    }
    reserve(candidate);
    return candidate;
}

// Drop underscores Ada forbids and start with a letter.
std::string NameTable::sanitize(std::string_view name)
{
    std::string result;
    for (char c : name) {
        if (c == '_' && (result.empty() || result.back() == '_')) {
            continue;
        }
        result += c;
    }
    while (!result.empty() && result.back() == '_') {
        result.pop_back();
    }
    if (result.empty() || !std::isalpha(static_cast<unsigned char>(result[0]))) {
        result = "V_" + result;
    }
    return result;
}

// Check whether an operator compares.
static bool compares(TokenKind op)
{
    using Kind = TokenKind;
    return op == Kind::Equal || op == Kind::NotEqual || op == Kind::Less || op == Kind::LessEqual ||
           op == Kind::Greater || op == Kind::GreaterEqual;
}

// Spell a binary operator in Ada, and and or short-circuited.
static std::string_view ada_operator(TokenKind op)
{
    switch (op) {
        case TokenKind::Plus:
            return "+";
        case TokenKind::Minus:
            return "-";
        case TokenKind::Star:
            return "*";
        case TokenKind::Slash:
        case TokenKind::Div:
            return "/";
        case TokenKind::Mod:
            return "mod";
        case TokenKind::Equal:
            return "=";
        case TokenKind::NotEqual:
            return "/=";
        case TokenKind::Less:
            return "<";
        case TokenKind::LessEqual:
            return "<=";
        case TokenKind::Greater:
            return ">";
        case TokenKind::GreaterEqual:
            return ">=";
        case TokenKind::And:
            return "and then";
        case TokenKind::Or:
            return "or else";
        default:
            return "?";
    }
}

// Render by kind; a real result or a comparison with a real converts integer operands.
std::string ExpressionWriter::render(const Expression &expression, BaseType want) const
{
    using Base = BaseType;
    std::string text;
    switch (expression.m_kind) {
        case ExpressionKind::Integer:
            text = std::to_string(expression.m_integer);
            break;
        case ExpressionKind::Real:
            text = real_literal(expression.m_real);
            break;
        case ExpressionKind::Character:
            text = std::string("'") + static_cast<char>(expression.m_integer) + "'";
            break;
        case ExpressionKind::Boolean:
            text = expression.m_boolean ? "True" : "False";
            break;
        case ExpressionKind::Name: {
            auto substitution = m_substitutions.find(expression.m_symbol);
            text = substitution != m_substitutions.end() ? substitution->second
                                                         : expression.m_symbol->m_ada;
            break;
        }
        case ExpressionKind::Call: {
            text = expression.m_symbol->m_ada + " (";
            for (size_t i = 0; i < expression.m_args.size(); i++) {
                text += (i ? ", " : "") + render(*expression.m_args[i]);
            }
            text += ")";
            break;
        }
        case ExpressionKind::Unary:
            text = expression.m_op == TokenKind::Not
                       ? "(not " + render(*expression.m_args[0]) + ")"
                       : "(-" + render(*expression.m_args[0], expression.m_type) + ")";
            break;
        case ExpressionKind::Binary: {
            const Expression &left = *expression.m_args[0];
            const Expression &right = *expression.m_args[1];
            Base operands = Base::None;
            if (expression.m_type == Base::Real ||
                (compares(expression.m_op) &&
                 (left.m_type == Base::Real || right.m_type == Base::Real))) {
                operands = Base::Real;
            }
            text = "(" + render(left, operands) + " " + std::string(ada_operator(expression.m_op)) +
                   " " + render(right, operands) + ")";
            break;
        }
        case ExpressionKind::Tuple:
            text = "()";
            break;
    }
    if (want == Base::Real && expression.m_type == Base::Integer) {
        return "Long_Float (" + text + ")";
    }
    return text;
}

// Replace a symbol.
void ExpressionWriter::substitute(const Symbol *symbol, std::string text)
{
    m_substitutions[symbol] = std::move(text);
}

// Stop replacing a symbol.
void ExpressionWriter::forget(const Symbol *symbol)
{
    m_substitutions.erase(symbol);
}

// Name an Ada type.
std::string_view ExpressionWriter::ada_type(BaseType base)
{
    switch (base) {
        case BaseType::Real:
            return "Long_Float";
        case BaseType::Character:
            return "Character";
        case BaseType::Boolean:
            return "Boolean";
        default:
            return "Integer";
    }
}

// Give an initial value.
std::string_view ExpressionWriter::default_value(BaseType base)
{
    switch (base) {
        case BaseType::Real:
            return "0.0";
        case BaseType::Character:
            return "' '";
        case BaseType::Boolean:
            return "False";
        default:
            return "0";
    }
}

// Write the shortest round-trip digits with the point and exponent Ada needs.
std::string ExpressionWriter::real_literal(double value)
{
    char buffer[64];
    auto [end, status] = std::to_chars(buffer, buffer + sizeof buffer, value);
    std::string text(buffer, status == std::errc() ? end : buffer);
    size_t exponent = text.find('e');
    std::string mantissa = text.substr(0, exponent);
    if (mantissa.find('.') == std::string::npos) {
        mantissa += ".0";
    }
    return exponent == std::string::npos ? mantissa : mantissa + "E" + text.substr(exponent + 1);
}

// Spell an integer.
std::string ExpressionWriter::number(long long value)
{
    return value < 0 ? "(" + std::to_string(value) + ")" : std::to_string(value);
}

// Join texts with a separator.
static std::string join(const std::vector<std::string> &items, std::string_view separator)
{
    std::string text;
    for (size_t i = 0; i < items.size(); i++) {
        text += (i ? std::string(separator) : "") + items[i];
    }
    return text;
}

// Count the alternatives a guard's ranges unroll into.
static long long unrolled(const GuardedCommand &guard)
{
    long long count = 1;
    for (const std::unique_ptr<Range> &range : guard.m_ranges) {
        count *= range->m_last - range->m_first + 1;
    }
    return count;
}

// Keep the task body's names and the source's name.
ProcessWriter::ProcessWriter(const Family &family, const Variant &variant, NameTable &names,
                             std::string source)
    : m_family(family)
    , m_variant(variant)
    , m_names(names)
    , m_source(std::move(source))
{}

// Name the indices and locals, write the body, then put the declarations above it.
std::string ProcessWriter::write(int indent)
{
    for (Symbol *symbol : m_variant.m_index_symbols) {
        symbol->m_ada = m_names.make(symbol->m_name);
        m_declarations.push_back(symbol->m_ada + " : constant Integer := " + m_family.m_ada +
                                 "_Idx (Id, " + std::to_string(symbol->m_index_position) + ");");
    }
    for (Symbol *symbol : m_variant.m_locals) {
        symbol->m_ada = m_names.make(symbol->m_name);
        std::string type(ExpressionWriter::ada_type(symbol->m_base));
        std::string value(ExpressionWriter::default_value(symbol->m_base));
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
void ProcessWriter::line(std::string_view text)
{
    m_body += std::string(m_indent * 3, ' ');
    m_body += text;
    m_body += '\n';
}

// Declare a local.
std::string ProcessWriter::local(std::string_view base, std::string_view declaration)
{
    std::string name = m_names.make(base);
    m_declarations.push_back(name + " : " + std::string(declaration));
    return name;
}

// Declare Peer on first use.
const std::string &ProcessWriter::peer()
{
    if (m_peer.empty()) {
        m_peer = local("Peer", "Pid := 1;");
    }
    return m_peer;
}

// Write each command, then null if none wrote a statement.
void ProcessWriter::emit_commands(const std::vector<std::unique_ptr<Command>> &commands)
{
    size_t before = m_body.size();
    for (const std::unique_ptr<Command> &command : commands) {
        emit_command(*command);
    }
    if (m_body.size() == before) {
        line("null;");
    }
}

// Write a command by its kind; declarations are already above the block.
void ProcessWriter::emit_command(const Command &command)
{
    switch (command.m_kind) {
        case CommandKind::Skip:
            line("null;");
            return;
        case CommandKind::Declare:
            return;
        case CommandKind::Assign:
            emit_assignment(command);
            return;
        case CommandKind::Input:
            emit_input(command);
            return;
        case CommandKind::Output:
            emit_output(command);
            return;
        case CommandKind::Macro:
            emit_hook(command);
            return;
        case CommandKind::Block:
            emit_commands(command.m_body);
            return;
        case CommandKind::Alternative:
        case CommandKind::Repetitive:
            emit_alternative(command);
            return;
    }
}

// Assign one value, or a tuple's values all at once through temporaries.
void ProcessWriter::emit_assignment(const Command &command)
{
    if (command.m_targets.size() == 1) {
        const Expression &target = *command.m_targets[0];
        line(m_expressions.render(target) +
             " := " + m_expressions.render(*command.m_values[0], target.m_type) + ";");
        return;
    }
    std::vector<std::string> temporaries;
    for (size_t i = 0; i < command.m_values.size(); i++) {
        BaseType base = command.m_targets[i]->m_type;
        temporaries.push_back(
            local("Tmp", std::string(ExpressionWriter::ada_type(base)) +
                             " := " + std::string(ExpressionWriter::default_value(base)) + ";"));
        line(temporaries.back() + " := " + m_expressions.render(*command.m_values[i], base) + ";");
    }
    for (size_t i = 0; i < command.m_targets.size(); i++) {
        line(m_expressions.render(*command.m_targets[i]) + " := " + temporaries[i] + ";");
    }
}

// Wait for the input, failing if its source terminates first.
void ProcessWriter::emit_input(const Command &command)
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
void ProcessWriter::emit_output(const Command &command)
{
    const std::string &target = peer();
    const Entry &entry = *command.m_entry;
    std::vector<std::string> args;
    for (size_t i = 0; i < command.m_parts.size(); i++) {
        args.push_back(m_expressions.render(*command.m_parts[i], entry.m_types[i]));
    }
    std::string call = command.m_process.m_family->m_ada + "_T (" + target + ")." + entry.m_ada +
                       " (Id)" + (args.empty() ? "" : " (" + join(args, ", ") + ")");
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
void ProcessWriter::emit_hook(const Command &command)
{
    std::string text = "\"" + command.m_name;
    if (command.m_has_parens) {
        std::vector<std::string> images;
        for (const std::unique_ptr<Expression> &arg : command.m_args) {
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
void ProcessWriter::emit_alternative(const Command &command)
{
    bool repetitive = command.m_kind == CommandKind::Repetitive;
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
    for (const std::unique_ptr<GuardedCommand> &guard : guards) {
        long long count = unrolled(*guard);
        bases.push_back(total + 1);
        std::string choice =
            count == 1 ? std::to_string(total + 1)
                       : std::to_string(total + 1) + " .. " + std::to_string(total + count);
        bool input = guard->m_input != nullptr;
        inputMask += (inputMask.empty() ? "" : ", ") + choice + " => " + (input ? "True" : "False");
        total += count;
    }
    std::string range = "(1 .. " + std::to_string(total) + ")";
    std::string open = local("Open" + suffix, "R.Bool_Array " + range + " := (others => False);");
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
        const GuardedCommand &guard = *guards[g];
        int loops = 0;
        std::string offset = std::to_string(bases[g]);
        long long stride = unrolled(guard);
        for (const std::unique_ptr<Range> &range : guard.m_ranges) {
            long long count = range->m_last - range->m_first + 1;
            stride /= count;
            std::string counter = m_names.make("L" + suffix);
            line("for " + counter + " in " + ExpressionWriter::number(range->m_first) + " .. " +
                 ExpressionWriter::number(range->m_last) + " loop");
            m_indent++;
            line(range->m_symbol->m_ada + " := " + counter + ";");
            offset += " + (" + counter + " - " + ExpressionWriter::number(range->m_first) + ") * " +
                      std::to_string(stride);
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
    for (const std::unique_ptr<GuardedCommand> &guard : guards) {
        anyInput = anyInput || guard->m_input;
    }
    auto accepts = [&](auto when) {
        bool first = true;
        for (size_t g = 0; g < guards.size(); g++) {
            const GuardedCommand &guard = *guards[g];
            if (!guard.m_input) {
                continue;
            }
            long long count = unrolled(guard);
            for (long long k = 0; k < count; k++) {
                long long rest = k;
                long long stride = count;
                for (const std::unique_ptr<Range> &range : guard.m_ranges) {
                    long long size = range->m_last - range->m_first + 1;
                    stride /= size;
                    m_expressions.substitute(range->m_symbol,
                                             "(" + std::to_string(range->m_first + rest / stride) +
                                                 ")");
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
            for (const std::unique_ptr<Range> &range : guard.m_ranges) {
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
                line(ready + " (" + index + ") := " + ready + " (" + index + ") and then " + entry +
                     " (" + sources + " (" + index + "))'Count > 0;");
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
        line("R.Waiting (Id, \"alternative command at " + location(command.m_location) + "\");");
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
        line("exit when " + selected + " /= 0 or else not R.Any (" + open + ", " + inputs + ");");
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
        const GuardedCommand &guard = *guards[g];
        long long count = unrolled(guard);
        line("when " +
             (count == 1
                  ? std::to_string(bases[g])
                  : std::to_string(bases[g]) + " .. " + std::to_string(bases[g] + count - 1)) +
             " =>");
        m_indent++;
        long long stride = count;
        for (const std::unique_ptr<Range> &range : guard.m_ranges) {
            long long size = range->m_last - range->m_first + 1;
            stride /= size;
            line(range->m_symbol->m_ada + " := " + ExpressionWriter::number(range->m_first) +
                 " + ((" + selected + " - " + std::to_string(bases[g]) + ") / " +
                 std::to_string(stride) + ") mod " + std::to_string(size) + ";");
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
void ProcessWriter::emit_plain_alternative(const Command &command)
{
    const GuardedCommand &guard = *command.m_guards[0];
    std::string test = condition(guard);
    if (command.m_kind == CommandKind::Repetitive) {
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
    line("   R.Fail (Id, \"no guard of the alternative command at " + location(command.m_location) +
         " is true\");");
    line("end if;");
}

// Accept from a source, store the parts in the targets and trace the message.
void ProcessWriter::emit_accept(const Command &input, std::string_view source)
{
    const Entry &entry = *input.m_entry;
    std::vector<std::string> parameters;
    std::vector<std::string> names;
    for (size_t i = 0; i < entry.m_types.size(); i++) {
        names.push_back("Arg_" + std::to_string(i + 1));
        parameters.push_back(names.back() + " : " +
                             std::string(ExpressionWriter::ada_type(entry.m_types[i])));
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
std::string ProcessWriter::condition(const GuardedCommand &guard)
{
    std::vector<std::string> tests;
    for (const GuardElement &element : guard.m_elements) {
        if (element.m_condition) {
            tests.push_back(m_expressions.render(*element.m_condition));
        }
    }
    return join(tests, " and then ");
}

// Render has(1, 2), (1, 2) or 1 as an Ada String expression.
std::string ProcessWriter::message_image(const Command &command,
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
std::string ProcessWriter::process_id(const ProcessName &process)
{
    std::vector<std::string> args = {"Id"};
    for (const std::unique_ptr<Expression> &subscript : process.m_subscripts) {
        args.push_back(m_expressions.render(*subscript));
    }
    return process.m_family->m_ada + "_Id (" + join(args, ", ") + ")";
}

// Spell a position as file:line:column, ready to go inside an Ada string.
std::string ProcessWriter::location(const Location &location) const
{
    std::string text;
    for (char c : m_source) {
        text += c == '"' ? std::string("\"\"") : std::string(1, c);
    }
    return text + ":" + std::to_string(location.m_line) + ":" + std::to_string(location.m_column);
}

// Names the generated code uses besides the families' and the constants'.
constexpr std::string_view fixedNames[] = {
    "R",
    "Total",
    "Pid",
    "Id",
    "Ada",
    "Standard",
    "Integer",
    "Long_Float",
    "Boolean",
    "Character",
    "Natural",
    "Positive",
    "Duration",
    "String",
    "True",
    "False",
    "Tasking_Error",
    "Constraint_Error",
    "Program_Error",
    "Csp_Runtime",
    "E",
    "P",
    "Self",
    "I1",
};

// Suffixes of the names each family gets.
constexpr std::string_view familySuffixes[] = {
    "_Task", "_Access", "_T", "_Id", "_Map", "_Idx", "_Variant",
};

// Prepare to write a program.
AdaWriter::AdaWriter(Program &program, std::string unit, std::string source)
    : m_program(program)
    , m_unit(std::move(unit))
    , m_source(std::move(source))
{}

// Write the context, the declarations, the task bodies and the main statements.
std::string AdaWriter::write()
{
    name_everything();
    line("--  Generated by cspcc from " + m_source + ".");
    line("pragma Warnings (Off);");
    line("with Ada.Exceptions;");
    line("with Csp_Runtime;");
    line("");
    line("procedure " + m_unit + " is");
    m_indent++;
    line("");
    line("package R renames Csp_Runtime;");
    line("");
    line("Total : constant := " + std::to_string(m_program.m_total) + ";");
    line("subtype Pid is R.Pid range 1 .. Total;");
    write_constants();
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        write_family_spec(*family);
    }
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        write_family_body(*family);
    }
    m_indent--;
    line("");
    line("begin");
    m_indent++;
    write_main();
    m_indent--;
    line("end " + m_unit + ";");
    return m_text;
}

// Accept a lower-case identifier with no doubled or trailing underscore.
bool AdaWriter::valid_unit(std::string_view name)
{
    if (name.empty() || !std::islower(static_cast<unsigned char>(name[0])) || name.back() == '_' ||
        name.find("__") != std::string_view::npos || name == "csp_runtime") {
        return false;
    }
    for (char c : name) {
        if (!std::islower(static_cast<unsigned char>(c)) &&
            !std::isdigit(static_cast<unsigned char>(c)) && c != '_') {
            return false;
        }
    }
    return !NameTable().taken(name);
}

// Lower-case the name, turn other characters into underscores and fix what Ada rejects.
std::string AdaWriter::unit_from(std::string_view name)
{
    std::string unit;
    for (char c : name) {
        unsigned char u = static_cast<unsigned char>(c);
        char next = std::isalnum(u) && u < 0x80 ? static_cast<char>(std::tolower(u)) : '_';
        if (next == '_' && (unit.empty() || unit.back() == '_')) {
            continue;
        }
        unit += next;
    }
    while (!unit.empty() && unit.back() == '_') {
        unit.pop_back();
    }
    if (unit.empty() || !std::islower(static_cast<unsigned char>(unit[0]))) {
        unit = "csp_" + unit;
    }
    if (!valid_unit(unit)) {
        unit += "_main";
    }
    return unit;
}

// Take the fixed names, then name the constants, the families and their entries.
void AdaWriter::name_everything()
{
    m_names.reserve(m_unit);
    for (std::string_view name : fixedNames) {
        m_names.reserve(name);
    }
    size_t arity = 0;
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        for (const std::unique_ptr<Entry> &entry : family->m_entries) {
            arity = std::max(arity, entry->m_types.size());
        }
    }
    for (size_t i = 1; i <= arity; i++) {
        m_names.reserve("Arg_" + std::to_string(i));
    }
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        std::string base = NameTable::sanitize(family->m_name);
        std::string candidate = base;
        auto clashes = [&](const std::string &prefix) {
            return std::any_of(std::begin(familySuffixes), std::end(familySuffixes),
                               [&](std::string_view suffix) {
                                   return m_names.taken(prefix + std::string(suffix));
                               });
        };
        for (int n = 2; clashes(candidate); n++) {
            candidate = base + "_" + std::to_string(n);
        }
        for (std::string_view suffix : familySuffixes) {
            m_names.reserve(candidate + std::string(suffix));
        }
        family->m_ada = candidate;
    }
    for (Symbol *constant : m_program.m_constants) {
        constant->m_ada = m_names.make(constant->m_name);
    }
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        NameTable entries = m_names;
        for (const std::unique_ptr<Entry> &entry : family->m_entries) {
            entry->m_ada = entries.make(entry->m_anonymous ? "Msg" : "C_" + entry->m_constructor);
        }
    }
}

// Append a line.
void AdaWriter::line(std::string_view text)
{
    if (!text.empty()) {
        m_text += std::string(m_indent * 3, ' ');
    }
    m_text += text;
    m_text += '\n';
}

// Write integers as named numbers and other constants as typed ones.
void AdaWriter::write_constants()
{
    if (!m_program.m_constants.empty()) {
        line("");
    }
    for (const Symbol *constant : m_program.m_constants) {
        if (constant->m_base == BaseType::Integer && constant->m_known) {
            line(constant->m_ada +
                 " : constant := " + ExpressionWriter::number(constant->m_integer) + ";");
            continue;
        }
        line(constant->m_ada + " : constant " +
             std::string(ExpressionWriter::ada_type(constant->m_base)) +
             " := " + ExpressionWriter().render(*constant->m_value, constant->m_base) + ";");
    }
}

// Write the task type with an entry family per message kind, then the tables.
void AdaWriter::write_family_spec(const Family &family)
{
    const std::string &a = family.m_ada;
    std::string first = std::to_string(family.m_first);
    std::string last = std::to_string(family.m_last);
    line("");
    if (family.m_entries.empty()) {
        line("task type " + a + "_Task (Id : Pid);");
    } else {
        line("task type " + a + "_Task (Id : Pid) is");
        m_indent++;
        for (const std::unique_ptr<Entry> &entry : family.m_entries) {
            std::vector<std::string> parameters;
            for (size_t i = 0; i < entry->m_types.size(); i++) {
                parameters.push_back("Arg_" + std::to_string(i + 1) + " : " +
                                     std::string(ExpressionWriter::ada_type(entry->m_types[i])));
            }
            std::string profile;
            for (size_t i = 0; i < parameters.size(); i++) {
                profile += (i ? "; " : "") + parameters[i];
            }
            line("entry " + entry->m_ada + " (Pid)" +
                 (profile.empty() ? "" : " (" + profile + ")") + ";");
        }
        m_indent--;
        line("end " + a + "_Task;");
    }
    line("type " + a + "_Access is access " + a + "_Task;");
    line(a + "_T : array (Pid range " + first + " .. " + last + ") of " + a + "_Access;");
    std::string variants;
    for (const Instance &instance : family.m_instances) {
        variants += (variants.empty() ? "" : ", ") + std::to_string(instance.m_pid) + " => " +
                    std::to_string(instance.m_variant);
    }
    line(a + "_Variant : constant array (Pid range " + first + " .. " + last +
         ") of Positive := (" + variants + ");");
    if (family.m_arity == 0) {
        line("function " + a + "_Id (Self : Pid) return Pid is (" + first + ");");
        return;
    }
    std::string dimensions;
    std::string ranges;
    std::string checks;
    std::string subscripts;
    std::string parameters;
    std::string image;
    std::map<std::vector<long long>, int> pids;
    std::string indices;
    for (const Instance &instance : family.m_instances) {
        pids[instance.m_indices] = instance.m_pid;
        std::string tuple;
        for (size_t d = 0; d < instance.m_indices.size(); d++) {
            tuple += (d ? ", " : "") + std::to_string(d + 1) + " => " +
                     ExpressionWriter::number(instance.m_indices[d]);
        }
        indices +=
            (indices.empty() ? "" : ", ") + std::to_string(instance.m_pid) + " => (" + tuple + ")";
    }
    for (int d = 0; d < family.m_arity; d++) {
        std::string i = "I" + std::to_string(d + 1);
        ranges += (d ? ", " : "") + ExpressionWriter::number(family.m_lows[d]) + " .. " +
                  ExpressionWriter::number(family.m_highs[d]);
        checks += i + " in " + a + "_Map'Range (" + std::to_string(d + 1) + ") and then ";
        subscripts += (d ? ", " : "") + i;
        parameters += "; " + i + " : Integer";
        image += std::string(d ? " & \", \" & " : "") + "R.Img (" + i + ")";
    }
    std::vector<long long> prefix;
    line(a + "_Map : constant array (" + ranges +
         ") of Natural := " + map_aggregate(family, pids, prefix) + ";");
    line(a + "_Idx : constant array (Pid range " + first + " .. " + last + ", 1 .. " +
         std::to_string(family.m_arity) + ") of Integer := (" + indices + ");");
    line("function " + a + "_Id (Self : Pid" + parameters + ") return Pid is");
    line("begin");
    m_indent++;
    line("if " + checks + a + "_Map (" + subscripts + ") /= 0 then");
    line("   return " + a + "_Map (" + subscripts + ");");
    line("end if;");
    line("R.Fail (Self, \"no process " + family.m_name + "(\" & " + image + " & \")\");");
    m_indent--;
    line("end " + a + "_Id;");
}

// Write the task body: wait for the start, run the variant, then say it is done.
void AdaWriter::write_family_body(const Family &family)
{
    const std::string &a = family.m_ada;
    NameTable names = m_names;
    for (const std::unique_ptr<Entry> &entry : family.m_entries) {
        names.reserve(entry->m_ada);
    }
    line("");
    line("task body " + a + "_Task is");
    line("begin");
    m_indent++;
    line("R.Wait_Start;");
    bool several = family.m_variants.size() > 1;
    if (several) {
        line("case " + a + "_Variant (Id) is");
        m_indent++;
    }
    for (size_t v = 0; v < family.m_variants.size(); v++) {
        if (several) {
            line("when " + std::to_string(v + 1) + " =>");
        }
        ProcessWriter writer(family, *family.m_variants[v], names, m_source);
        m_text += writer.write(m_indent + (several ? 1 : 0));
    }
    if (several) {
        line("when others =>");
        line("   null;");
        m_indent--;
        line("end case;");
    }
    line("R.Done (Id);");
    m_indent--;
    line("exception");
    line("   when E : others =>");
    line("      R.Fail (Id, Ada.Exceptions.Exception_Name (E) & \": \" & "
         "Ada.Exceptions.Exception_Message (E));");
    line("end " + a + "_Task;");
}

// Write a nested named aggregate, listing the subscripts that name a process.
std::string AdaWriter::map_aggregate(const Family &family,
                                     const std::map<std::vector<long long>, int> &pids,
                                     std::vector<long long> &prefix) const
{
    size_t d = prefix.size();
    bool innermost = d + 1 == static_cast<size_t>(family.m_arity);
    std::string text = "(";
    for (long long i = family.m_lows[d]; i <= family.m_highs[d]; i++) {
        prefix.push_back(i);
        std::string element;
        if (innermost) {
            auto pid = pids.find(prefix);
            element = std::to_string(pid == pids.end() ? 0 : pid->second);
        } else {
            element = map_aggregate(family, pids, prefix);
        }
        prefix.pop_back();
        text += ExpressionWriter::number(i) + " => " + element + ", ";
    }
    std::string others = "0";
    for (size_t k = d + 1; k < static_cast<size_t>(family.m_arity); k++) {
        others = "(others => " + others + ")";
    }
    return text + "others => " + others + ")";
}

// Name each process and create its task, then let them all start.
void AdaWriter::write_main()
{
    line("R.Init (Total);");
    for (const std::unique_ptr<Family> &family : m_program.m_families) {
        const std::string &a = family->m_ada;
        if (family->m_arity == 0) {
            std::string pid = std::to_string(family->m_first);
            line("R.Set_Name (" + pid + ", \"" + family->m_name + "\");");
            line(a + "_T (" + pid + ") := new " + a + "_Task (" + pid + ");");
            continue;
        }
        std::string image;
        for (int d = 0; d < family->m_arity; d++) {
            image += std::string(d ? " & \",\" & " : "") + "R.Img (" + a + "_Idx (P, " +
                     std::to_string(d + 1) + "))";
        }
        line("for P in " + a + "_T'Range loop");
        line("   R.Set_Name (P, \"" + family->m_name + "(\" & " + image + " & \")\");");
        line("   " + a + "_T (P) := new " + a + "_Task (P);");
        line("end loop;");
    }
    line("R.Start;");
}
