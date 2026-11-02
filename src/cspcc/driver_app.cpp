/*
 * C++ file for the cspcc driver.
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

// Project headers for the stages of a translation.
#include "cspcc/driver_app.h"
#include "cspcc/ast_printer.h"
#include "cspcc/diagnostics_engine.h"
#include "cspcc/lexer_tokenizer.h"
#include "cspcc/parser_grammar.h"

// Standard headers for files and streams.
#include <fstream>
#include <iostream>
#include <sstream>

// Namespace for the cspcc driver.
namespace cspcc
{

    // Keep the program's name and its arguments.
    DriverApp::DriverApp(int argc, const char *const *argv)
        : m_program(argc > 0 ? argv[0] : "cspcc")
        , m_args(argv + (argc > 0 ? 1 : 0), argv + argc)
    {
        // Empty.
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
        return diagnostics.error_count() ? 1 : 0;
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

}
