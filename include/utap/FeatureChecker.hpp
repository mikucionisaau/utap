// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020-2024 Aalborg University.
   Copyright (C) 2002-2006 Uppsala University and Aalborg University.

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

#ifndef UTAP_FEATURECHECKER_HH
#define UTAP_FEATURECHECKER_HH

#include "utap/document.hpp"
#include "utap/expression.hpp"
#include "utap/statement.hpp"

namespace UTAP {
class FeatureChecker : public DocumentVisitor, public AbstractStatementVisitor
{
private:
    SupportedMethods supported_methods{};

public:
    explicit FeatureChecker(Document& document);
    SupportedMethods get_supported_methods() { return supported_methods; }

    void visit_edge(Edge& edge) override;
    void visit_assignment(Expression& ass);
    void visit_guard(Expression& guard);
    void visit_location(Location& state) override;
    void visit_variable(Variable&) override;
    bool visit_template_before(Template&) override;

    void visit_frame(const Frame& frame);
    bool is_rate_disallowed_in_symbolic(const Expression& e);
};

}  // namespace UTAP

#endif /* UTAP_FEATURECHECKER_HH */
