// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2018 Uppsala University and Aalborg University.

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

#include "utap/ExpressionBuilder.hpp"

#include "utap/TypeChecker.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstring>

using namespace UTAP;
using namespace Constants;

inline static bool isMITL(const Expression& e)
{
    switch (e.get_kind()) {
    case MITL_FORMULA:
    case MITL_RELEASE:
    case MITL_UNTIL:
    case MITL_CONJ:
    case MITL_DISJ:
    case MITL_NEXT:
    case MITL_ATOM:
    case MITL_EXISTS:
    case MITL_FORALL: return true;
    default: return false;
    }
}

inline static Expression toMITLAtom(const Expression& e) { return Expression::create_unary(MITL_ATOM, e); }

ExpressionBuilder::ExpressionBuilder(Document& doc): document{doc}
{
    Frame frame = doc.get_builtin_decls();
    assert(frame != Frame{});
    push_frame(std::move(frame));
}

void ExpressionBuilder::add_position(uint32_t position, uint32_t offset, uint32_t line,
                                     std::shared_ptr<std::string> path)
{
    document.add_position(position, offset, line, std::move(path));
}

void ExpressionBuilder::handle_error(const TypeException& ex) { document.add_error(position, ex.what()); }

void ExpressionBuilder::handle_warning(const TypeException& ex) { document.add_warning(position, ex.what()); }

void ExpressionBuilder::builtin_decl_end() { push_frame(document.get_globals().frame); }

bool ExpressionBuilder::resolve(std::string_view name, Symbol& uid) const
{
    assert(!frames.empty());
    return frames.top().resolve(name, uid);
}

bool ExpressionBuilder::is_type(std::string_view name)
{
    Symbol uid;
    if (!resolve(name, uid)) {
        return false;
    }
    return uid.get_type().get_kind() == TYPEDEF;
}

Expression ExpressionBuilder::make_constant(int value) const { return Expression::create_constant(value, position); }

Expression ExpressionBuilder::make_constant(double value) const { return Expression::create_double(value, position); }

Expression ExpressionBuilder::make_constant(std::string_view value) const
{
    auto is = std::istringstream{std::string{value}};
    auto newstring = std::string{};
    is >> std::quoted(newstring);
    StringIndex str = document.add_string(std::move(newstring));
    return Expression::create_string(str, position);
}

Type ExpressionBuilder::apply_prefix(TypePrefix prefix, Type type)
{
    switch (prefix) {
    case TypePrefix::CONST: return type.create_prefix(CONSTANT, position);
    case TypePrefix::SYSTEM_META:
        // Meta in the syntax corresponds to a static variable internally.
        // Internal "meta" variables correspond to state meta variables.
        return type.create_prefix(SYSTEM_META, position);
    case TypePrefix::URGENT: return type.create_prefix(URGENT, position);
    case TypePrefix::BROADCAST: return type.create_prefix(BROADCAST, position);
    case TypePrefix::URGENT_BROADCAST: return type.create_prefix(URGENT, position).create_prefix(BROADCAST, position);
    case TypePrefix::HYBRID: return type.create_prefix(HYBRID, position);
    default: return type;
    }
}

void ExpressionBuilder::type_duplicate() { typeFragments.duplicate(); }

void ExpressionBuilder::type_pop() { typeFragments.pop(); }

