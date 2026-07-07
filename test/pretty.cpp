// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002 Uppsala University and Aalborg University.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#include "utap/PrettyPrinter.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

using namespace std;
using namespace UTAP::Constants;

static bool newSyntax = (getenv("UPPAAL_OLD_SYNTAX") == nullptr);

/**
 * Test for pretty printer
 */
int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " MODEL\n\n";
            std::cerr << "where MODEL is a UPPAAL .xml, xta, or .ta file\n";
            return 1;
        }
        auto path = std::filesystem::path{argv[1]};
        if (!std::filesystem::exists(path))
            throw std::runtime_error("File does not exist: " + path.string());
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("Path is not a regular file: " + path.string());

        auto pretty = UTAP::PrettyPrinter{cout};

        if (path.extension() == ".xml") {
            parse_XML_file(path, pretty, newSyntax);
        } else {
            auto file_path = path.string();
            if (FILE* file = fopen(file_path.c_str(), "r"); file != nullptr) {
                parse_XTA(file, pretty, newSyntax);
            } else {
                std::perror(file_path.c_str());
                std::exit(EXIT_FAILURE);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Caught unknown exception." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return 0;
}
