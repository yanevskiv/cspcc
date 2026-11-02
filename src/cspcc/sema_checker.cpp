// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/sema_checker.h"
#include "cspcc/ast_cloner.h"
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"

#include <algorithm>
#include <cctype>

namespace cspcc
{

    namespace
    {

        // Largest number of processes a program may have.
        constexpr long long semaMaxProcesses = 10000;

        // Largest number of alternatives a guard's ranges may unroll into.
        constexpr long long semaMaxAlternatives = 1000;

        // Lower-case a name, for near-miss hints.
        std::string lower(std::string_view text)
        {
            std::string result(text);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            return result;
        }

        // Check whether a type is a number.
        bool numeric(BaseType base)
        {
            return base == BaseType::Integer || base == BaseType::Real;
        }

        // Map a type's name to a type, the slides' double and portion included.
        std::optional<BaseType> base_of(std::string_view name)
        {
            if (name == "integer" || name == "int") {
                return BaseType::Integer;
            }
            if (name == "real" || name == "double" || name == "float") {
                return BaseType::Real;
            }
            if (name == "character" || name == "char") {
                return BaseType::Character;
            }
            if (name == "boolean" || name == "bool") {
                return BaseType::Boolean;
            }
            return std::nullopt;
        }

    }

    // Prepare to check one program.
    SemaChecker::SemaChecker(DiagnosticsEngine &diagnostics)
        : m_diagnostics(diagnostics)
    {
    }

    // Run the stages in order; each needs the ones before it.
    std::unique_ptr<Program>
    SemaChecker::check(Ast &ast, const std::vector<Define> &defines)
    {
        m_program = std::make_unique<Program>();
        m_scopes.assign(1, {});
        check_constants(ast, defines);
        std::vector<Definition *> processes = classify_definitions(ast);
        build_families(processes);
        for (const std::unique_ptr<Family> &family : m_program->m_families) {
            for (const std::unique_ptr<Variant> &variant : family->m_variants) {
                resolve_variant(*family, *variant);
            }
        }
        match_outputs();
        if (m_program->m_total == 0 && m_diagnostics.error_count() == 0) {
            m_diagnostics.error({}, "ERR_SEMA_NO_PROCESSES", "the program has no processes");
        }
        if (m_diagnostics.error_count()) {
            return nullptr;
        }
        return std::move(m_program);
    }

    // Name a type.
    std::string_view SemaChecker::type_name(BaseType base)
    {
        switch (base) {
            case BaseType::Integer:
                return "integer";
            case BaseType::Real:
                return "real";
            case BaseType::Character:
                return "character";
            case BaseType::Boolean:
                return "boolean";
            case BaseType::None:
                break;
        }
        return "unknown";
    }

    // Describe a message such as has(integer) or (integer, real).
    std::string SemaChecker::message_text(std::string_view constructor, bool anonymous,
                                          const std::vector<BaseType> &types)
    {
        std::string parts;
        for (size_t i = 0; i < types.size(); i++) {
            parts += (i ? ", " : "") + std::string(type_name(types[i]));
        }
        if (!anonymous) {
            return std::string(constructor) + "(" + parts + ")";
        }
        return types.size() == 1 ? parts : "(" + parts + ")";
    }

    // Resolve the constants in order, a -D replacing a const's value or coming first.
    void SemaChecker::check_constants(Ast &ast,
                                      const std::vector<Define> &defines)
    {
        std::vector<ConstantValue> pending;
        for (const std::unique_ptr<ConstantDefinition> &constant : ast.m_constants) {
            pending.push_back({constant->m_name, constant->m_location, constant->m_value.get()});
        }
        std::vector<ConstantValue> added;
        for (const Define &define : defines) {
            Expression *value = parse_define(define);
            if (!value) {
                continue;
            }
            auto same =
                std::find_if(pending.begin(), pending.end(), [&](const ConstantValue &p) {
                    return p.m_name == define.m_name;
                });
            if (same != pending.end()) {
                same->m_value = value;
            } else {
                added.push_back({define.m_name, {}, value});
            }
        }
        pending.insert(pending.begin(), added.begin(), added.end());
        for (const ConstantValue &constant : pending) {
            if (m_scopes[0].count(constant.m_name)) {
                m_diagnostics.error(constant.m_location, "ERR_SEMA_REDECLARED",
                                    "the constant '" + constant.m_name + "' is defined twice");
                continue;
            }
            BaseType base = resolve_expression(*constant.m_value);
            Symbol *symbol =
                declare(constant.m_name, SymbolKind::Constant, base, constant.m_location);
            symbol->m_value = constant.m_value;
            if (base == BaseType::Integer) {
                std::optional<long long> value = fold(*constant.m_value);
                symbol->m_known = value.has_value();
                symbol->m_integer = value.value_or(0);
            }
            m_program->m_constants.push_back(symbol);
        }
    }

