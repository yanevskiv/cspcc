// (c) 2026 Ivan Janevski, GPLv3 or later; see LICENSE.md

#include "cspcc/driver_app.h"
#include "cspcc/ast_printer.h"
#include "cspcc/diagnostics_engine.h"
#include "cspcc/emit_ada_writer.h"
#include "cspcc/emit_runtime.h"
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"
#include "cspcc/sema_checker.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

// Version of cspcc.
#define CSPCC_VERSION "0.2.0"

namespace cspcc
{

    // Keep the program's name and its arguments.
    DriverApp::DriverApp(int argc, const char *const *argv)
        : m_program(argc > 0 ? argv[0] : "cspcc")
        , m_args(argv + (argc > 0 ? 1 : 0), argv + argc)
    {
    }

    // Parse the options, then read and translate the input.
    int DriverApp::run()
    {
        std::string error;
        if (!m_options.parse(m_args, error)) {
            std::cerr << "cspcc: " << error << '\n';
            return 2;
        }
        if (m_options.wants_help()) {
            DriverOptions::print_help(std::cout, m_program);
            return 0;
        }
        if (m_options.wants_version()) {
            std::cout << "cspcc " << CSPCC_VERSION << '\n';
            return 0;
        }
        if (m_options.input().empty()) {
            DriverOptions::print_help(std::cerr, m_program);
            return 2;
        }
        std::string source;
        if (!read_file(m_options.input(), source)) {
            std::cerr << "cspcc: can't read '" << m_options.input() << "'\n";
            return 1;
        }
        return translate(source);
    }

    // Run the stages, stopping after the first that reports errors.
    int DriverApp::translate(const std::string &source)
    {
        DiagnosticsEngine diagnostics(m_options.input(), source, std::cerr);
        diagnostics.set_warnings(m_options.warnings_enabled(), m_options.warnings_as_errors());
        LexerTokenizer tokenizer(source, diagnostics);
        std::vector<data::DataToken> tokens = tokenizer.tokenize();
        if (m_options.dumps_tokens()) {
            LexerTokenizer::dump(tokens, std::cout);
            return diagnostics.error_count() ? 1 : 0;
        }
        if (diagnostics.error_count()) {
            return 1;
        }
        ParserGrammar parser(tokens, diagnostics);
        std::unique_ptr<data::DataAst> ast = parser.parse_program();
        if (!ast) {
            return 1;
        }
        if (m_options.dumps_ast()) {
            AstPrinter(std::cout).print(*ast);
            return 0;
        }
        SemaChecker checker(diagnostics);
        std::unique_ptr<data::DataProgram> program = checker.check(*ast, m_options.defines());
        if (!program) {
            return 1;
        }
        return emit(*program);
    }

    // Name the output after the input unless -o names it, then write the files.
    int DriverApp::emit(data::DataProgram &program)
    {
        std::filesystem::path input(m_options.input());
        std::filesystem::path output(m_options.output());
        if (output.empty()) {
            output = EmitAdaWriter::unit_from(input.stem().string()) + ".adb";
        }
        std::string unit = output.stem().string();
        if (output.extension() != ".adb" || !EmitAdaWriter::valid_unit(unit)) {
            std::cerr << "cspcc: the output must be a lower-case Ada unit name ending in .adb, "
                         "such as philosophers.adb, not '"
                      << output.string() << "'\n";
            return 2;
        }
        EmitAdaWriter writer(program, ada_name(unit), input.filename().string());
        if (!write_file(output, writer.write())) {
            std::cerr << "cspcc: can't write '" << output.string() << "'\n";
            return 1;
        }
        if (!m_options.writes_runtime()) {
            return 0;
        }
        std::filesystem::path directory = output.parent_path();
        if (!write_file(directory / "csp_runtime.ads", EmitRuntime::spec()) ||
            !write_file(directory / "csp_runtime.adb", EmitRuntime::body())) {
            std::cerr << "cspcc: can't write the runtime next to '" << output.string() << "'\n";
            return 1;
        }
        return 0;
    }

    // Read a whole file.
    bool DriverApp::read_file(const std::string &path, std::string &text)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        text = buffer.str();
        return true;
    }

    // Write a file unless it already holds the text, so gnatmake keeps its objects.
    bool DriverApp::write_file(const std::filesystem::path &path, std::string_view text)
    {
        std::string old;
        if (read_file(path.string(), old) && old == text) {
            return true;
        }
        std::ofstream file(path, std::ios::binary);
        file << text;
        return static_cast<bool>(file);
    }

    // Capitalize the first letter and each letter after an underscore.
    std::string DriverApp::ada_name(std::string_view unit)
    {
        std::string name(unit);
        for (size_t i = 0; i < name.size(); i++) {
            if (i == 0 || name[i - 1] == '_') {
                name[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[i])));
            }
        }
        return name;
    }

}
