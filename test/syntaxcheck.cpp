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

#include "utap/utap.hpp"

#include <iostream>
#include <string>
#include <vector>

using UTAP::Document;

int main(int argc, char* argv[])
{
    using namespace std::literals::string_literals;
    try {
        if (argc < 2 || argc > 3) {
            std::cerr << "Synopsis: check [-b] <filename>" << std::endl;
            return 1;
        }
        auto old = ("-b"s == argv[1]);

        auto doc = Document{};
        auto name = std::string{argv[argc - 1]};

        if (name.substr(name.length() - 4) == ".xml") {
            parse_XML_file(name.c_str(), doc, !old);
        } else {
            FILE* file = fopen(name.c_str(), "r");
            if (file == nullptr) {
                perror(name.c_str());
                return 1;
            }
            parse_XTA(file, doc, !old);
            fclose(file);
        }
        for (const auto& err : doc.get_errors())
            std::cerr << err << std::endl;
        for (const auto& warn : doc.get_warnings())
            std::cerr << warn << std::endl;
        return doc.get_errors().empty() && doc.get_warnings().empty() ? 0 : 2;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 3;
    }
}