    // Parse a -D value as an expression of its own.
    Expression *SemaChecker::parse_define(const Define &define)
    {
        DiagnosticsEngine local("-D " + define.m_name, define.m_value, m_diagnostics.stream());
        LexerTokenizer tokenizer(define.m_value, local);
        std::vector<Token> tokens = tokenizer.tokenize();
        std::unique_ptr<Expression> value;
        if (!local.error_count()) {
            value = ParserGrammar(tokens, local).parse_lone_expression();
        }
        if (!value) {
            m_diagnostics.error({}, "ERR_SEMA_BAD_DEFINE",
                                "the value of -D " + define.m_name + " isn't an expression");
            return nullptr;
        }
        m_program->m_expressions.push_back(std::move(value));
        return m_program->m_expressions.back().get();
    }

    // Sort the definitions into macros and processes.
    std::vector<Definition *> SemaChecker::classify_definitions(Ast &ast)
    {
        std::set<std::string> uses;
        for (const std::unique_ptr<Definition> &definition : ast.m_definitions) {
            collect_macro_uses(definition->m_body, uses);
        }
        std::vector<Definition *> processes;
        for (const std::unique_ptr<Definition> &definition : ast.m_definitions) {
            bool macro = !definition->m_in_parallel && !definition->m_has_subscripts &&
                         uses.count(definition->m_name);
            if (!macro) {
                processes.push_back(definition.get());
                continue;
            }
            auto [existing, inserted] = m_macros.emplace(definition->m_name, definition.get());
            if (!inserted) {
                m_diagnostics.error(definition->m_location, "ERR_SEMA_REDECLARED",
                                    "the macro '" + definition->m_name + "' is defined twice");
                m_diagnostics.note(existing->second->m_location, "first defined here");
            }
        }
        return processes;
    }

    // Walk commands and guards for macro uses.
    void
    SemaChecker::collect_macro_uses(const std::vector<std::unique_ptr<Command>> &commands,
                                    std::set<std::string> &names)
    {
        for (const std::unique_ptr<Command> &command : commands) {
            if (command->m_kind == CommandKind::Macro) {
                names.insert(command->m_name);
            }
            collect_macro_uses(command->m_body, names);
            for (const std::unique_ptr<GuardedCommand> &guard : command->m_guards) {
                collect_macro_uses(guard->m_body, names);
            }
        }
    }

    // Make a family per name and a variant per definition, then number the processes.
    void SemaChecker::build_families(const std::vector<Definition *> &definitions)
    {
        std::map<Family *, std::set<std::vector<long long>>> taken;
        for (Definition *definition : definitions) {
            if (m_macros.count(definition->m_name)) {
                m_diagnostics.error(definition->m_location, "ERR_SEMA_REDECLARED",
                                    "'" + definition->m_name + "' is both a process and a macro");
                continue;
            }
            Family *&family = m_families[definition->m_name];
            if (!family) {
                m_program->m_families.push_back(std::make_unique<Family>());
                family = m_program->m_families.back().get();
                family->m_name = definition->m_name;
                family->m_location = definition->m_location;
                family->m_arity = static_cast<int>(definition->m_subscripts.size());
            }
            if (family->m_arity != static_cast<int>(definition->m_subscripts.size())) {
                m_diagnostics.error(definition->m_location, "ERR_SEMA_ARITY",
                                    "'" + family->m_name + "' has " +
                                        std::to_string(definition->m_subscripts.size()) +
                                        " subscripts here and " + std::to_string(family->m_arity) +
                                        " where it is first defined");
                m_diagnostics.note(family->m_location, "first defined here");
                continue;
            }
            family->m_variants.push_back(std::make_unique<Variant>());
            family->m_variants.back()->m_definition = definition;
            enumerate_instances(*family, *family->m_variants.back(), taken[family]);
        }
        int pid = 1;
        for (const std::unique_ptr<Family> &family : m_program->m_families) {
            family->m_first = pid;
            family->m_lows.assign(family->m_arity, 0);
            family->m_highs.assign(family->m_arity, 0);
            for (size_t i = 0; i < family->m_instances.size(); i++) {
                Instance &instance = family->m_instances[i];
                instance.m_pid = pid++;
                for (int d = 0; d < family->m_arity; d++) {
                    long long index = instance.m_indices[d];
                    family->m_lows[d] = i ? std::min(family->m_lows[d], index) : index;
                    family->m_highs[d] = i ? std::max(family->m_highs[d], index) : index;
                }
            }
            family->m_last = pid - 1;
        }
        m_program->m_total = pid - 1;
    }

