// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
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

#include "utap/document.hpp"

#include "utap/builder.hpp"
#include "utap/statement.hpp"

#include <functional>  // std::mem_fn
#include <iostream>
#include <sstream>
#include <stack>
#include <utility>  // declval

#include <cassert>

#ifdef __MINGW32__
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

using namespace UTAP;
using namespace Constants;

template <typename Item>
std::string Stringify<Item>::str() const
{
    auto os = std::ostringstream{};
    static_cast<const Item*>(this)->print(os);
    return os.str();
}

template <typename Item>
std::string StringifyIndent<Item>::str(const std::string& indent) const
{
    auto os = std::ostringstream{};
    static_cast<const Item*>(this)->print(os, indent);
    return os.str();
}

namespace UTAP {  // Explicit instantiations to generate implementation
template struct Stringify<ChanPriority>;
template struct Stringify<Variable>;
template struct Stringify<Location>;
template struct Stringify<Edge>;
template struct Stringify<Function>;
template struct Stringify<Declarations>;
template struct Stringify<LSCSimRegion>;
template struct Stringify<LSCCut>;
}  // namespace UTAP

std::ostream& Location::print(std::ostream& os) const
{
    os << "LOCATION (" << uid.get_name() << ", ";
    invariant.print(os) << ", ";
    exp_rate.print(os) << ')';
    return os;
}

std::ostream& Edge::print(std::ostream& os) const
{
    os << "EDGE (";
    src->print(os) << ' ';
    dst->print(os) << ")\n";
    os << "\t";
    guard.print(os) << ", ";
    sync.print(os) << ", ";
    assign.print(os);
    return os;
}

std::ostream& Function::print(std::ostream& os) const
{
    auto type = uid.get_type();                                         // the function type/signature
    type.get(0).print(os) << ' '                                        // return type
                          << uid.get_name() << '(';                     // function name
    if (type.size() > 1) {                                              // has arguments
        type.get(1).print_declaration(os) << ' ' << type.get_label(1);  // first parameter type and name
        for (uint32_t i = 2; i < type.size(); ++i)                      // remaining parameters
            type.get(i).print_declaration(os << ", ") << ' ' << type.get_label(i);
    }
    os << ")\n{\n";  // open function body
    for (const auto& variable : variables)
        variable.print(os << "    ") << ";\n";
    body->print(os, INDENT);
    return os << "}";
}

std::ostream& Variable::print(std::ostream& os) const
{
    std::string type = uid.get_type().declaration();
    if (uid.get_type().is_array()) {
        auto i = type.find('[');
        assert(i != std::string::npos);
        os << type.substr(0, i) << ' ' << uid.get_name() << type.substr(i, type.length() - i);
    } else {
        os << type << " " << uid.get_name();
    }
    if (!init.empty())
        init.print(os << " = ");
    return os;
}

bool Declarations::add_function(Type type, std::string_view name, position_t pos, Function*& fun)
{
    bool duplicate = frame.contains(name);
    fun = &functions.emplace_back();
    fun->uid = frame.add_symbol(name, std::move(type), pos, fun);  // Add symbol
    return !duplicate;
}

std::ostream& Declarations::print(std::ostream& os, bool global) const
{
    print_constants(os) << "\n";
    print_typedefs(os) << "\n";
    print_variables(os, global) << "\n";
    print_functions(os);
    return os;
}

std::string Declarations::str(bool global) const
{
    auto os = std::ostringstream{};
    print(os, global);
    return os.str();
}

std::ostream& Declarations::print_constants(std::ostream& os) const
{
    if (!variables.empty()) {
        bool first = true;
        for (const auto& variable : variables) {
            if (variable.uid.get_type().get_kind() == CONSTANT) {
                if (first) {
                    os << "// constants\n";
                    first = false;
                }
                variable.print(os) << ";\n";
            }
        }
    }
    return os;
}

std::ostream& Declarations::print_typedefs(std::ostream& os) const
{
    bool first = true;
    for (const auto& symbol : frame) {
        if (symbol.get_type().get_kind() == TYPEDEF) {
            if (first) {
                os << "// type definitions\n";
                first = false;
            }
            symbol.get_type().print_declaration(os) << ";\n";
        }
    }
    return os;
}

