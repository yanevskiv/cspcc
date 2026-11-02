// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#ifndef CSPCC_DRIVER_APP_H
#define CSPCC_DRIVER_APP_H

#include "cspcc/program.h"
#include "cspcc/driver_options.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Namespace for cspcc declarations.
namespace cspcc
{
    // Runs one cspcc invocation, from the CSP file to the Ada files.
    class DriverApp
    {
    public:
        // Keep the command line.
        DriverApp(int argc, const char *const *argv);

        // Run the invocation.
        int run();

    private:
        // Translate the input, already read.
        int translate(const std::string &source);

        // Write the Ada main procedure and, unless --no-runtime, the runtime next to it.
        int emit(Program &program);

        // Read a whole file.
        static bool read_file(const std::string &path, std::string &text);

        // Write a file, leaving it untouched when it already holds the text.
        static bool write_file(const std::filesystem::path &path, std::string_view text);

        // Spell a unit's file name as an Ada name.
        static std::string ada_name(std::string_view unit);

        // Name the program was run as.
        std::string m_program;

        // Arguments after the program's name.
        std::vector<const char *> m_args;

        // Parsed options.
        DriverOptions m_options;
    };

}

#endif