    // Take the cartesian product of the label's subscripts.
    void SemaChecker::enumerate_instances(Family &family, Variant &variant,
                                          std::set<std::vector<long long>> &taken)
    {
        std::vector<std::pair<long long, long long>> bounds;
        long long count = 1;
        for (LabelSubscript &subscript : variant.m_definition->m_subscripts) {
            if (subscript.m_range) {
                if (!resolve_bounds(*subscript.m_range)) {
                    return;
                }
                bounds.emplace_back(subscript.m_range->m_first, subscript.m_range->m_last);
            } else {
                std::optional<long long> value =
                    constant(*subscript.m_value, "a label's subscript");
                if (!value) {
                    return;
                }
                bounds.emplace_back(*value, *value);
            }
            count *= bounds.back().second - bounds.back().first + 1;
            if (count <= 0 || count > semaMaxProcesses) {
                m_diagnostics.error(variant.m_definition->m_location, "ERR_SEMA_TOO_MANY",
                                    count <= 0 ? "the label defines no process"
                                               : "the label defines more than 10000 processes");
                return;
            }
        }
        int variantNumber = static_cast<int>(family.m_variants.size());
        std::vector<long long> indices;
        for (const std::pair<long long, long long> &bound : bounds) {
            indices.push_back(bound.first);
        }
        for (long long n = 0; n < count; n++) {
            if (!taken.insert(indices).second) {
                std::string name = family.m_name + "(";
                for (size_t d = 0; d < indices.size(); d++) {
                    name += (d ? ", " : "") + std::to_string(indices[d]);
                }
                m_diagnostics.error(variant.m_definition->m_location, "ERR_SEMA_DUPLICATE_PROCESS",
                                    "the process " + name + ") is defined twice");
                return;
            }
            Instance instance;
            instance.m_indices = indices;
            instance.m_variant = variantNumber;
            family.m_instances.push_back(instance);
            for (size_t d = indices.size(); d-- > 0;) {
                if (++indices[d] <= bounds[d].second) {
                    break;
                }
                indices[d] = bounds[d].first;
            }
        }
    }

    // Resolve a body in the scope of the label's indices.
    void SemaChecker::resolve_variant(Family &family, Variant &variant)
    {
        m_family = &family;
        m_variant = &variant;
        m_scopes.emplace_back();
        int position = 1;
        for (LabelSubscript &subscript : variant.m_definition->m_subscripts) {
            if (subscript.m_range) {
                Symbol *symbol =
                    declare(subscript.m_range->m_variable, SymbolKind::Index,
                            BaseType::Integer, subscript.m_range->m_location);
                symbol->m_index_position = position;
                subscript.m_range->m_symbol = symbol;
                variant.m_index_symbols.push_back(symbol);
            }
            position++;
        }
        m_scopes.emplace_back();
        variant.m_definition->m_body = resolve_commands(std::move(variant.m_definition->m_body));
        m_scopes.pop_back();
        m_scopes.pop_back();
        m_family = nullptr;
        m_variant = nullptr;
    }

