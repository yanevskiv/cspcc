// The command-line options of cspcc.
// Copyright (C) 2026 Ivan Janevski
// SPDX-License-Identifier: GPL-3.0-or-later

#include "options.h"

// Parse the arguments; an option's value may be joined (-oFILE) or split (-o FILE).
bool Options::parse(std::span<const char *const> args, std::string &error)
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

// Print what cspcc does, its options, and how to build and run what it writes.
void Options::print_help(std::ostream &stream, std::string_view program)
{
    stream
        << "Usage: " << program << " [options] INPUT\n"
        << "\n"
        << "Translate INPUT, a program in Hoare's CSP (1978), into Ada. Each process\n"
        << "becomes an Ada task. cspcc writes the main procedure (NAME.adb) and the\n"
        << "runtime it uses (csp_runtime.ads and csp_runtime.adb) into the current\n"
        << "directory; GNAT builds them into a program that runs the processes.\n"
        << "\n"
        << "Example:\n"
        << "  cspcc philosophers.csp         writes philosophers.adb and the runtime\n"
        << "  gnatmake philosophers.adb      builds ./philosophers\n"
        << "  ./philosophers --timeout=2     runs it for two seconds\n"
        << "\n"
        << "Options:\n"
        << "  -o FILE.adb      write the main procedure to FILE.adb, and the runtime next\n"
        << "                   to it; FILE must be a lower-case Ada name, such as\n"
        << "                   philosophers (default: INPUT's name in lower case,\n"
        << "                   with _ for other characters)\n"
        << "  -D NAME=VALUE    give the constant NAME the value VALUE, also when INPUT\n"
        << "                   defines it with NAME :: 5 or const NAME = 5\n"
        << "  --no-runtime     write only the main procedure, for when the runtime is\n"
        << "                   already there\n"
        << "  --dump-tokens    print INPUT's tokens, one per line with its position,\n"
        << "                   and stop\n"
        << "  -w               don't report warnings\n"
        << "  -Werror          report warnings as errors\n"
        << "  -h, --help       show this help\n"
        << "  --version        show cspcc's version\n"
        << "\n"
        << "The translated program takes:\n"
        << "  --trace          print every message as it passes between two processes\n"
        << "  --quiet          don't print hooks\n"
        << "  --delay=MS       pause each hook for up to MS milliseconds (default 10)\n"
        << "  --seed=N         seed the random choices, to repeat a run\n"
        << "  --timeout=S      stop after S seconds\n"
        << "A hook is a command that nothing defines, such as EAT or work(): it prints\n"
        << "the process and the command, then pauses for a moment.\n"
        << "\n"
        << "Exit status:\n"
        << "  cspcc            0 when INPUT is translated, 1 on errors in INPUT, 2 on bad\n"
        << "                   options\n"
        << "  the program      0 when every process ends or at --timeout, 1 when a\n"
        << "                   process fails (such as an alternative command with no\n"
        << "                   true guard), 2 on bad options, 3 on a deadlock, after\n"
        << "                   listing what each process waits for\n";
}

// Return the input.
const std::string &Options::input() const
{
    return m_input;
}

// Return the output.
const std::string &Options::output() const
{
    return m_output;
}

// Return the -D constants.
const std::vector<Define> &Options::defines() const
{
    return m_defines;
}

// Report --help.
bool Options::wants_help() const
{
    return m_wants_help;
}

// Report --version.
bool Options::wants_version() const
{
    return m_wants_version;
}

// Report --dump-tokens.
bool Options::dumps_tokens() const
{
    return m_dumps_tokens;
}

// Report whether --no-runtime was left out.
bool Options::writes_runtime() const
{
    return m_writes_runtime;
}

// Report whether -w was left out.
bool Options::warnings_enabled() const
{
    return m_warnings_enabled;
}

// Report -Werror.
bool Options::warnings_as_errors() const
{
    return m_warnings_as_errors;
}

// Split NAME=VALUE, requiring both halves.
bool Options::add_define(std::string_view text, std::string &error)
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
