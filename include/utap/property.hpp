// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2011-2023 Aalborg University.
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

#ifndef VERIFIER_PROPERTY
#define VERIFIER_PROPERTY

#include "utap.hpp"

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>

#include <utap/StatementBuilder.hpp>
#include <utap/TypeChecker.hpp>
#include <utap/document.hpp>

namespace UTAP {

/** Query quantifiers (which also capture the type of the query) */
enum class quant_t {
    empty = -1,  // fictitious value to denote unknown/unused quantifier
    /* Standard UPPAAL quantifiers. */
    AG,        // A[]
    EE,        // E<>
    EG,        // E[]
    AE,        // A<>
    leads_to,  // p --> q

    /* Statistical Model Checking extensions: */
    probaMinBox,         // Pr[...]([] ...) >= p
    probaMinDiamond,     // Pr[...](<> ...) >= p
    probaBox,            // Pr[...]([] ...)
    probaDiamond,        // Pr[...](<> ...)
    probaCompare,        // Pr[...](...) >= Pr[...](...)
    probaExpected,       // E[...](max: ...)
    probaSimulate,       // simulate [...] { ... }
    probaSimulateReach,  // simulate [...] { ... } : n : p
    Mitl,

    /* TIGA quantifiers for controller synthesis:
     * - A[ p U q ] : AUntil
     * - A[ p W q ] : AWeakUntil
     * - A<> p and A[] p. */
    control_AF,
    control_AUntil,
    control_AG,
    control_AWeakUntil,

    /* TIGA quantifiers with Buchi conditions. */
    control_AB,
    control_ABuchi,

    /* Extensions for testing: E<> control: ... */
    EF_control_AF,
    EF_control_AUntil,
    EF_control_AG,
    EF_control_AWeakUntil,

    /* Extensions for time optimality */
    control_opt_AF,
    control_opt_AUntil,
    control_opt_Def1_AF,
    control_opt_Def1_AUntil,
    control_opt_Def2_AF,
    control_opt_Def2_AUntil,

    /* Extensions for partial observability */
    PO_control_AF,
    PO_control_AUntil,
    PO_control_AG,
    PO_control_AWeakUntil,

    /* Extensions for smc */
    control_SMC_AUntil,
    control_SMC_AF,
    control_MinExp,
    control_MaxExp,
    strategy_load,
    strategy_save,

    /* Extensions for getting supremum/infimum/bounds for variable/clock values */
    supremum,
    infimum,
    bounds,

    /* Prob properties */
    PMax,

    /* LSC scenario property */
    scenario
};

/** property status */
enum class status_t { WAITING, RUNNING, DONE_TRUE, DONE_FALSE, DONE_MAYBE_TRUE, DONE_MAYBE_FALSE, DONE_ERROR };

inline const char* to_string(status_t s)
{
    switch (s) {
    case status_t::WAITING: return "waiting";
    case status_t::RUNNING: return "running";
    case status_t::DONE_TRUE: return "satisfied";
    case status_t::DONE_FALSE: return "unsatisfied";
    case status_t::DONE_MAYBE_TRUE: return "maybe satisfied";
    case status_t::DONE_MAYBE_FALSE: return "maybe unsatisfied";
    case status_t::DONE_ERROR: return "error";
    }
    assert(false);
    return nullptr;
}

enum StrategyType { None, ZoneStrategy, NonZoneStrategy };

struct expectation
{
    std::variant<status_t, double> result;
    uint64_t time_ms{0};
    uint64_t mem_kib{0};
    expectation() = default;
    template <typename Res>
    explicit expectation(Res res): result{res}
    {}
};

/**
 * Information about a property.
 */
struct PropInfo
{
    int line{0};                                  /**< Line no. in input file */
    int no{0};                                    /**< No. in input file, zero-indexed */
    quant_t type{};                               /**< Type of quantification */
    Expression intermediate{};              /**< State property as Expression */
    Options options{};                      /**< Options associated with the query */
    StrategyType result_type{};                   /**< which type the property returns*/
    std::vector<PropInfo*> subjections{};         /**< which strategies to evaluate under*/
    PropInfo* imitation = nullptr;                /**< which strategies to sample from*/
    std::string declaration{};                    /**< which strategy the property declares*/
    std::unique_ptr<expectation> expect{nullptr}; /**< expected result */
    PropInfo(int line, int no, Expression intermediate): line{line}, no{no}, intermediate{intermediate} {}
    /** TODO: refactor ModelChecker::modifySystem() to get rid of PropInfo copies */
    PropInfo(const PropInfo& o):
        line{o.line}, no{o.no}, type{o.type}, intermediate{o.intermediate}, options{o.options},
        result_type{o.result_type}, subjections{o.subjections}, imitation{o.imitation}, declaration{o.declaration}
    {
        if (o.expect)
            expect = std::make_unique<expectation>(*o.expect);
    }
    PropInfo& operator=(const PropInfo&) = delete;  // hopefully there is no need
    PropInfo(PropInfo&&) noexcept = default;
    PropInfo& operator=(PropInfo&&) noexcept = default;
    ~PropInfo() noexcept = default;