    // Replace each macro by a resolved copy of its body; a body that is one block is spliced.
    std::vector<std::unique_ptr<Command>>
    SemaChecker::resolve_commands(std::vector<std::unique_ptr<Command>> commands)
    {
        std::vector<std::unique_ptr<Command>> result;
        for (std::unique_ptr<Command> &command : commands) {
            auto macro = command->m_kind == CommandKind::Macro
                             ? m_macros.find(command->m_name)
                             : m_macros.end();
            if (macro == m_macros.end()) {
                resolve_command(*command);
                result.push_back(std::move(command));
                continue;
            }
            if (command->m_has_parens) {
                m_diagnostics.error(command->m_location, "ERR_SEMA_MACRO_ARGUMENTS",
                                    "the macro '" + command->m_name + "' takes no arguments");
                continue;
            }
            if (std::find(m_expanding.begin(), m_expanding.end(), command->m_name) !=
                m_expanding.end()) {
                m_diagnostics.error(command->m_location, "ERR_SEMA_MACRO_RECURSIVE",
                                    "the macro '" + command->m_name + "' uses itself");
                continue;
            }
            const std::vector<std::unique_ptr<Command>> &body = macro->second->m_body;
            bool spliced = body.size() == 1 && body[0]->m_kind == CommandKind::Block;
            m_expanding.push_back(command->m_name);
            std::vector<std::unique_ptr<Command>> expanded =
                resolve_commands(AstCloner::clone_commands(spliced ? body[0]->m_body : body));
            m_expanding.pop_back();
            for (std::unique_ptr<Command> &copy : expanded) {
                result.push_back(std::move(copy));
            }
        }
        return result;
    }

    // Resolve a command by its kind; a macro left here has no definition, so it is a hook.
    void SemaChecker::resolve_command(Command &command)
    {
        switch (command.m_kind) {
            case CommandKind::Skip:
                return;
            case CommandKind::Declare:
                resolve_declaration(command);
                return;
            case CommandKind::Assign:
                resolve_assignment(command);
                return;
            case CommandKind::Input:
            case CommandKind::Output:
                resolve_communication(command);
                return;
            case CommandKind::Macro:
                command.m_is_hook = true;
                for (std::unique_ptr<Expression> &arg : command.m_args) {
                    resolve_expression(*arg);
                }
                return;
            case CommandKind::Block:
                m_scopes.emplace_back();
                command.m_body = resolve_commands(std::move(command.m_body));
                m_scopes.pop_back();
                return;
            case CommandKind::Alternative:
            case CommandKind::Repetitive:
                for (std::unique_ptr<GuardedCommand> &guard : command.m_guards) {
                    resolve_guarded(*guard);
                }
                return;
        }
    }

    // Declare variables of a scalar or array type; an unknown type is taken as integer.
    void SemaChecker::resolve_declaration(Command &command)
    {
        std::optional<BaseType> base = base_of(command.m_type.m_name);
        if (!base) {
            m_diagnostics.warning(command.m_type.m_location, "WARN_SEMA_UNKNOWN_TYPE",
                                  "'" + command.m_type.m_name +
                                      "' isn't a CSP type; it is taken as integer");
            base = BaseType::Integer;
        }
        std::vector<std::pair<long long, long long>> dimensions;
        for (size_t d = 0; d < command.m_type.m_lows.size(); d++) {
            std::optional<long long> low = constant(*command.m_type.m_lows[d], "an array's bound");
            std::optional<long long> high =
                constant(*command.m_type.m_highs[d], "an array's bound");
            if (!low || !high) {
                return;
            }
            if (*low > *high) {
                m_diagnostics.error(command.m_type.m_lows[d]->m_location, "ERR_SEMA_EMPTY_RANGE",
                                    "the array's bounds " + std::to_string(*low) + ".." +
                                        std::to_string(*high) + " are empty");
                return;
            }
            dimensions.emplace_back(*low, *high);
        }
        for (size_t i = 0; i < command.m_names.size(); i++) {
            const std::string &name = command.m_names[i];
            auto previous = m_scopes.back().find(name);
            if (previous != m_scopes.back().end()) {
                m_diagnostics.error(command.m_name_locations[i], "ERR_SEMA_REDECLARED",
                                    "'" + name + "' is declared twice in the same block");
                m_diagnostics.note(previous->second->m_location, "first declared here");
                continue;
            }
            Symbol *symbol =
                declare(name, SymbolKind::Variable, *base, command.m_name_locations[i]);
            symbol->m_dimensions = dimensions;
            command.m_symbols.push_back(symbol);
            if (m_variant) {
                m_variant->m_locals.push_back(symbol);
            }
        }
    }

    // Resolve each target and value of an assignment.
    void SemaChecker::resolve_assignment(Command &command)
    {
        if (command.m_targets.size() != command.m_values.size()) {
            m_diagnostics.error(command.m_location, "ERR_SEMA_TUPLE_SIZE",
                                "the assignment has " + std::to_string(command.m_targets.size()) +
                                    " targets and " + std::to_string(command.m_values.size()) +
                                    " values");
            return;
        }
        for (size_t i = 0; i < command.m_targets.size(); i++) {
            BaseType to = resolve_target(*command.m_targets[i]);
            BaseType from = resolve_expression(*command.m_values[i]);
            if (to != BaseType::None && from != BaseType::None &&
                !assignable(to, from)) {
                m_diagnostics.error(command.m_values[i]->m_location, "ERR_SEMA_TYPE",
                                    "can't assign " + std::string(type_name(from)) + " to " +
                                        std::string(type_name(to)));
            }
        }
    }