std::ostream& Declarations::print_variables(std::ostream& os, bool global) const
{
    if (!variables.empty()) {
        os << "// variables\n";
        for (const auto& var : variables)
            if (var.uid.get_type().get_kind() != CONSTANT)
                var.print(os) << ";\n";
    }
    return os;
}

std::ostream& Declarations::print_functions(std::ostream& os) const
{
    if (!functions.empty()) {
        os << "// functions\n";
        for (const auto& fun : functions)
            fun.print(os) << "\n\n";
    }
    return os;
}

std::ostream& Instance::print_mapping(std::ostream& os) const
{
    for (const auto& [symbol, expr] : mapping)
        os << symbol.get_name() << " = " << expr.str() << "\n";
    return os;
}

std::ostream& Instance::print_parameters(std::ostream& os) const
{
    auto b = std::begin(parameters), e = std::end(parameters);
    if (b != e) {
        b->get_type().print_declaration(os) << " " << b->get_name();
        while (++b != e)
            b->get_type().print_declaration(os << ", ") << " " << b->get_name();
    }
    return os;
}

std::ostream& Instance::print_arguments(std::ostream& os) const
{
    auto b = std::begin(parameters), e = std::end(parameters);
    if (b != e) {
        auto itr = mapping.find(*b);
        assert(itr != std::end(mapping));
        itr->second.print(os);
        while (++b != e) {
            itr = mapping.find(*b);
            assert(itr != std::end(mapping));
            itr->second.print(os << ", ");
        }
    }
    return os;
}

std::string Instance::mapping_str() const
{
    auto os = std::ostringstream{};
    print_mapping(os);
    return os.str();
}

std::string Instance::parameters_str() const
{
    auto os = std::ostringstream{};
    print_parameters(os);
    return os.str();
}

std::string Instance::arguments_str() const
{
    auto os = std::ostringstream{};
    print_arguments(os);
    return os.str();
}

Location& Template::add_location(std::string_view name, Expression inv, Expression er, position_t pos)
{
    bool duplicate = frame.contains(name);
    auto& loc = locations.emplace_back();
    loc.uid = frame.add_symbol(name, Type::create_primitive(LOCATION), pos, &loc);
    loc.nr = static_cast<int32_t>(locations.size() - 1);
    loc.invariant = std::move(inv);
    loc.exp_rate = std::move(er);
    if (duplicate)
        throw duplicate_definition_error(name);
    return loc;
}

// FIXME: like for unnamed locations, a name is autegenerated
// this name may conflict with user-defined names
Branchpoint& Template::add_branchpoint(std::string_view name, position_t pos)
{
    bool duplicate = frame.contains(name);
    auto& branchpoint = branchpoints.emplace_back();
    branchpoint.uid = frame.add_symbol(name, Type::create_primitive(BRANCHPOINT), pos, &branchpoint);
    branchpoint.bpNr = static_cast<int32_t>(branchpoints.size() - 1);
    if (duplicate)
        throw duplicate_definition_error(name);
    return branchpoint;
}

Edge& Template::add_edge(Symbol src, Symbol dst, bool control, std::string_view actname)
{
    int32_t nr = edges.empty() ? 0 : edges.back().nr + 1;
    Edge& edge = edges.emplace_back();
    if (src.get_type().is_location()) {
        edge.src = static_cast<Location*>(src.get_data());
        edge.srcb = nullptr;
    } else {
        edge.src = nullptr;
        edge.srcb = static_cast<Branchpoint*>(src.get_data());
    }
    if (dst.get_type().is_location()) {
        edge.dst = static_cast<Location*>(dst.get_data());
        edge.dstb = nullptr;
    } else {
        edge.dst = nullptr;
        edge.dstb = static_cast<Branchpoint*>(dst.get_data());
    }

    edge.control = control;
    edge.actname = actname;
    edge.nr = nr;
    return edge;
}

// LSC
LSCInstanceLine& Template::add_instance_line()
{
    // bool duplicate = frame.get_index_of(name) != -1;

    LSCInstanceLine& instance = instances.emplace_back();
    // instance.uid = frame.add_symbol(name, Type::create_primitive(INSTANCELINE), &instance);
    instance.instance_nr = static_cast<int32_t>(instances.size() - 1);

    //    if (duplicate)
    //    {
    //        throw TypeException(boost::format("$Duplicate_definition_of %1%") % name);
    //    }

    return instance;
}

