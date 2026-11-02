// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_EMIT_ADA_WRITER_H
#define CSPCC_EMIT_ADA_WRITER_H

#include "cspcc/data/data_family.h"
#include "cspcc/data/data_program.h"
#include "cspcc/emit_name_table.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Writes a checked program as one Ada procedure with a task type per process family.
    class EmitAdaWriter
    {
    public:
        // Prepare to write a program.
        EmitAdaWriter(data::DataProgram &program, std::string unit, std::string source);

        // Write the whole procedure.
        std::string write();

        // Check whether a name can be the Ada main procedure's file name.
        static bool valid_unit(std::string_view name);

        // Make a unit name from any file name.
        static std::string unit_from(std::string_view name);

    private:
        // Give Ada names to the constants, the families and their entries.
        void name_everything();

        // Append a line at the current indentation.
        void line(std::string_view text);

        // Write the constants.
        void write_constants();

        // Write a family's task type, its table of processes and its lookups.
        void write_family_spec(const data::DataFamily &family);

        // Write a family's task body.
        void write_family_body(const data::DataFamily &family);

        // Write the aggregate mapping subscripts to process numbers.
        std::string map_aggregate(const data::DataFamily &family,
                                  const std::map<std::vector<long long>, int> &pids,
                                  std::vector<long long> &prefix) const;

        // Write the main procedure's statements.
        void write_main();

        // Program being written.
        data::DataProgram &m_program;

        // Name of the Ada main procedure.
        std::string m_unit;

        // Name of the CSP file.
        std::string m_source;

        // Names taken at the level of the main procedure.
        EmitNameTable m_names;

        // Text written so far.
        std::string m_text;

        // Indentation of the next line, in Ada levels.
        int m_indent = 0;
    };

}

#endif