    // Resolve the process, then the message: a constructor, an element, a value or a tuple.
    void SemaChecker::resolve_communication(Command &command)
    {
        bool input = command.m_kind == CommandKind::Input;
        resolve_process(command.m_process);
        Expression &message = *command.m_message;
        command.m_parts.clear();
        command.m_constructor.clear();
        command.m_anonymous = true;
        Symbol *symbol =
            message.m_kind == ExpressionKind::Call ? lookup(message.m_name) : nullptr;
        if (message.m_kind == ExpressionKind::Tuple) {
            for (std::unique_ptr<Expression> &item : message.m_args) {
                command.m_parts.push_back(item.get());
            }
        } else if (message.m_kind == ExpressionKind::Call && !symbol) {
            command.m_constructor = message.m_name;
            command.m_anonymous = false;
            for (std::unique_ptr<Expression> &arg : message.m_args) {
                command.m_parts.push_back(arg.get());
            }
        } else {
            command.m_parts.push_back(&message);
        }
        std::vector<BaseType> types;
        bool resolved = true;
        for (Expression *part : command.m_parts) {
            if (input && part->m_kind == ExpressionKind::Name && !lookup(part->m_name)) {
                m_diagnostics.error(part->m_location, "ERR_SEMA_UNDECLARED",
                                    "'" + part->m_name + "' isn't declared");
                m_diagnostics.note(part->m_location, "a signal is written with parentheses, such "
                                                     "as " +
                                                         part->m_name + "()");
                resolved = false;
                continue;
            }
            BaseType base = input ? resolve_target(*part) : resolve_expression(*part);
            resolved = resolved && base != BaseType::None;
            types.push_back(base);
        }
        if (!resolved || !command.m_process.m_family) {
            return;
        }
        if (input) {
            command.m_entry = add_entry(*m_family, command);
        } else {
            m_outputs.emplace_back(&command, m_family);
        }
    }

    // Declare a guard's bound variables, then resolve its elements, input and body.
    void SemaChecker::resolve_guarded(GuardedCommand &guard)
    {
        m_scopes.emplace_back();
        long long count = 1;
        for (std::unique_ptr<Range> &range : guard.m_ranges) {
            if (!resolve_bounds(*range)) {
                continue;
            }
            count *= range->m_last - range->m_first + 1;
            if (count > semaMaxAlternatives) {
                m_diagnostics.error(range->m_location, "ERR_SEMA_TOO_MANY",
                                    "the guard's ranges unroll into more than 1000 alternatives");
            }
            range->m_symbol = declare(range->m_variable, SymbolKind::Bound,
                                      BaseType::Integer, range->m_location);
            if (m_variant) {
                m_variant->m_locals.push_back(range->m_symbol);
            }
        }
        for (GuardElement &element : guard.m_elements) {
            if (element.m_declaration) {
                resolve_declaration(*element.m_declaration);
                continue;
            }
            BaseType base = resolve_expression(*element.m_condition);
            if (base != BaseType::None && base != BaseType::Boolean) {
                m_diagnostics.error(element.m_condition->m_location, "ERR_SEMA_TYPE",
                                    "a guard's condition must be boolean, not " +
                                        std::string(type_name(base)));
            }
        }
        if (guard.m_input) {
            resolve_communication(*guard.m_input);
        }
        guard.m_body = resolve_commands(std::move(guard.m_body));
        m_scopes.pop_back();
    }

    // Find the family and check its subscripts.
    void SemaChecker::resolve_process(ProcessName &process)
    {
        for (std::unique_ptr<Expression> &subscript : process.m_subscripts) {
            BaseType base = resolve_expression(*subscript);
            if (base != BaseType::None && base != BaseType::Integer) {
                m_diagnostics.error(subscript->m_location, "ERR_SEMA_TYPE",
                                    "a process subscript must be an integer, not " +
                                        std::string(type_name(base)));
            }
        }
        auto family = m_families.find(process.m_name);
        if (family == m_families.end()) {
            report_unknown_process(process);
            return;
        }
        if (static_cast<int>(process.m_subscripts.size()) != family->second->m_arity) {
            m_diagnostics.error(process.m_location, "ERR_SEMA_SUBSCRIPTS",
                                "'" + process.m_name + "' takes " +
                                    std::to_string(family->second->m_arity) + " subscripts, not " +
                                    std::to_string(process.m_subscripts.size()));
            return;
        }
        process.m_family = family->second;
    }