LSCMessage& Template::add_message(Symbol src, Symbol dst, int loc, bool pch)
{
    int32_t nr = messages.empty() ? 0 : messages.back().nr + 1;
    auto& message = messages.emplace_back(nr);
    message.src = static_cast<LSCInstanceLine*>(src.get_data());
    message.dst = static_cast<LSCInstanceLine*>(dst.get_data());
    message.location = loc;
    message.is_in_prechart = pch;
    return message;
}

LSCUpdate& Template::add_update(Symbol anchor, int loc, bool pch)
{
    int32_t nr = updates.empty() ? 0 : updates.back().nr + 1;
    auto& update = updates.emplace_back(nr);
    update.anchor = static_cast<LSCInstanceLine*>(anchor.get_data());
    update.location = loc;
    update.is_in_prechart = pch;
    return update;
}

LSCCondition& Template::add_condition(std::vector<Symbol> anchors, int loc, bool pch, bool isHot)
{
    int32_t nr = conditions.empty() ? 0 : conditions.back().nr + 1;
    auto& condition = conditions.emplace_back(nr);

    for (auto& anchor : anchors) {
        condition.anchors.push_back(static_cast<LSCInstanceLine*>(anchor.get_data()));  // TODO
    }
    condition.location = loc;
    condition.is_in_prechart = pch;
    condition.isHot = isHot;
    return condition;
}

template <typename Fn, typename Element, typename Res = std::invoke_result_t<Fn, Element>>
std::deque<Res> collect(Fn&& fn, const std::deque<Element>& elements)
{
    auto res = std::deque<Res>{};
    std::transform(std::begin(elements), std::end(elements), std::back_inserter(res), std::forward<Fn>(fn));
    return res;
}

/**
 * returns the simregions of an LSC scenario.
 * A simregion is a simultaneous region containing
 * 1 or 0 message,
 * 1 or 0 update and
 * 1 or 0 condition,
 * at the same location.
 * a message, update or condition must be in only one simregion.
 */
std::vector<LSCSimRegion> Template::get_simregions()
{
    // cout <<"=======LSC: get_simregions=======\n";
    // Copy the numbers of messages, conditions and updates from the scenario
    auto m_nr = collect(std::mem_fn(&LSCMessage::get_nr), messages);
    auto c_nr = collect(std::mem_fn(&LSCCondition::get_nr), conditions);
    auto u_nr = collect(std::mem_fn(&LSCUpdate::get_nr), updates);

    auto simregions = std::vector<LSCSimRegion>{};
    simregions.reserve(m_nr.size());

    /// iterate over messages
    for (auto& message_nr : m_nr) {
        LSCSimRegion s = LSCSimRegion();
        s.set_message(messages, message_nr);

        LSCInstanceLine* source = s.message->src;
        LSCInstanceLine* target = s.message->dst;
        int y = s.message->location;
        /**
         * we give priority to the condition on the target, if there is also one
         * in the source, it must be part of another simregion
         */
        if (get_condition(*target, y, s.condition)) {
            for (auto c_itr = c_nr.begin(); c_itr != c_nr.end(); ++c_itr) {
                if (*c_itr == s.condition->nr) {
                    c_nr.erase(c_itr);
                    break;
                }
            }
        } else if (get_condition(*source, y, s.condition)) {
            for (auto c_itr = c_nr.begin(); c_itr != c_nr.end(); ++c_itr) {
                if (*c_itr == s.condition->nr) {
                    c_nr.erase(c_itr);
                    break;
                }
            }
        }
        if (get_update(*target, y, s.update)) {
            for (auto u_itr = u_nr.begin(); u_itr != u_nr.end(); ++u_itr) {
                if (*u_itr == s.update->nr) {
                    u_nr.erase(u_itr);
                    break;
                }
            }
        } else if (get_update(*source, y, s.update)) {
            for (auto u_itr = u_nr.begin(); u_itr != u_nr.end(); ++u_itr) {
                if (*u_itr == s.update->nr) {
                    u_nr.erase(u_itr);
                    break;
                }
            }
        }
        s.nr = static_cast<uint32_t>(simregions.size());
        simregions.push_back(s);
    }

    /// iterate over remaining conditions
    for (auto& c_itr : c_nr) {
        auto s = LSCSimRegion{};
        s.set_condition(conditions, c_itr);

        int y = s.condition->location;
        if (get_update(s.condition->anchors, y, s.update)) {
            for (auto u_itr = u_nr.begin(); u_itr != u_nr.end(); ++u_itr) {
                if (*u_itr == s.update->nr) {
                    u_nr.erase(u_itr);
                    break;
                }
            }
        }
        s.nr = static_cast<uint32_t>(simregions.size());
        simregions.push_back(s);
    }

    /// iterate over remaining updates
    for (auto& u_itr : u_nr) {
        auto s = LSCSimRegion();
        s.set_update(updates, u_itr);
        s.nr = static_cast<uint32_t>(simregions.size());
        simregions.push_back(s);
    }

    /*// cout << "-----unordered simregions-----\n";
    for (unsigned int i = 0; i < simregions.size(); ++i){
        // cout << simregions[i].str() << " " << simregions[i].nr<< "\n";
    } //test OK*/

    return simregions;
}

