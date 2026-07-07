// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2020 Aalborg University.
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

#ifndef UTAP_COMMON_HH
#define UTAP_COMMON_HH

namespace UTAP {
namespace Constants {
enum Kind {
    PLUS,
    MINUS,
    MULT,
    DIV,
    MOD,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_LSHIFT,
    BIT_RSHIFT,
    AND,
    OR,
    XOR,
    POW,
    MIN,
    MAX,
    RATE,
    FRACTION,

    /********************************************************
     * Relational operators
     */
    LT,
    LE,
    EQ,
    NEQ,
    GE,
    GT,

    /********************************************************
     * Unary operators
     */
    NOT,
    FORALL,
    EXISTS,
    SUM,

    /********************************************************
     * Built-in functions
     */
    ABS_F,
    FABS_F,
    FMOD_F,
    FMA_F,
    FMAX_F,
    FMIN_F,
    FDIM_F,
    EXP_F,
    EXP2_F,
    EXPM1_F,
    LN_F,
    LOG_F,
    LOG10_F,
    LOG2_F,
    LOG1P_F,
    POW_F,
    SQRT_F,
    CBRT_F,
    HYPOT_F,
    SIN_F,
    COS_F,
    TAN_F,
    ASIN_F,
    ACOS_F,
    ATAN_F,
    ATAN2_F,
    SINH_F,
    COSH_F,
    TANH_F,
    ASINH_F,
    ACOSH_F,
    ATANH_F,
    ERF_F,
    ERFC_F,
    TGAMMA_F,
    LGAMMA_F,
    CEIL_F,
    FLOOR_F,
    TRUNC_F,
    ROUND_F,
    FINT_F,
    LDEXP_F,
    ILOGB_F,
    LOGB_F,
    NEXT_AFTER_F,
    COPY_SIGN_F,
    FP_CLASSIFY_F,
    IS_FINITE_F,
    IS_INF_F,
    IS_NAN_F,
    IS_NORMAL_F,
    SIGNBIT_F,
    IS_UNORDERED_F,
    RANDOM_F,
    RANDOM_ARCSINE_F,
    RANDOM_BETA_F,
    RANDOM_GAMMA_F,
    RANDOM_NORMAL_F,
    RANDOM_POISSON_F,
    RANDOM_TRI_F,
    RANDOM_WEIBULL_F,

    /********************************************************
     * Assignment operators
     */
    ASSIGN,
    ASS_PLUS,
    ASS_MINUS,
    ASS_DIV,
    ASS_MOD,
    ASS_MULT,
    ASS_AND,
    ASS_OR,
    ASS_XOR,
    ASS_LSHIFT,
    ASS_RSHIFT,

    /*******************************************************
     * CTL Quantifiers
     */
    EF,
    EG,
    AF,
    AG,
    LEADS_TO,
    A_UNTIL,
    A_WEAK_UNTIL,
    A_BUCHI,
    PMAX,
    PROBA_MIN_BOX,
    PROBA_MIN_DIAMOND,
    PROBA_BOX,      ///< probability estimation of staying on course
    PROBA_DIAMOND,  ///< probability estimation of eventuality
    PROBA_CMP,      ///< probability comparison without estimation
    PROBA_EXP,      ///< expected value estimation
    SIMULATE,
    SIMULATEREACH,
    BOX,
    DIAMOND,

    /*******************************************************
     * Control Synthesis Operator
     */
    CONTROL,
    PO_CONTROL,
    EF_CONTROL,
    CONTROL_TOPT,
    CONTROL_TOPT_DEF1,
    CONTROL_TOPT_DEF2,
    SMC_CONTROL,
    MIN_EXP,
    MAX_EXP,
    SAVE_STRAT,
    LOAD_STRAT,

    /*******************************************************
     * Get supremum, infimum or (generalized) bounds for variable/clock values
     */
    SUP_VAR,
    INF_VAR,
    BOUNDS_VAR,

    /*******************************************************
     * Verify a LSC scenario
     */
    SCENARIO,   // identifier of the LSC instance
    SCENARIO2,  // scenario property of size 2 "obsTA.lmin --> obsTA.lmax"

    /*******************************************************
     * Additional constants used by ExpressionProgram's and
     * the TypeCheckBuilder (but not by the parser, although
     * some of then ought to be used, FIXME).
     */
    // VAR_INDEX is only used during compilation when creating processes.
    // this is the *only* type to be sent as a ref for shorthand process
    // initialization or a spawn.
    VAR_INDEX,
    IDENTIFIER,
    CONSTANT,
    ARRAY,
    POST_INCREMENT,
    PRE_INCREMENT,
    POST_DECREMENT,
    PRE_DECREMENT,
    UNARY_MINUS,
    LIST,
    DOT,
    INLINE_IF,
    COMMA,
    SYNC,
    DEADLOCK,
    FUN_CALL,
    FUN_CALL_EXT,

    /*******************************************************
     * Types
     */
    UNKNOWN,
    VOID_TYPE,
    CLOCK,
    INT,
    DOUBLE,
    STRING,
    BOOL,
    SCALAR,
    LOCATION,
    LOCATION_EXPR,
    CHANNEL,
    COST,
    INVARIANT,
    INVARIANT_WR,
    GUARD,
    DIFF,
    CONSTRAINT,
    FORMULA,
    BRANCHPOINT,
    PROBABILITY,
    // LSC
    INSTANCE_LINE,
    MESSAGE,
    CONDITION,
    UPDATE,

    RANGE,
    LABEL,
    RECORD,
    REF,
    URGENT,
    COMMITTED,
    BROADCAST,
    HYBRID,
    TYPEDEF,
    PROCESS,
    PROCESS_SET,
    INSTANCE,  // TA template (instantiated or not)
    SYSTEM_META,
    FUNCTION,
    FUNCTION_EXTERNAL,
    LSC_INSTANCE,  // LSC template (instantiated or not)

    /******************************************************
     * MITL Extension
     */
    MITL_FORMULA,
    MITL_RELEASE,
    MITL_UNTIL,
    MITL_CONJ,
    MITL_DISJ,
    MITL_NEXT,
    MITL_ATOM,
    MITL_EXISTS,
    MITL_FORALL,
    /*Dynamic */
    SPAWN,
    EXIT,
    NUMOF,
    FORALL_DYNAMIC,
    EXISTS_DYNAMIC,
    SUM_DYNAMIC,
    FOREACH_DYNAMIC,
    DYNAMIC_EVAL,
    PROCESS_VAR,
    DOUBLE_INV_GUARD,

};

/**********************************************************
 * Synchronisations:
 */
enum Synchronisation { SYNC_QUE, SYNC_BANG, SYNC_CSP };
}  // namespace Constants

/** Type for specifying which XTA part to parse (syntax switch) */
enum XTAPart {
    S_XTA,  // entire document
    S_DECLARATION,
    S_LOCAL_DECL,
    S_INST,
    S_SYSTEM,
    S_PARAMETERS,
    S_INVARIANT,
    S_EXPONENTIAL_RATE,
    S_SELECT,
    S_GUARD,
    S_SYNC,
    S_ASSIGN,
    S_EXPRESSION,
    S_EXPRESSION_LIST,
    S_PROPERTY,
    S_XTA_PROCESS,
    S_PROBABILITY,
    /*LSC*/ S_INSTANCE_LINE,
    S_MESSAGE,
    S_UPDATE,
    S_CONDITION
};

}  // namespace UTAP

#endif