    // Type an expression bottom-up, quietly giving None above an operand that failed.
    BaseType SemaChecker::resolve_expression(Expression &expression)
    {
        using Base = BaseType;
        using Kind = TokenKind;
        Base result = Base::None;
        switch (expression.m_kind) {
            case ExpressionKind::Integer:
                result = Base::Integer;
                break;
            case ExpressionKind::Real:
                result = Base::Real;
                break;
            case ExpressionKind::Character:
                result = Base::Character;
                break;
            case ExpressionKind::Boolean:
                result = Base::Boolean;
                break;
            case ExpressionKind::Name:
            case ExpressionKind::Call: {
                bool call = expression.m_kind == ExpressionKind::Call;
                Symbol *symbol = lookup(expression.m_name);
                if (!symbol) {
                    m_diagnostics.error(expression.m_location,
                                        call ? "ERR_SEMA_CONSTRUCTOR_IN_EXPRESSION"
                                             : "ERR_SEMA_UNDECLARED",
                                        call ? "'" + expression.m_name +
                                                   "' isn't an array; a constructor can only be "
                                                   "sent or received"
                                             : "'" + expression.m_name + "' isn't declared");
                    if (m_families.count(expression.m_name)) {
                        m_diagnostics.note(expression.m_location,
                                           "'" + expression.m_name +
                                               "' is a process; a process is only named in "
                                               "inputs and outputs");
                    }
                    break;
                }
                expression.m_symbol = symbol;
                size_t subscripts = call ? expression.m_args.size() : 0;
                if (subscripts != symbol->m_dimensions.size()) {
                    m_diagnostics.error(expression.m_location, "ERR_SEMA_SUBSCRIPTS",
                                        "'" + expression.m_name + "' has " +
                                            std::to_string(symbol->m_dimensions.size()) +
                                            " dimensions, not " + std::to_string(subscripts));
                    break;
                }
                bool good = true;
                for (std::unique_ptr<Expression> &arg : expression.m_args) {
                    Base index = resolve_expression(*arg);
                    if (index != Base::None && index != Base::Integer) {
                        m_diagnostics.error(arg->m_location, "ERR_SEMA_TYPE",
                                            "an array subscript must be an integer, not " +
                                                std::string(type_name(index)));
                    }
                    good = good && index == Base::Integer;
                }
                result = good ? symbol->m_base : Base::None;
                break;
            }
            case ExpressionKind::Unary: {
                Base operand = resolve_expression(*expression.m_args[0]);
                bool minus = expression.m_op == Kind::Minus;
                if (operand == Base::None) {
                    break;
                }
                if (minus ? numeric(operand) : operand == Base::Boolean) {
                    result = operand;
                    break;
                }
                m_diagnostics.error(expression.m_location, "ERR_SEMA_TYPE",
                                    std::string(minus ? "'-' needs a number"
                                                      : "'not' needs a "
                                                        "boolean") +
                                        ", not " + std::string(type_name(operand)));
                break;
            }
            case ExpressionKind::Binary: {
                Base left = resolve_expression(*expression.m_args[0]);
                Base right = resolve_expression(*expression.m_args[1]);
                if (left == Base::None || right == Base::None) {
                    break;
                }
                Kind op = expression.m_op;
                bool arithmetic =
                    op == Kind::Plus || op == Kind::Minus || op == Kind::Star || op == Kind::Slash;
                bool integral = op == Kind::Mod || op == Kind::Div;
                bool equality = op == Kind::Equal || op == Kind::NotEqual;
                bool logical = op == Kind::And || op == Kind::Or;
                bool both = numeric(left) && numeric(right);
                if (arithmetic && both) {
                    result = left == Base::Real || right == Base::Real ? Base::Real : Base::Integer;
                } else if (integral && left == Base::Integer && right == Base::Integer) {
                    result = Base::Integer;
                } else if (logical && left == Base::Boolean && right == Base::Boolean) {
                    result = Base::Boolean;
                } else if (equality && (both || left == right)) {
                    result = Base::Boolean;
                } else if (!arithmetic && !integral && !logical && !equality &&
                           (both || (left == Base::Character && right == Base::Character))) {
                    result = Base::Boolean;
                } else {
                    m_diagnostics.error(expression.m_location, "ERR_SEMA_TYPE",
                                        std::string(LexerTokenizer::kind_name(op)) +
                                            " can't take " + std::string(type_name(left)) +
                                            " and " + std::string(type_name(right)));
                }
                break;
            }
            case ExpressionKind::Tuple:
                m_diagnostics.error(expression.m_location, "ERR_SEMA_TUPLE",
                                    "a tuple can only be sent, received or assigned");
                break;
        }
        expression.m_type = result;
        return result;
    }

