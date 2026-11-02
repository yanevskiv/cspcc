// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_PROGRAM_H
#define CSPCC_PROGRAM_H

#include "cspcc/ast.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Describes what a name stands for.
    enum class SymbolKind {
        Constant,
        Variable,
        Index,
        Bound,
    };

    // Describes a constant, a variable, a label index or a bound variable.
    struct Symbol
    {
        // Name as written in CSP.
        std::string m_name;

        // What the name stands for.
        SymbolKind m_kind = SymbolKind::Variable;

        // Scalar type, or the element type of an array.
        BaseType m_base = BaseType::Integer;

        // Bounds of each dimension of an array.
        std::vector<std::pair<long long, long long>> m_dimensions;

        // Position of the declaration.
        Location m_location;

        // Name the Ada program gives the symbol.
        std::string m_ada;

        // Position of a label index among the label's subscripts, from 1.
        int m_index_position = 0;

        // Value of a constant.
        Expression *m_value = nullptr;

        // Whether the integer value of a constant is known.
        bool m_known = false;

        // Integer value of a constant.
        long long m_integer = 0;
    };

    // Describes one kind of message a process family receives.
    struct Entry
    {
        // Constructor of the message, empty when it has none.
        std::string m_constructor;

        // Whether the message is a plain value or tuple.
        bool m_anonymous = true;

        // Types of the message's parts.
        std::vector<BaseType> m_types;

        // Name of the Ada entry.
        std::string m_ada;
    };

    // Describes one process: its subscripts, its body and its number.
    struct Instance
    {
        // Subscripts of the process.
        std::vector<long long> m_indices;

        // Variant whose commands the process runs, from 1.
        int m_variant = 0;

        // Number of the process in the whole program, from 1.
        int m_pid = 0;
    };

    // Describes one definition of a family, such as M(i:1..3, 0)::WEST.
    struct Variant
    {
        // Definition the variant comes from.
        Definition *m_definition = nullptr;

        // Symbols of the label's ranges.
        std::vector<Symbol *> m_index_symbols;

        // Variables and bound variables declared in the body.
        std::vector<Symbol *> m_locals;
    };

    // Describes the processes sharing a name, such as Phil(0) to Phil(4).
    struct Family
    {
        // Name of the processes.
        std::string m_name;

        // Position of the first definition.
        Location m_location;

        // Number of subscripts each process has.
        int m_arity = -1;

        // Definitions of the family.
        std::vector<std::unique_ptr<Variant>> m_variants;

        // Processes of the family.
        std::vector<Instance> m_instances;

        // Messages the family receives.
        std::vector<std::unique_ptr<Entry>> m_entries;

        // Number of the family's first process.
        int m_first = 0;

        // Number of the family's last process.
        int m_last = 0;

        // Smallest subscript in each dimension.
        std::vector<long long> m_lows;

        // Largest subscript in each dimension.
        std::vector<long long> m_highs;

        // Prefix of the family's Ada names.
        std::string m_ada;
    };

    // Describes a checked program: its constants and its process families.
    struct Program
    {
        // Constants in the order they are defined.
        std::vector<Symbol *> m_constants;

        // Process families in source order.
        std::vector<std::unique_ptr<Family>> m_families;

        // Every symbol of the program.
        std::vector<std::unique_ptr<Symbol>> m_symbols;

        // Expressions sema creates, such as the values of -D.
        std::vector<std::unique_ptr<Expression>> m_expressions;

        // Number of processes.
        int m_total = 0;
    };

    // Describes a -D NAME=VALUE option.
    struct Define
    {
        // Name of the constant.
        std::string m_name;

        // Value as written.
        std::string m_value;
    };

    // Describes a constant's name and value, from a const or a -D.
    struct ConstantValue
    {
        // Name of the constant.
        std::string m_name;

        // Position of the const, or none for a -D.
        Location m_location;

        // Value, owned by the syntax tree or the program.
        Expression *m_value = nullptr;
    };
}

#endif
