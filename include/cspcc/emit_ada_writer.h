/*
 * C++ header for the Ada main procedure of a translated CSP program.
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

#ifndef CSPCC_EMIT_ADA_WRITER_H
#define CSPCC_EMIT_ADA_WRITER_H

#include "cspcc/data/data_family.h"
#include "cspcc/data/data_program.h"
#include "cspcc/emit_name_table.h"

#include <map>
#include <string>
#include <string_view>
#include <vector>

/** Namespace for cspcc declarations. */
namespace cspcc
{
    /** Writes a checked program as one Ada procedure with a task type per process family. */
    class EmitAdaWriter
    {
    public:
        /**
         * Prepare to write a program.
         *
         * @param program Checked program, whose Ada names the writer fills in.
         * @param unit Name of the Ada main procedure.
         * @param source Name of the CSP file, for comments and messages.
         */
        EmitAdaWriter(data::DataProgram &program, std::string unit, std::string source);

        /**
         * Write the whole procedure.
         *
         * @return The text of the .adb file.
         */
        std::string write();

        /**
         * Check whether a name can be the Ada main procedure's file name.
         *
         * @param name File name without its extension.
         * @return Whether it is a lower-case Ada identifier that isn't reserved or the runtime's.
         */
        static bool valid_unit(std::string_view name);

        /**
         * Make a unit name from any file name.
         *
         * @param name File name without its extension.
         * @return A valid unit name.
         */
        static std::string unit_from(std::string_view name);

    private:
        /**
         * Give Ada names to the constants, the families and their entries.
         */
        void name_everything();

        /**
         * Append a line at the current indentation.
         *
         * @param text Line to append.
         */
        void line(std::string_view text);

        /**
         * Write the constants.
         */
        void write_constants();

        /**
         * Write a family's task type, its table of processes and its lookups.
         *
         * @param family Family to write.
         */
        void write_family_spec(const data::DataFamily &family);

        /**
         * Write a family's task body.
         *
         * @param family Family to write.
         */
        void write_family_body(const data::DataFamily &family);

        /**
         * Write the aggregate mapping subscripts to process numbers.
         *
         * @param family Family whose map to write.
         * @param pids Process number of each subscript tuple.
         * @param prefix Subscripts fixed so far.
         * @return The aggregate.
         */
        std::string map_aggregate(const data::DataFamily &family,
                                  const std::map<std::vector<long long>, int> &pids,
                                  std::vector<long long> &prefix) const;

        /**
         * Write the main procedure's statements.
         */
        void write_main();

        /** Program being written. */
        data::DataProgram &m_program;

        /** Name of the Ada main procedure. */
        std::string m_unit;

        /** Name of the CSP file. */
        std::string m_source;

        /** Names taken at the level of the main procedure. */
        EmitNameTable m_names;

        /** Text written so far. */
        std::string m_text;

        /** Indentation of the next line, in Ada levels. */
        int m_indent = 0;
    };

}

#endif