    template <typename Res>
    void set_expect(Res res)
    {
        expect = std::make_unique<expectation>(res);
    }
    void set_expect_mem(uint64_t kb)
    {
        if (!expect)
            expect = std::make_unique<expectation>();
        expect->mem_kib = kb;
    }
    void set_expect_time(uint64_t millis)
    {
        if (!expect)
            expect = std::make_unique<expectation>();
        expect->time_ms = millis;
    }
};

class PropertyBuilder : public StatementBuilder, public std::enable_shared_from_this<PropertyBuilder>
{
public:
    using Ptr = std::shared_ptr<PropertyBuilder>;
    using CPtr = std::shared_ptr<const PropertyBuilder>;
    typedef std::list<PropInfo>::const_iterator const_iterator;

private:
    TypeChecker tc;
    std::string lastExpect{};

protected:
    std::list<PropInfo> properties{};  // TigaPropertyBuilder assumes stable list and stores pointers.

    Variable* add_variable(Type type, std::string_view name, Expression init, position_t pos) override;
    bool add_function(Type type, std::string_view name, position_t pos) override;

    void type_check(Expression& expr) { tc.visitProperty(expr); }
    bool allow_process_references() override;

    virtual void type_property(Expression);

public:
    explicit PropertyBuilder(const Document& doc):
        StatementBuilder{const_cast<Document&>(doc)}, tc{const_cast<Document&>(doc)}
    {
        // The document is already fully parsed, so builtin_decl_end() was never
        // called for this builder. Push the innermost declaration frame (a
        // descendant of the global frame) so property expressions can resolve
        // both global and system declarations (e.g. process instances).
        push_frame(document.get_system_declarations().frame);
    }

    void clear() { properties.clear(); }
    const std::list<PropInfo>& get_properties() const { return properties; }

    // parse queries in buffer
    void parse(const char* buf) override;
    // parse queries in file
    void parse(FILE* file);
    // parse single query
    void parse(const char* buf, const std::string& xpath, const Options&);

    const_iterator begin() const { return properties.begin(); }
    const_iterator end() const { return properties.end(); }

    void property() override;
    void scenario(std::string_view) override;
    void handle_expect(std::string_view text) override;

    bool is_SMC(const Expression* expr = nullptr) const;
};

class TigaPropertyBuilder final : public PropertyBuilder
{
protected:
    std::map<std::string, PropInfo*, std::less<>> declarations{};

    // Should be implemented as frames instead if an actual language evolves
    // also use set to avoid (evaluating) duplicates
    std::vector<PropInfo*> subjections{};
    PropInfo* _imitation{nullptr};

    void type_property(Expression) override;
    void strategy_declaration(std::string_view) override;
    void subjection(std::string_view) override;
    void imitation(std::string_view) override;
    void expr_optimize(int, int, int, int) override;

public:
    TigaPropertyBuilder(const Document& doc): PropertyBuilder{doc} {}

    /* Should be implemented by verifier/property.h at some point.
       virtual void param_property(size_t, Constants::Kind);
       virtual void decl_param_id(std::string_view);
    */
};

}  // namespace UTAP

#endif /* VERIFIER_PROPERTY */