/**
 * gets the condition on the given instance, at y location,
 * returns false if there isn't any
 */
bool Template::get_condition(LSCInstanceLine& instance, int y, LSCCondition*& simCondition)
{
    for (auto& condition : conditions) {
        if (condition.location == y) {
            for (auto& anchor : condition.anchors) {
                LSCInstanceLine* instancej = anchor;
                if (instancej->instance_nr == instance.instance_nr) {
                    simCondition = &condition;
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * gets the update on the given instance at y location,
 * returns false if there isn't any
 */
bool Template::get_update(LSCInstanceLine& instance, int y, LSCUpdate*& simUpdate)
{
    for (auto& update : updates) {
        if (update.location == y) {
            if (update.anchor->instance_nr == instance.instance_nr) {
                simUpdate = &update;
                return true;
            }
        }
    }
    return false;
}

/**
 * gets the first update on one of the given instances, at y location
 * (in simUpdate), returns false if there isn't any
 */
bool Template::get_update(std::vector<LSCInstanceLine*>& instances, int y, LSCUpdate*& simUpdate)
{
    for (auto& instance : instances) {
        if (get_update(*instance, y, simUpdate))
            return true;
    }
    return false;
}

void LSCInstanceLine::add_parameters(Instance& inst, Frame params, const std::vector<Expression>& arguments1)
{
    unbound = params.get_size();
    parameters = std::move(params);
    parameters.add(inst.parameters);
    mapping = inst.mapping;
    arguments = static_cast<uint32_t>(arguments1.size());
    templ = inst.templ;
    for (auto i = 0u; i < arguments1.size(); ++i)
        mapping[inst.parameters[i]] = arguments1[i];
}
/**
 * return the simregions anchored to this instance,
 * ordered by location number
 */
std::vector<LSCSimRegion> LSCInstanceLine::get_simregions(const std::vector<LSCSimRegion>& simregions)
{
    auto i_simregions = std::vector<LSCSimRegion>{};
    // get the simregions anchored to this instance
    for (const auto& reg : simregions) {
        const LSCMessage* m = reg.message;
        if ((m->src->instance_nr == this->instance_nr || m->dst->instance_nr == this->instance_nr)) {
            i_simregions.push_back(reg);
            continue;
        }

        const LSCUpdate* u = reg.update;
        if (u->anchor->instance_nr == this->instance_nr) {
            i_simregions.push_back(reg);
            continue;
        }

        const LSCCondition* c = reg.condition;
        for (auto* instance : c->anchors) {
            if (instance->instance_nr == this->instance_nr) {
                i_simregions.push_back(reg);
                break;
            }
        }
    }
    // ordering the simregions by location number
    sort(i_simregions.begin(), i_simregions.end(), LSCSimRegionComparator());

    //    std::cout << "--------instance--------\n";
    //    for (auto& reg : i_simregions) {
    //        std::cout << reg.str() << " " << reg.getLoc() << " " << reg.is_in_prechart()<<"\n";
    //    } //test OK

    return i_simregions;
}

int LSCSimRegion::get_loc() const
{
    if (has_message())
        return message->location;
    if (has_condition())
        return condition->location;
    if (has_update())
        return update->location;
    assert(false);
    return -1;  // should not happen
}

bool LSCSimRegion::is_in_prechart() const
{
    if (has_message())
        return message->is_in_prechart;
    if (has_condition())
        return condition->is_in_prechart;
    if (has_update())
        return update->is_in_prechart;
    assert(false);
    return false;  // should not happen
}

void LSCSimRegion::set_message(std::deque<LSCMessage>& messages, uint32_t nr)
{
    for (auto& message : messages) {
        if (message.nr == nr) {
            this->message = &message;
            return;
        }
    }
}

void LSCSimRegion::set_condition(std::deque<LSCCondition>& conditions, uint32_t nr)
{
    for (auto& condition : conditions) {
        if (condition.nr == nr) {
            this->condition = &condition;
            return;
        }
    }
}

void LSCSimRegion::set_update(std::deque<LSCUpdate>& updates, uint32_t nr)
{
    for (auto& update : updates) {
        if (update.nr == nr) {
            this->update = &update;
            return;
        }
    }
}

std::ostream& LSCSimRegion::print(std::ostream& os) const
{
    os << "s(";
    auto need_sep = false;
    if (has_message()) {
        message->label.print(os << "m:");
        need_sep = true;
    }
    if (has_condition()) {
        if (need_sep)
            os << " ";
        else
            need_sep = true;
        os << "c:";
        if (condition->isHot)
            os << " HOT ";
        condition->label.print(os);
    }
    if (has_update()) {
        if (need_sep)
            os << " ";
        update->label.print(os << "u:");
    }
    return os << ")";
}

inline auto find_simregion_by_nr(uint32_t nr)
{
    return [nr](const LSCSimRegion& reg) { return reg.nr == nr; };
}

void LSCCut::erase(const LSCSimRegion& s)
{
    simregions.erase(std::remove_if(simregions.begin(), simregions.end(), find_simregion_by_nr(s.nr)),
                     simregions.end());
}
bool LSCCut::contains(const LSCSimRegion& s) const
{
    return std::find_if(simregions.begin(), simregions.end(), find_simregion_by_nr(s.nr)) != simregions.end();
}

/**
 * returns true if the cut is in the prechart,
 * given one of the following simregions.
 * if one of the following simregions is not in the prechart,
 * then all following simregions aren't in the prechart (because of the
 * construction of the partial order),
 * and the cut is not in the prechart (but can contain only simregions
 * that are in the prechart, if it is the limit between the prechart
 * and the mainchart)
 */
bool LSCCut::is_in_prechart(const LSCSimRegion& fSimregion) const
{
    if (!is_in_prechart())
        return false;
    if (!fSimregion.is_in_prechart())
        return false;
    return true;
}

bool LSCCut::is_in_prechart() const
{
    return std::all_of(simregions.begin(), simregions.end(), [](auto& sr) { return sr.is_in_prechart(); });
}

bool LSCCut::equals(const LSCCut& y) const
{
    if (simregions.size() != y.simregions.size())
        return false;
    auto ycopy = y.simregions;
    for (const auto& s : simregions)
        ycopy.erase(std::remove_if(ycopy.begin(), ycopy.end(), find_simregion_by_nr(s.nr)), ycopy.end());
    return ycopy.empty();
}

std::ostream& LSCCut::print(std::ostream& os) const
{
    os << "CUT(";
    if (auto b = simregions.begin(), e = simregions.end(); b != e) {
        b->print(os);
        while (++b != e)
            b->print(os << " ");
    }
    return os << ")";
}

/**
 * return true if the LSC is of invariant mode
 */
bool Template::is_invariant() const
{
    if (is_TA)
        return false;
    return mode == "invariant";
}

std::ostream& ChanPriority::print(std::ostream& os) const
{
    os << "chan priority ";
    auto head_s = head.str();
    if (head_s.empty())
        head_s = "default";

    os << head_s;
    for (const auto& [ch, expr] : tail) {
        if (ch == '<')
            os << ' ';
        os << ch << ' ';
        expr.print(os);
    }
    return os;
}

Document::Document()
{
    global.frame = Frame::make().make_sub();
    system_declarations.frame = global.frame.make_sub();
#ifdef ENABLE_CORA
    addVariable(&global, Type::create_primitive(COST), "cost", Expression());
#endif
}

void Document::add(Library&& lib) { libraries.push_back(std::move(lib)); }

Library& Document::last_library()
{
    if (libraries.empty())
        throw std::runtime_error("$No_library_loaded");
    return libraries.back();
}

/** Creates and returns a new template. The template is created with
 *  the given name and parameters and added to the global frame. The
 *  method does not check for duplicate declarations. An instance with
 *  the same name and parameters is added as well.
 */
Template& Document::add_template(std::string_view name, const Frame& params, position_t position, const bool is_TA,
                                 std::string_view typeLSC, std::string_view mode)
{
    Type type = (is_TA) ? Type::create_instance(params) : Type::create_LSC_instance(params);
    Template& templ = templates.emplace_back();
    templ.parameters = params;
    templ.frame = global.frame.make_sub();
    templ.frame.add(params);
    templ.templ = &templ;
    templ.uid = global.frame.add_symbol(name, type, position, (Instance*)&templ);
    templ.arguments = 0;
    templ.unbound = params.get_size();
    templ.is_TA = is_TA;
    templ.dynamic = false;
    // LSC
    templ.type = typeLSC;
    templ.mode = mode;
    return templ;
}

Template& Document::add_dynamic_template(std::string_view name, const Frame& params, position_t pos)
{
    Type type = Type::create_instance(params);
    dyn_templates.emplace_back();
    Template& templ = dyn_templates.back();
    templ.parameters = params;
    templ.frame = global.frame.make_sub();
    templ.frame.add(params);
    templ.templ = &templ;
    templ.uid = global.frame.add_symbol(name, type, pos, (Instance*)&templ);
    templ.arguments = 0;
    templ.unbound = params.get_size();
    templ.is_TA = true;
    templ.dynamic = true;
    templ.dyn_index = static_cast<int>(dyn_templates.size() - 1);
    templ.is_defined = false;
    return templ;
}

std::vector<Template*>& Document::get_dynamic_templates()
{
    if (dyn_templates_vec.size() != dyn_templates.size()) {
        dyn_templates_vec.clear();
        dyn_templates_vec.reserve(dyn_templates.size());
        for (auto&& t : dyn_templates)
            dyn_templates_vec.push_back(&t);
    }
    return dyn_templates_vec;
}

inline auto equal_name(std::string_view name)
{
    return [name](const auto& e) { return (e.uid.get_name() == name); };
}

const Template* Document::find_template(std::string_view name) const
{
    auto has_name = equal_name(name);
    auto it = std::find_if(templates.begin(), templates.end(), has_name);
    if (it == templates.end()) {
        it = std::find_if(std::begin(dyn_templates), std::end(dyn_templates), has_name);
        if (it == std::end(dyn_templates))
            return nullptr;
    }
    return &(*it);
}

Template* Document::find_dynamic_template(std::string_view name)
{
    auto it = std::find_if(dyn_templates.begin(), dyn_templates.end(), equal_name(name));
    if (it == std::end(dyn_templates))
        return nullptr;
    return &(*it);
}

Instance& Document::add_instance(std::string_view name, Instance& inst, Frame params,
                                 const std::vector<Expression>& arguments, position_t pos)
{
    Type type = Type::create_instance(params);
    Instance& instance = instances.emplace_back();
    instance.uid = global.frame.add_symbol(name, type, pos, &instance);
    instance.unbound = params.get_size();
    instance.parameters = std::move(params);
    instance.parameters.add(inst.parameters);
    instance.mapping = inst.mapping;
    instance.arguments = static_cast<uint32_t>(arguments.size());
    instance.templ = inst.templ;
    for (auto i = 0u; i < arguments.size(); ++i)
        instance.mapping[inst.parameters[i]] = arguments[i];
    return instance;
}

Instance& Document::add_LSC_instance(std::string_view name, Instance& inst, Frame params,
                                     const std::vector<Expression>& arguments, position_t pos)
{
    auto type = Type::create_LSC_instance(params);
    auto& instance = lsc_instances.emplace_back();
    instance.uid = global.frame.add_symbol(name, type, pos, &instance);
    instance.unbound = params.get_size();
    instance.parameters = std::move(params);
    instance.parameters.add(inst.parameters);
    instance.mapping = inst.mapping;
    instance.arguments = static_cast<uint32_t>(arguments.size());
    instance.templ = inst.templ;
    for (auto i = 0u; i < arguments.size(); ++i)
        instance.mapping[inst.parameters[i]] = arguments[i];
    return instance;
}

void Document::remove_process(Instance& instance)
{
    get_globals().frame.remove(instance.uid);
    for (auto itr = processes.cbegin(); itr != processes.cend(); ++itr) {
        if (itr->uid == instance.uid) {
            processes.erase(itr);
            break;
        }
    }
}

void Document::add_process(Instance& instance, position_t pos)
{
    Type type;
    Instance& process = processes.emplace_back(instance);
    if (process.unbound == 0)
        type = Type::create_process(process.templ->frame);
    else
        type = Type::create_process_set(instance.uid.get_type());
    process.uid = global.frame.add_symbol(instance.uid.get_name(), type, pos, &process);
}

bool Document::queries_empty() const { return queries.empty(); }

void Document::add_gantt(Declarations* context, GanttEntry g) { context->ganttChart.push_back(std::move(g)); }

void Document::add_query(Query query) { queries.push_back(std::move(query)); }

Options& Document::get_options() { return model_options; }

void Document::set_options(const Options& options) { model_options = options; }

// Add a regular variable
Variable* Document::add_variable(Declarations* context, Frame frame, Type type, std::string_view name, Expression initial, position_t pos)
{
    auto* var = add_variable(context->variables, std::move(frame), std::move(type), name, pos);
    var->init = std::move(initial);
    return var;
}

Variable* Document::add_variable_to_function(Function* function, Frame frame, Type type, std::string_view name,
                                             Expression initial, position_t pos)
{
    Variable* var = add_variable(function->variables, std::move(frame), std::move(type), name, pos);
    var->init = std::move(initial);
    return var;
}

// Add a regular variable
Variable* Document::add_variable(std::list<Variable>& variables, Frame frame, Type type, std::string_view name,
                                 position_t pos)
{
    bool duplicate = frame.contains(name);
    // Add variable
    Variable& var = variables.emplace_back();
    // Add symbol
    var.uid = frame.add_symbol(name, std::move(type), pos, &var);
    if (duplicate)
        throw duplicate_definition_error(name);
    return &var;
}

void Document::copy_variables_from_to(const Template* from, Template* to) const
{
    for (auto&& var : from->variables) {
        to->variables.push_back(var);
        to->frame.add(var.uid);
    }
}

void Document::copy_functions_from_to(const Template* from, Template* to) const
{
    // TODO to be implemented and to be used in Translator::lscProcBegin (see Translator.cpp)
}

void Document::add_progress_measure(Declarations* context, Expression guard, Expression measure)
{
    context->progress.emplace_back(std::move(guard), std::move(measure));
}

static void visit(DocumentVisitor& visitor, Frame frame)
{
    for (uint32_t i = 0; i < frame.get_size(); ++i) {
        Type type = frame[i].get_type();
        if (type.get_kind() == TYPEDEF) {
            visitor.visit_typedef(frame[i]);
            continue;
        }

        void* data = frame[i].get_data();
        type = type.strip_array();
        // TODO: use visitor dispatch to recover the type
        if ((type.is(Constants::INT) || type.is(Constants::STRING) || type.is(Constants::DOUBLE) ||
             type.is(Constants::BOOL) || type.is(CLOCK) || type.is(CHANNEL) || type.is(SCALAR) ||
             type.get_kind() == RECORD) &&
            data != nullptr)  // <--- ignore parameters
        {
            visitor.visit_variable(*static_cast<Variable*>(data));
        } else if (type.is(LOCATION)) {
            visitor.visit_location(*static_cast<Location*>(data));
        } else if (type.is(LOCATION_EXPR)) {
            visitor.visit_location(*static_cast<Location*>(data));
        } else if (type.is(FUNCTION)) {
            visitor.visit_function(*static_cast<Function*>(data));
        } else if (type.is(FUNCTION_EXTERNAL)) {
            // we cannot look inside a external function, skip.
        } else if (type.is(INSTANCE_LINE)) {
            visitor.visit_instance_line(*static_cast<LSCInstanceLine*>(data));
        }
    }
}

void visitTemplate(Template& t, DocumentVisitor& visitor)
{
    if (visitor.visit_template_before(t)) {
        visit(visitor, t.frame);
        for (auto& edge : t.edges)
            visitor.visit_edge(edge);
        for (auto& message : t.messages)
            visitor.visit_message(message);
        for (auto& update : t.updates)
            visitor.visit_update(update);
        for (auto& cond : t.conditions)
            visitor.visit_condition(cond);
        visitor.visit_template_after(t);
    }
}

void Document::accept(DocumentVisitor& visitor)
{
    visitor.visit_doc_before(*this);
    visit(visitor, global.frame.get_parent());
    visit(visitor, global.frame);
    visit(visitor, system_declarations.frame);
    for (auto& templ : templates)
        visitTemplate(templ, visitor);
    for (auto& templ : dyn_templates)
        visitTemplate(templ, visitor);

    for (uint32_t i = 0; i < global.frame.get_size(); ++i) {
        const Type type = global.frame[i].get_type().strip_array();
        void* data = global.frame[i].get_data();
        if (type.is(PROCESS) || type.is(PROCESS_SET)) {
            visitor.visit_process(*static_cast<Instance*>(data));
        } else if (type.is(INSTANCE)) {
            visitor.visit_instance(*static_cast<Instance*>(data));
        } else if (type.is(LSC_INSTANCE)) {
            visitor.visit_instance(*static_cast<Instance*>(data));
        }
    }

    for (auto&& decl : system_declarations.iodecl)
        visitor.visit_io_decl(decl);

    // Maybe not ideal place for this:
    for (auto&& progress : system_declarations.progress)
        visitor.visit_progress(progress);

    for (auto&& gantt : system_declarations.ganttChart)
        visitor.visit_gantt(gantt);

    visitor.visit_doc_after(*this);
}

void Document::begin_chan_priority(Expression chan)
{
    has_priorities |= true;
    ChanPriority priorities;
    priorities.head = std::move(chan);
    chan_priorities.push_back(priorities);
}

void Document::add_chan_priority(char separator, Expression chan)
{
    assert(separator == ',' || separator == '<');
    ChanPriority::Tail& tail = chan_priorities.back().tail;
    tail.emplace_back(separator, std::move(chan));
}

void Document::set_proc_priority(std::string_view name, int priority)
{
    has_priorities |= (priority != 0);
    proc_priority.emplace(name, priority);
}

int Document::get_proc_priority(std::string_view name) const
{
    auto it = proc_priority.find(name);
    assert(it != proc_priority.end());
    return it->second;
}

void Document::add_position(uint32_t position, uint32_t offset, uint32_t line, std::shared_ptr<std::string> path)
{
    positions.add(position, offset, line, std::move(path));
}

const PositionIndex::Line& Document::find_position(uint32_t position) const { return positions.find(position); }

void Document::add_channel(bool is_broadcast) { has_non_broadcast_chan |= !is_broadcast; }

const PositionIndex::Line& Document::find_first_position(uint32_t position) const
{
    return positions.find_first_line(position);
}

void Document::add_error(position_t position, std::string msg, std::string context)
{
    errors.emplace_back(positions.find(position.start), positions.find(position.end), position, std::move(msg),
                        std::move(context));
}

void Document::add_warning(position_t position, std::string msg, std::string context)
{
    warnings.emplace_back(positions.find(position.start), positions.find(position.end), position, std::move(msg),
                          std::move(context));
}

IODecl* Document::add_io_decl()
{
    global.iodecl.emplace_back();
    return &global.iodecl.back();
}

void Document::set_supported_methods(const SupportedMethods& supported_methods)
{
    this->supported_methods = supported_methods;
}