void ExpressionBuilder::type_bool(TypePrefix prefix)
{
    Type type = Type::create_primitive(Constants::BOOL, position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_int(TypePrefix prefix)
{
    Type type = Type::create_primitive(Constants::INT, position);
    if (prefix != TypePrefix::CONST) {
        type = Type::create_range(type, make_constant(defaultIntMin), make_constant(defaultIntMax), position);
    }
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_string(TypePrefix prefix)
{
    if (prefix != TypePrefix::CONST) {
        typeFragments.push(Type::create_primitive(VOID_TYPE));
        throw TypeException("$Strings_should_always_be_const");
    }
    Type type = Type::create_primitive(Constants::STRING, position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_double(TypePrefix prefix)
{
    Type type = Type::create_primitive(Constants::DOUBLE, position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_bounded_int(TypePrefix prefix)
{
    Type type = Type::create_primitive(Constants::INT, position);
    type = Type::create_range(type, fragments[1], fragments[0], position);
    fragments.pop(2);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_channel(TypePrefix prefix)
{
    bool is_broadcast = prefix == TypePrefix::BROADCAST || prefix == TypePrefix::URGENT_BROADCAST;
    document.add_channel(is_broadcast);
    Type type = Type::create_primitive(CHANNEL, position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_clock(TypePrefix prefix)
{
    Type type = Type::create_primitive(CLOCK, position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::type_void()
{
    Type type = Type::create_primitive(VOID_TYPE, position);
    typeFragments.push(type);
}

static void collectDependencies(std::set<Symbol>& dependencies, const Expression& expr)
{
    std::set<Symbol> symbols;
    expr.collect_possible_reads(symbols);
    while (!symbols.empty()) {
        Symbol s = *symbols.begin();
        symbols.erase(s);
        if (dependencies.find(s) == dependencies.end()) {
            dependencies.insert(s);
            if (auto* data = s.get_data(); data) {
                auto* v = static_cast<Variable*>(data);
                v->init.collect_possible_reads(symbols);
            }
        }
    }
}

void ExpressionBuilder::type_scalar(TypePrefix prefix)
{
    Expression lower, upper;

    expr_nat(1);
    expr_binary(MINUS);
    upper = fragments[0];
    lower = make_constant(0);
    fragments.pop();

    auto type = Type::create_primitive(SCALAR, position);
    type = Type::create_range(type, lower, upper, position);
    type = apply_prefix(prefix, type);

    auto count = std::to_string(scalar_count++);

    type = type.create_label("#scalarset" + count, position);

    if (currentTemplate != nullptr) {
        /* Local scalar definitions are local to a particular process
         * - not to the template. Therefore we prefix it with the
         * template name and rename the template name to the process
         * name whenever evaluating a P.symbol expression (where P is
         * a processs). See exprDot().
         */
        type = type.create_label(currentTemplate->uid.get_name() + "::", position);

        /* There are restrictions on how the size of a scalar set is
         * given (may not depend on free process parameters).
         * Therefore mark all symbols in upper and those that they
         * depend on as restricted.
         */
        collectDependencies(currentTemplate->restricted, upper);
    }
    typeFragments.push(type);
}

void ExpressionBuilder::type_name(TypePrefix prefix, std::string_view name)
{
    Symbol uid;
    assert(resolve(name, uid));

    if (!resolve(name, uid) || uid.get_type().get_kind() != TYPEDEF) {
        typeFragments.push(Type::create_primitive(VOID_TYPE));
        throw TypeException("$Identifier_is_undeclared_or_not_a_type_name");
    }

    Type type = uid.get_type()[0];

    /* We create a label here such that we can track the
     * position. This is not needed for type checking (we only use
     * name equivalence for scalarset, and they have a name embedded
     * in the type, see type_scalar()).
     */
    type = type.create_label(uid.get_name(), position);
    typeFragments.push(apply_prefix(prefix, type));
}

void ExpressionBuilder::expr_true()
{
    Expression expr = make_constant(1);
    expr.set_type(Type::create_primitive(Constants::BOOL));
    fragments.push(expr);
}

void ExpressionBuilder::expr_false()
{
    Expression expr = make_constant(0);
    expr.set_type(Type::create_primitive(Constants::BOOL));
    fragments.push(expr);
}

void ExpressionBuilder::expr_double(double d)
{
    Expression expr = Expression::create_double(d, position);
    expr.set_type(Type::create_primitive(Constants::DOUBLE));
    fragments.push(expr);
}

void ExpressionBuilder::expr_string(std::string_view name) { fragments.push(make_constant(std::string{name})); }

void ExpressionBuilder::expr_identifier(std::string_view name)
{
    Symbol uid;

    if (!resolve(name, uid)) {
        expr_false();
        throw unknown_identifier_error(name);
    }

    fragments.push(Expression::create_identifier(uid, position));
}

void ExpressionBuilder::expr_deadlock() { fragments.push(Expression::create_deadlock(position)); }

void ExpressionBuilder::expr_nat(int32_t n) { fragments.push(make_constant(n)); }

void ExpressionBuilder::expr_call_begin() {}

// expects n argument expressions on the stack
void ExpressionBuilder::expr_call_end(uint32_t n)
{
    auto e = Expression{};
    auto type = Type{};
    const auto* instance = static_cast<Instance*>(nullptr);

    // n+1'th element from the top is the identifier.
    auto id = fragments[n];

    /* Create vector of sub expressions: The first expression
     * evaluates to the function or processset. The remaining
     * expressions are the arguments.
     */
    auto expr = std::vector<Expression>{};
    for (int i = n; i >= 0; --i)
        expr.push_back(fragments[i]);
    fragments.pop(n + 1);

    /* The expression we create depends on whether id is a
     * function or a processset.
     */
    switch (id.get_type().get_kind()) {
    case FUNCTION_EXTERNAL:
    case FUNCTION:
        if (expr.size() != id.get_type().size())
            handle_error(TypeException{"$Wrong_number_of_arguments"});
        e = Expression::create_nary(id.get_type().get_kind() == FUNCTION ? FUN_CALL : FUN_CALL_EXT, expr, position,
                                    id.get_type()[0]);
        break;

    case PROCESS_SET:
        if (expr.size() - 1 != id.get_type().size())
            handle_error(TypeException{"$Wrong_number_of_arguments"});
        instance = static_cast<const Instance*>(id.get_symbol().get_data());

        /* Process set lookups are represented as expressions indexing
         * into an array. To satisfy the type checker, we create a
         * type matching this structure.
         */
        type = Type::create_process(instance->templ->frame);
        for (auto i = 0u; i < instance->unbound; ++i) {
            type = Type::create_array(type, instance->parameters[instance->unbound - i - 1].get_type());
        }

        /* Now create the expression. Each argument to the process set
         * lookup is represented as an ARRAY expression.
         */
        e = id;
        e.set_type(type);
        for (size_t i = 1; i < expr.size(); ++i) {
            type = type.get_sub();
            e = Expression::create_binary(ARRAY, e, expr[i], position, type);
        }
        break;

    default:
        handle_error(TypeException{"$Function_expected"});
        e = make_constant(0);
        break;
    }

    fragments.push(std::move(e));
}

// 2 expr     // array[index]
void ExpressionBuilder::expr_array()
{
    // Pop sub-expressions
    Expression var = fragments[1];
    Expression index = fragments[0];
    fragments.pop(2);

    Type element;
    Type type = var.get_type();
    if (type.is_array()) {
        element = type.get_sub();
    } else {
        element = Type();
    }

    fragments.push(Expression::create_binary(ARRAY, var, index, position, element));
}

// 1 expr
void ExpressionBuilder::expr_post_increment()
{
    fragments[0] = Expression::create_unary(POST_INCREMENT, fragments[0], position);
}

void ExpressionBuilder::expr_pre_increment()
{
    fragments[0] = Expression::create_unary(PRE_INCREMENT, fragments[0], position, fragments[0].get_type());
}

void ExpressionBuilder::expr_post_decrement()  // 1 expr
{
    fragments[0] = Expression::create_unary(POST_DECREMENT, fragments[0], position);
}

void ExpressionBuilder::expr_pre_decrement()
{
    fragments[0] = Expression::create_unary(PRE_DECREMENT, fragments[0], position, fragments[0].get_type());
}

void ExpressionBuilder::expr_builtin_function1(Kind kind)
{
    fragments[0] = Expression::create_unary(kind, fragments[0], position);
}

void ExpressionBuilder::expr_builtin_function2(Kind kind)
{
    Expression lvalue = fragments[1];
    Expression rvalue = fragments[0];
    fragments.pop(1);
    fragments[0] = Expression::create_binary(kind, lvalue, rvalue, position, lvalue.get_type());
}

void ExpressionBuilder::expr_builtin_function3(Kind kind)
{
    Expression value1 = fragments[2];
    Expression value2 = fragments[1];
    Expression value3 = fragments[0];
    fragments.pop(2);
    fragments[0] = Expression::create_ternary(kind, value1, value2, value3, position, value1.get_type());
}

void ExpressionBuilder::expr_assignment(Kind op)  // 2 expr
{
    Expression lvalue = fragments[1];
    Expression rvalue = fragments[0];
    fragments.pop(2);
    fragments.push(Expression::create_binary(op, lvalue, rvalue, position, lvalue.get_type()));
}

void ExpressionBuilder::expr_unary(Kind unaryop)  // 1 expr
{
    switch (unaryop) {
    case PLUS:
        /* Unary plus can be ignored */
        break;
    case MINUS:
        unaryop = UNARY_MINUS;
        [[fallthrough]];
    default: fragments[0] = Expression::create_unary(unaryop, fragments[0], position, fragments[0].get_type());
    }
}

void ExpressionBuilder::expr_binary(Kind binaryop)  // 2 expr
{
    Kind mitlop = (binaryop == AND ? MITL_CONJ : MITL_DISJ);
    Kind op = binaryop;
    Expression left = fragments[1];
    Expression right = fragments[0];
    if (isMITL(left) || isMITL(right)) {
        op = mitlop;
        if (!(isMITL(left) && isMITL(right))) {
            if (isMITL(left)) {
                op = mitlop;
                right = toMITLAtom(right);
            } else if (isMITL(right)) {
                op = mitlop;
                left = toMITLAtom(left);
            }
        }
    }
    fragments.pop(2);
    fragments.push(Expression::create_binary(op, left, right, position));
}

void ExpressionBuilder::expr_nary(Kind kind, uint32_t num)
{
    // Pop fields
    auto fields = std::vector<Expression>(num);
    for (uint32_t i = 0; i < num; ++i)
        fields[i] = fragments[num - 1 - i];
    fragments.pop(num);

    // Create N-ary expression
    fragments.push(Expression::create_nary(kind, fields, position));
}

void ExpressionBuilder::expr_scenario(std::string_view name)
{
    auto uid = Symbol{};
    auto check [[maybe_unused]] = resolve(name, uid);
    assert(check);
    auto scen = Expression::create_identifier(uid);
    auto expr = Expression::create_unary(SCENARIO, scen, position);
    fragments.push(Expression::create_unary(SCENARIO, scen, position));
}

Expression ExpressionBuilder::exprScenario()
{
    Symbol uid;
    bool check [[maybe_unused]] = resolve(document.obsTA, uid);
    assert(check);
    Expression obs = Expression::create_identifier(uid);  // std::cout << obs << std::endl;
    auto i = obs.get_type().find_index_of("lmin");
    Expression left =
        Expression::create_dot(obs, i.value_or(-1), position,
                               Type::create_primitive(Constants::BOOL));  // std::cout << left << std::endl;

    obs = Expression::create_identifier(uid);
    i = obs.get_type().find_index_of("lmax");
    Expression right =
        Expression::create_dot(obs, i.value_or(-1), position,
                               Type::create_primitive(Constants::BOOL));  // std::cout << right << std::endl;
    return Expression::create_binary(SCENARIO2, left, right, position);
}

void ExpressionBuilder::expr_ternary(Kind ternaryop, bool firstMissing)  // 3 expr
{
    Expression first = firstMissing ? make_constant(1) : fragments[2];
    Expression second = fragments[1];
    Expression third = fragments[0];
    fragments.pop(firstMissing ? 2 : 3);
    fragments.push(Expression::create_ternary(ternaryop, first, second, third, position));
}

void ExpressionBuilder::expr_inline_if()
{
    Expression c = fragments[2];
    Expression t = fragments[1];
    Expression e = fragments[0];
    fragments.pop(3);

    fragments.push(Expression::create_ternary(INLINE_IF, c, t, e, position));
}

void ExpressionBuilder::expr_comma()
{
    Expression e1 = fragments[1];
    Expression e2 = fragments[0];
    fragments.pop(2);
    fragments.push(Expression::create_binary(COMMA, e1, e2, position, e2.get_type()));
}

void ExpressionBuilder::expr_location()
{
    Expression expr = fragments[0];
    Type type = expr.get_type();
    if (type.is_process()) {
        // TODO: create a separate type for location expressions and get rid of magical constants
        // we use special max-value to denote this special "meta-variable"
        expr = Expression::create_dot(expr, std::numeric_limits<int32_t>::max(), position,
                                      Type::create_primitive(Constants::LOCATION_EXPR));
    } else {
        handle_error(not_a_process_error(expr.str(true)));
    }
    fragments[0] = expr;
}

void ExpressionBuilder::expr_dot(std::string_view id)
{
    Expression expr = fragments[0];
    Type type = expr.get_type();
    if (type.is_record()) {
        auto i = type.find_index_of(id);
        if (!i) {
            handle_error(has_no_such_member_error(id));
        } else {
            expr = Expression::create_dot(expr, *i, position, type.get_sub(*i));
        }
    } else if (type.is_process()) {
        Symbol name = expr.get_symbol();
        auto* process = static_cast<Instance*>(name.get_data());
        auto i = type.find_index_of(id);
        if (!i) {
            handle_error(has_no_such_member_error(id));
        } else if (type.get_sub(*i).is_location()) {
            expr = Expression::create_dot(expr, *i, position, Type::create_primitive(Constants::BOOL));
        } else {
            type = type.get_sub(*i).rename(process->templ->uid.get_name() + "::", name.get_name() + "::");
            for (const auto& [s, e] : process->mapping)
                type = type.subst(s, e);
            expr = Expression::create_dot(expr, *i, position, type);
        }
    } else if (type.is(PROCESS_VAR)) {
        Symbol uid;
        // temporarily set the frame to that of its associated template
        if (dynamicFrames.find(expr.get_symbol().get_name()) == dynamicFrames.end()) {
            throw unknown_identifier_error(expr.get_symbol().get_name());
        }
        push_frame(dynamicFrames[expr.get_symbol().get_name()]);

        if (!resolve(id, uid)) {
            expr_false();
            throw unknown_identifier_error(id);
        }
        pop_frame();  // Remove that frame again
        Expression identifier = Expression::create_identifier(uid, position);

        expr = (Expression::create_nary(
            DYNAMIC_EVAL, {identifier, expr}, position,
            identifier.get_type().is_location()
                ? Type::create_primitive(Constants::BOOL, position)
                : identifier.get_type()));  // Type::createPrimitive (Constants::BOOL,position)));
    } else {
        handle_error(is_not_a_struct_error(expr.str(true)));
    }
    fragments[0] = expr;
}

void ExpressionBuilder::expr_forall_begin(std::string_view name)
{
    Type type = typeFragments[0];
    typeFragments.pop();

    if (!type.is(CONSTANT)) {
        type = type.create_prefix(CONSTANT);
    }

    push_frame(frames.top().make_sub());
    Symbol symbol = frames.top().add_symbol(name, type, position);

    if (!type.is_integer() && !type.is_scalar()) {
        handle_error(TypeException{"$Quantifier_must_range_over_integer_or_scalar_set"});
    }
}

void ExpressionBuilder::expr_forall_end(std::string_view name)
{
    /* Create the forall expression. The symbol is added as an identifier
     * expression as the first child. Notice that the frame is discarded
     * but the identifier expression will maintain a reference to the
     * symbol so it will not be deallocated.
     */
    fragments[0] = Expression::create_binary(FORALL, Expression::create_identifier(frames.top()[0], position),
                                             fragments[0], position);
    pop_frame();
}

void ExpressionBuilder::expr_exists_begin(std::string_view name) { expr_forall_begin(name); }

void ExpressionBuilder::expr_exists_end(std::string_view name)
{
    /* Create the exists expression. The symbol is added as an identifier
     * expression as the first child. Notice that the frame is discarded
     * but the identifier expression will maintain a reference to the
     * symbol so it will not be deallocated.
     */
    fragments[0] = Expression::create_binary(EXISTS, Expression::create_identifier(frames.top()[0], position),
                                             fragments[0], position);
    pop_frame();
}

void ExpressionBuilder::expr_sum_begin(std::string_view name) { expr_forall_begin(name); }

void ExpressionBuilder::expr_sum_end(std::string_view name)
{
    /* Create the sum expression. The symbol is added as an identifier
     * expression as the first child. Notice that the frame is discarded
     * but the identifier expression will maintain a reference to the
     * symbol so it will not be deallocated.
     */
    fragments[0] = Expression::create_binary(SUM, Expression::create_identifier(frames.top()[0], position),
                                             fragments[0], position);
    pop_frame();
}

void ExpressionBuilder::expr_proba_qualitative(Constants::Kind pathType, Constants::Kind comp, double probBound)
{
    auto invert = (comp == LE);
    auto& boundTypeOrBoundedExpr = fragments[3];
    auto& bound = fragments[2];
    auto& runs = fragments[1];
    auto& predicate = fragments[0];

    auto args = std::vector<Expression>{runs, boundTypeOrBoundedExpr, bound,
                                        invert ? Expression::create_unary(NOT, predicate, position) : predicate,
                                        Expression::create_double(invert ? 1.0 - probBound : probBound, position)};

    fragments.pop(4);
    fragments.push(Expression::create_nary(invert ? (pathType == BOX ? PROBA_MIN_DIAMOND : PROBA_MIN_BOX)
                                                  : (pathType == BOX ? PROBA_MIN_BOX : PROBA_MIN_DIAMOND),
                                           std::move(args), position));
}

void ExpressionBuilder::expr_optimize_exp(Constants::Kind kind, PRICETYPE ptype, Constants::Kind goal_type)
{
    if (goal_type != Constants::DIAMOND)
        handle_error(TypeException{"$Wrong_path_quantifier"});

    auto boundVar = fragments[4];
    auto bound = fragments[3];
    auto discrete = fragments[2];
    auto cont = fragments[1];
    auto goal = fragments[0];

    if (!discrete.is_true() && !cont.is_true()) {
        discrete.set_type(Type::create_primitive(LIST, position));
        cont.set_type(Type::create_primitive(LIST, position));
    }
    Expression price;
    Expression level = make_constant(0);
    size_t nb = 4;
    switch (ptype) {
    case TIMEPRICE:  // use time
        price = make_constant(1);
        break;
    case EXPRPRICE:  // user-provided expression
        price = fragments[5];
        ++nb;
        break;
    case PROBAPRICE: price = goal; break;
    default: handle_error(TypeException{"$Unknown_price_type"});
    }

    assert(nb <= fragments.size());

    auto args = std::vector<Expression>{boundVar, bound, goal, price, level, discrete, cont};
    fragments.pop(nb);
    fragments.push(Expression::create_nary(kind, std::move(args), position));
}

void ExpressionBuilder::expr_load_strategy()
{
    Expression discrete = fragments[2];
    Expression cont = fragments[1];
    Expression strat = fragments[0];
    if (!discrete.is_true() && !cont.is_true()) {
        discrete.set_type(Type::create_primitive(LIST, position));
        cont.set_type(Type::create_primitive(LIST, position));
    }
    fragments.pop(3);
    fragments.push(Expression::create_ternary(LOAD_STRAT, strat, discrete, cont, position));
}

void ExpressionBuilder::expr_save_strategy(std::string_view strategy_name)
{
    assert(fragments.size() == 1);
    fragments[0] = Expression::create_binary(SAVE_STRAT, fragments[0], make_constant(strategy_name), position);
}

void ExpressionBuilder::expr_proba_quantitative(Constants::Kind pathType)
{
    auto& boundTypeOrBoundedExpr = fragments[4];
    auto& bound = fragments[3];
    auto& runs = fragments[2];
    auto& predicate = fragments[1];
    auto& untilCond = fragments[0];

    auto args = std::vector<Expression>{runs, boundTypeOrBoundedExpr, bound, predicate, untilCond};
    fragments.pop(5);
    fragments.push(Expression::create_nary((pathType == BOX ? PROBA_BOX : PROBA_DIAMOND), std::move(args), position));
}

void ExpressionBuilder::expr_proba_compare(Constants::Kind pathType1, Constants::Kind pathType2)
{
    auto& boundTypeOrBoundedExpr1 = fragments[7];
    auto& bound1 = fragments[6];
    auto& runs1 = fragments[5];
    auto& predicate1 = fragments[4];

    auto& boundTypeOrBoundedExpr2 = fragments[3];
    auto& bound2 = fragments[2];
    auto& runs2 = fragments[1];
    auto& predicate2 = fragments[0];

    if (runs1.get_value() != -1 || runs2.get_value() != -1)
        throw TypeException("The number of runs is not supported in probability comparison");

    auto args = std::vector<Expression>{boundTypeOrBoundedExpr1, bound1, make_constant(pathType1), predicate1,
                                        boundTypeOrBoundedExpr2, bound2, make_constant(pathType2), predicate2};

    fragments.pop(8);
    fragments.push(Expression::create_nary(PROBA_CMP, std::move(args), position));
}

void ExpressionBuilder::expr_proba_expected(std::string_view aggregatingOp)
{
    auto& boundTypeOrBoundedExpr = fragments[3];
    auto& bound = fragments[2];
    auto& runs = fragments[1];
    auto& expression = fragments[0];

    int aggOpId;
    if (aggregatingOp == "min")
        aggOpId = 0;
    else if (aggregatingOp == "max")
        aggOpId = 1;
    else
        throw TypeException("min or max expected");
    // TODO: add "acc" when the semantics is defined.

    auto args = std::vector<Expression>{runs, boundTypeOrBoundedExpr, bound, make_constant(aggOpId), expression};
    fragments.pop(4);
    fragments.push(Expression::create_nary(PROBA_EXP, std::move(args), position));
}

void ExpressionBuilder::expr_simulate(int nbExpr, bool hasReach, int numberOfAcceptingRuns)
{
    // Stack:
    // conditional bound name
    // expr bound
    // nbExpr * expr
    // if (hasReach) => expr
    auto offset = nbExpr + (hasReach ? 1 : 0);
    auto& boundTypeOrBoundedExpr = fragments[2 + offset];
    auto& bound = fragments[1 + offset];
    auto runs = fragments[0 + offset];

    if (runs.get_kind() == CONSTANT && runs.get_type().is_integer() && runs.get_value() < 0)
        runs = make_constant(1);

    auto args = std::vector<Expression>{};
    args.reserve(offset + 4);  // 3-from-above + offset*expressions + numberOfAcceptingRuns
    args.push_back(runs);
    args.push_back(boundTypeOrBoundedExpr);
    args.push_back(bound);
    for (auto i = 0; i < nbExpr; ++i)
        args.push_back(fragments[offset - 1 - i]);  // recover the original order

    if (hasReach) {
        auto& predicate = fragments[0];
        args.push_back(predicate);
        args.push_back(make_constant(numberOfAcceptingRuns));
    }

    fragments.pop(offset + 3);
    if (hasReach)
        fragments.push(Expression::create_nary(SIMULATEREACH, std::move(args), position));
    else
        fragments.push(Expression::create_nary(SIMULATE, std::move(args), position));
}

void ExpressionBuilder::expr_MITL_formula()
{
    Expression mitl = fragments[0];
    if (!isMITL(mitl))
        mitl = toMITLAtom(mitl);
    Expression form = Expression::create_unary(MITL_FORMULA, mitl, position);
    fragments.pop();
    fragments.push(form);
}

void ExpressionBuilder::expr_MITL_until(int low, int high)
{
    Expression left = fragments[1];
    Expression right = fragments[0];
    if (!isMITL(left))
        left = toMITLAtom(left);
    if (!isMITL(right))
        right = toMITLAtom(right);
    auto lowd = make_constant(low);
    auto highd = make_constant(high);
    auto args = std::vector<Expression>{left, lowd, highd, right};
    Expression form = Expression::create_nary(MITL_UNTIL, std::move(args), position);
    fragments.pop(2);
    fragments.push(form);
}

void ExpressionBuilder::expr_MITL_release(int low, int high)
{
    Expression left = fragments[1];
    Expression right = fragments[0];
    if (!isMITL(left))
        left = toMITLAtom(left);
    if (!isMITL(right))
        right = toMITLAtom(right);
    auto lowd = make_constant(low);
    auto highd = make_constant(high);
    auto args = std::vector<Expression>{left, lowd, highd, right};
    fragments.pop(2);
    fragments.push(Expression::create_nary(MITL_RELEASE, std::move(args), position));
}

/*transform the diamond <>[low,high]phi into a (true U[low,high] phi) structure */
void ExpressionBuilder::expr_MITL_diamond(int low, int high)
{
    auto left = Expression::create_unary(MITL_ATOM, make_constant(1));
    auto right = fragments[0];
    if (!isMITL(right))
        right = toMITLAtom(right);
    auto lowd = make_constant(low);
    auto highd = make_constant(high);
    auto args = std::vector<Expression>{left, lowd, highd, right};
    Expression form = Expression::create_nary(MITL_UNTIL, std::move(args), position);
    fragments.pop(1);
    fragments.push(form);
}

/*transform the diamond [][low,high]phi into a (false R[low,high] phi) structure */
void ExpressionBuilder::expr_MITL_box(int low, int high)
{
    auto left = Expression::create_unary(MITL_ATOM, make_constant(0));
    auto right = fragments[0];
    if (!isMITL(right))
        right = toMITLAtom(right);
    auto lowd = make_constant(low);
    auto highd = make_constant(high);
    auto args = std::vector<Expression>{left, lowd, highd, right};
    Expression form = Expression::create_nary(MITL_RELEASE, std::move(args), position);
    fragments.pop(1);
    fragments.push(form);
}

void ExpressionBuilder::expr_MITL_disj()
{
    auto& left = fragments[1];
    auto& right = fragments[0];
    Expression form = Expression::create_binary(MITL_DISJ, left, right, position);
    fragments.pop(2);
    fragments.push(form);
}

void ExpressionBuilder::expr_MITL_conj()
{
    auto left = fragments[1];
    auto right = fragments[0];
    fragments.pop(2);
    fragments.push(Expression::create_binary(MITL_CONJ, left, right, position));
}

void ExpressionBuilder::expr_MITL_next()
{
    Expression next = fragments[0];
    if (!isMITL(next))
        next = toMITLAtom(next);
    fragments.pop();
    fragments.push(Expression::create_unary(MITL_NEXT, next, position));
}

void ExpressionBuilder::expr_MITL_atom()
{
    Expression atom = fragments[0];
    if (!isMITL(atom)) {
        fragments.pop();
        fragments.push(Expression::create_unary(MITL_ATOM, atom, position));
    }
}

void ExpressionBuilder::expr_spawn(int n)
{
    Expression id = fragments[n];
    auto exprs = std::vector<Expression>(n + 1);
    for (auto i = 0; i <= n; ++i)
        exprs[i] = fragments[n - i];
    fragments.pop(n + 1);
    fragments.push(Expression::create_nary(SPAWN, std::move(exprs), position, id.get_type()));
}

void ExpressionBuilder::expr_exit() { fragments.push(Expression::create_exit(position)); }

void ExpressionBuilder::expr_numof()
{
    Expression id = fragments[0];
    Type t = Type::create_primitive(Constants::INT, position);
    fragments.pop();
    fragments.push(Expression::create_unary(NUMOF, id, position, t));
}

void ExpressionBuilder::expr_forall_dynamic_begin(std::string_view name, std::string_view temp)
{
    push_frame(frames.top().make_sub());
    frames.top().add_symbol(name, Type::create_primitive(PROCESS_VAR, position), position);
    Template* templ = document.find_dynamic_template(temp);
    if (templ == nullptr)
        throw unknown_dynamic_template_error(temp);
    // dynamicFrames[name]=templ->frame;
    push_dynamic_frame_of(templ, name);
}

void ExpressionBuilder::expr_forall_dynamic_end(std::string_view name)
{
    // At this instant we should have expression on top of the stack and the template identifier
    // below it
    auto& expr = fragments[0];
    auto& process = fragments[1];
    auto identifier = Expression::create_identifier(frames.top()[0], position);
    bool mitl = isMITL(expr);
    if (mitl) {
        if (expr.get_kind() == MITL_ATOM) {
            expr = expr.get(0).clone();
            mitl = false;
        }
    }

    auto exprs = std::vector<Expression>{identifier, process, expr};
    fragments.pop(2);
    fragments.push(Expression::create_nary((mitl ? MITL_FORALL : FORALL_DYNAMIC), std::move(exprs), position,
                                           Type::create_primitive(Constants::BOOL, position)));
    pop_frame();
    pop_dynamic_frame_of(name);
}
void ExpressionBuilder::expr_exists_dynamic_begin(std::string_view name, std::string_view temp)
{
    push_frame(frames.top().make_sub());
    frames.top().add_symbol(name, Type::create_primitive(Constants::PROCESS_VAR, position), position);
    if (Template* templ = document.find_dynamic_template(temp); templ == nullptr)
        throw unknown_dynamic_template_error(temp);
    else {
        // dynamicFrames [name]=templ->frame;
        push_dynamic_frame_of(templ, name);
    }
}

void ExpressionBuilder::expr_exists_dynamic_end(std::string_view name)
{
    Expression expr = fragments[0];
    Expression process = fragments[1];
    Expression identifier = Expression::create_identifier(frames.top()[0], position);
    bool mitl = isMITL(expr);
    if (mitl) {
        if (expr.get_kind() == MITL_ATOM) {
            expr = expr.get(0).clone();
            mitl = false;
        }
    }
    auto exprs = std::vector<Expression>{identifier, process, expr};
    fragments.pop(2);
    fragments.push(Expression::create_nary((mitl ? MITL_EXISTS : EXISTS_DYNAMIC), std::move(exprs), position,
                                           Type::create_primitive(Constants::BOOL, position)));
    pop_frame();
    pop_dynamic_frame_of(name);
}

void ExpressionBuilder::expr_sum_dynamic_begin(std::string_view name, std::string_view temp)
{
    push_frame(frames.top().make_sub());
    frames.top().add_symbol(name, Type::create_primitive(Constants::PROCESS_VAR, position), position);
    Template* templ = document.find_dynamic_template(temp);
    if (templ == nullptr)
        throw unknown_dynamic_template_error(temp);
    // dynamicFrames [name]=templ->frame;
    push_dynamic_frame_of(templ, name);
}

void ExpressionBuilder::expr_sum_dynamic_end(std::string_view name)
{
    Expression& expr = fragments[0];
    Expression& process = fragments[1];
    Expression identifier = Expression::create_identifier(frames.top()[0], position);
    auto exprs = std::vector<Expression>{identifier, process, expr};
    fragments.pop(2);
    fragments.push(Expression::create_nary(SUM_DYNAMIC, std::move(exprs), position, expr.get_type()));
    pop_frame();
    pop_dynamic_frame_of(name);
}

void ExpressionBuilder::expr_foreach_dynamic_begin(std::string_view name, std::string_view temp)
{
    push_frame(frames.top().make_sub());
    frames.top().add_symbol(name, Type::create_primitive(Constants::PROCESS_VAR, position), position);
    if (auto* t = document.find_dynamic_template(temp); t != nullptr) {
        // dynamicFrames [name]=document->find_dynamic_template(temp)->frame;
        push_dynamic_frame_of(document.find_dynamic_template(temp), name);
    } else
        throw unknown_dynamic_template_error(temp);
}

void ExpressionBuilder::expr_foreach_dynamic_end(std::string_view name)
{
    auto& expr = fragments[0];
    auto& process = fragments[1];
    auto identifier = Expression::create_identifier(frames.top()[0], position);
    auto exprs = std::vector<Expression>{identifier, process, expr};
    fragments.pop(2);
    fragments.push(Expression::create_nary(FOREACH_DYNAMIC, std::move(exprs), position,
                                           Type::create_primitive(Constants::INT, position)));
    pop_frame();
    pop_dynamic_frame_of(name);
}

void ExpressionBuilder::push_dynamic_frame_of(Template* t, std::string_view name)
{
    if (!t->is_defined)
        throw TypeException("Template referenced before used");
    dynamicFrames.emplace(std::string{name}, t->frame);
}

void ExpressionBuilder::pop_dynamic_frame_of(std::string_view name)
{
    if (auto it = dynamicFrames.find(name); it != dynamicFrames.end())
        dynamicFrames.erase(it);
}