    // Type a target: a scalar variable or an element of an array variable.
    BaseType SemaChecker::resolve_target(Expression &expression)
    {
        bool name = expression.m_kind == ExpressionKind::Name;
        bool call = expression.m_kind == ExpressionKind::Call;
        if (!name && !call) {
            m_diagnostics.error(expression.m_location, "ERR_SEMA_NOT_ASSIGNABLE",
                                "only a variable or an array element can be assigned to");
            return BaseType::None;
        }
        Symbol *symbol = lookup(expression.m_name);
        if (symbol && symbol->m_kind != SymbolKind::Variable) {
            m_diagnostics.error(expression.m_location, "ERR_SEMA_NOT_ASSIGNABLE",
                                "'" + expression.m_name + "' is a " +
                                    (symbol->m_kind == SymbolKind::Constant
                                         ? "constant"
                                         : "process or range index") +
                                    " and can't be assigned to");
            return BaseType::None;
        }
        return resolve_expression(expression);
    }

    // Compute both bounds of a range.
    bool SemaChecker::resolve_bounds(Range &range)
    {
        std::optional<long long> low = constant(*range.m_low, "a range's bound");
        std::optional<long long> high = constant(*range.m_high, "a range's bound");
        if (!low || !high) {
            return false;
        }
        if (*low > *high) {
            m_diagnostics.error(range.m_location, "ERR_SEMA_EMPTY_RANGE",
                                "the range " + std::to_string(*low) + ".." + std::to_string(*high) +
                                    " is empty");
            return false;
        }
        range.m_first = *low;
        range.m_last = *high;
        return true;
    }

    // Fold integer literals, known constants and integer operations.
    std::optional<long long> SemaChecker::fold(const Expression &expression) const
    {
        using Kind = TokenKind;
        switch (expression.m_kind) {
            case ExpressionKind::Integer:
                return expression.m_integer;
            case ExpressionKind::Name: {
                Symbol *symbol = lookup(expression.m_name);
                if (symbol && symbol->m_kind == SymbolKind::Constant && symbol->m_known) {
                    return symbol->m_integer;
                }
                return std::nullopt;
            }
            case ExpressionKind::Unary: {
                std::optional<long long> operand = fold(*expression.m_args[0]);
                if (operand && expression.m_op == Kind::Minus) {
                    return -*operand;
                }
                return std::nullopt;
            }
            case ExpressionKind::Binary: {
                std::optional<long long> left = fold(*expression.m_args[0]);
                std::optional<long long> right = fold(*expression.m_args[1]);
                if (!left || !right) {
                    return std::nullopt;
                }
                switch (expression.m_op) {
                    case Kind::Plus:
                        return *left + *right;
                    case Kind::Minus:
                        return *left - *right;
                    case Kind::Star:
                        return *left * *right;
                    case Kind::Slash:
                    case Kind::Div:
                        return *right ? std::optional<long long>(*left / *right) : std::nullopt;
                    case Kind::Mod:
                        return *right ? std::optional<long long>(*left % *right) : std::nullopt;
                    default:
                        return std::nullopt;
                }
            }
            default:
                return std::nullopt;
        }
    }

    // Fold a value that must be constant.
    std::optional<long long> SemaChecker::constant(const Expression &expression,
                                                   std::string_view what)
    {
        std::optional<long long> value = fold(expression);
        if (!value) {
            m_diagnostics.error(expression.m_location, "ERR_SEMA_NOT_CONSTANT",
                                std::string(what) + " must be an integer constant");
        }
        return value;
    }

