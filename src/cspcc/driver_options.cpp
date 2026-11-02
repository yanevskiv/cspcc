// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/driver_options.h"

namespace cspcc
{

    // Parse the arguments; an option's value may be joined (-oFILE) or split (-o FILE).
    bool DriverOptions::parse(std::span<const char *const> args, std::string &error)
    {
        for (size_t i = 0; i < args.size(); i++) {
            std::string_view arg = args[i];
            if (arg == "-h" || arg == "--help") {
                m_wants_help = true;
            } else if (arg == "--version") {
                m_wants_version = true;
            } else if (arg == "--dump-tokens") {
                m_dumps_tokens = true;
            } else if (arg == "--no-runtime") {
                m_writes_runtime = false;
            } else if (arg == "-w") {
                m_warnings_enabled = false;
            } else if (arg == "-Werror") {
                m_warnings_as_errors = true;
            } else if (arg == "-o" || arg == "-D") {
                if (i + 1 == args.size()) {
                    error = "option '" + std::string(arg) + "' needs a value";
                    return false;
                }
                std::string_view value = args[++i];
                if (arg == "-o") {
                    m_output = value;
                } else if (!add_define(value, error)) {
                    return false;
                }
            } else if (arg.starts_with("-o")) {
                m_output = arg.substr(2);
            } else if (arg.starts_with("-D")) {
                if (!add_define(arg.substr(2), error)) {
                    return false;
                }
            } else if (arg.size() > 1 && arg[0] == '-') {
                error = "unknown option '" + std::string(arg) + "'";
                return false;
            } else if (!m_input.empty()) {
                error = "more than one input: '" + m_input + "' and '" + std::string(arg) + "'";
                return false;
            } else {
                m_input = arg;
            }
        }
        return true;
    }

    // Print the options.
    void DriverOptions::print_help(std::ostream &stream, std::string_view program)
    {
        stream << "Usage: " << program << " [options] INPUT\n"
               << "  INPUT              a CSP program\n"
               << "  -o OUTPUT          write the Ada main procedure to OUTPUT (default: INPUT's\n"
               << "                     name with .adb, in the current directory)\n"
               << "  -D NAME=VALUE      define the constant NAME, or override its const\n"
               << "  --no-runtime       don't write csp_runtime.ads and csp_runtime.adb next to\n"
               << "                     OUTPUT\n"
               << "  --dump-tokens      print the tokens and stop\n"
               << "  -w                 report no warnings\n"
               << "  -Werror            report warnings as errors\n"
               << "  -h, --help         show this help and exit\n"
               << "  --version          show the version and exit\n";
    }

    // Return the input.
    const std::string &DriverOptions::input() const
    {
        return m_input;
    }

    // Return the output.
    const std::string &DriverOptions::output() const
    {
        return m_output;
    }

    // Return the -D constants.
    const std::vector<Define> &DriverOptions::defines() const
    {
        return m_defines;
    }

    // Report --help.
    bool DriverOptions::wants_help() const
    {
        return m_wants_help;
    }

    // Report --version.
    bool DriverOptions::wants_version() const
    {
        return m_wants_version;
    }

    // Report --dump-tokens.
    bool DriverOptions::dumps_tokens() const
    {
        return m_dumps_tokens;
    }

    // Report whether --no-runtime was left out.
    bool DriverOptions::writes_runtime() const
    {
        return m_writes_runtime;
    }

    // Report whether -w was left out.
    bool DriverOptions::warnings_enabled() const
    {
        return m_warnings_enabled;
    }

    // Report -Werror.
    bool DriverOptions::warnings_as_errors() const
    {
        return m_warnings_as_errors;
    }

    // Split NAME=VALUE, requiring both halves.
    bool DriverOptions::add_define(std::string_view text, std::string &error)
    {
        size_t equals = text.find('=');
        if (equals == std::string_view::npos || equals == 0 || equals + 1 == text.size()) {
            error = "-D takes NAME=VALUE, not '" + std::string(text) + "'";
            return false;
        }
        Define define;
        define.m_name = text.substr(0, equals);
        define.m_value = text.substr(equals + 1);
        m_defines.push_back(define);
        return true;
    }

}