    // Reuse the entry for the same constructor and types, or add one.
    Entry *SemaChecker::add_entry(Family &family,
                                            const Command &command)
    {
        std::vector<BaseType> types;
        for (const Expression *part : command.m_parts) {
            types.push_back(part->m_type);
        }
        for (const std::unique_ptr<Entry> &entry : family.m_entries) {
            if (entry->m_anonymous == command.m_anonymous &&
                entry->m_constructor == command.m_constructor && entry->m_types == types) {
                return entry.get();
            }
        }
        family.m_entries.push_back(std::make_unique<Entry>());
        Entry *entry = family.m_entries.back().get();
        entry->m_constructor = command.m_constructor;
        entry->m_anonymous = command.m_anonymous;
        entry->m_types = types;
        return entry;
    }

    // Match each output with an entry of its receiver, the same types or integers into reals,
    // or warn and give the receiver an entry it never accepts.
    void SemaChecker::match_outputs()
    {
        for (auto [command, sender] : m_outputs) {
            Family &receiver = *command->m_process.m_family;
            std::vector<BaseType> types;
            for (const Expression *part : command->m_parts) {
                types.push_back(part->m_type);
            }
            Entry *exact = nullptr;
            Entry *close = nullptr;
            for (const std::unique_ptr<Entry> &entry : receiver.m_entries) {
                if (entry->m_anonymous != command->m_anonymous ||
                    entry->m_constructor != command->m_constructor ||
                    entry->m_types.size() != types.size()) {
                    continue;
                }
                if (entry->m_types == types) {
                    exact = entry.get();
                }
                bool fits = true;
                for (size_t i = 0; i < types.size(); i++) {
                    fits = fits && assignable(entry->m_types[i], types[i]);
                }
                if (fits && !close) {
                    close = entry.get();
                }
            }
            command->m_entry = exact ? exact : close;
            if (command->m_entry) {
                continue;
            }
            m_diagnostics.warning(
                command->m_location, "WARN_SEMA_NO_MATCHING_INPUT",
                "no input of '" + receiver.m_name + "' receives " +
                    message_text(command->m_constructor, command->m_anonymous, types) +
                    ", so the output would wait forever");
            std::string receives;
            for (const std::unique_ptr<Entry> &entry : receiver.m_entries) {
                receives += (receives.empty() ? "" : ", ") +
                            message_text(entry->m_constructor, entry->m_anonymous, entry->m_types);
            }
            m_diagnostics.note(receiver.m_location, "'" + receiver.m_name + "' receives " +
                                                        (receives.empty() ? "nothing" : receives));
            command->m_entry = add_entry(receiver, *command);
        }
    }

    // Find a name, innermost scope first.
    Symbol *SemaChecker::lookup(const std::string &name) const
    {
        for (auto scope = m_scopes.rbegin(); scope != m_scopes.rend(); ++scope) {
            auto found = scope->find(name);
            if (found != scope->end()) {
                return found->second;
            }
        }
        return nullptr;
    }

    // Create a symbol owned by the program, visible in the innermost scope.
    Symbol *SemaChecker::declare(const std::string &name, SymbolKind kind,
                                           BaseType base,
                                           const Location &location)
    {
        m_program->m_symbols.push_back(std::make_unique<Symbol>());
        Symbol *symbol = m_program->m_symbols.back().get();
        symbol->m_name = name;
        symbol->m_kind = kind;
        symbol->m_base = base;
        symbol->m_location = location;
        m_scopes.back()[name] = symbol;
        return symbol;
    }

    // Report the name, suggesting a family or a macro spelled the same but for case.
    void SemaChecker::report_unknown_process(const ProcessName &process)
    {
        m_diagnostics.error(process.m_location, "ERR_SEMA_UNKNOWN_PROCESS",
                            "no process is named '" + process.m_name + "'");
        for (const auto &[name, family] : m_families) {
            if (lower(name) == lower(process.m_name)) {
                m_diagnostics.note(family->m_location, "did you mean '" + name + "'?");
                return;
            }
        }
        if (m_macros.count(process.m_name)) {
            m_diagnostics.note(m_macros[process.m_name]->m_location,
                               "'" + process.m_name +
                                   "' is a macro, since a command uses it; a process is "
                                   "defined in a parallel command or never used as a command");
        }
    }

    // Check that types match, an integer going into a real.
    bool SemaChecker::assignable(BaseType to, BaseType from)
    {
        return to == from ||
               (to == BaseType::Real && from == BaseType::Integer);
    }

}
