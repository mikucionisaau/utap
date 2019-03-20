// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2004 Uppsala University and Aalborg University.
   
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

#ifndef UTAP_SYSTEMBUILDER_H
#define UTAP_SYSTEMBUILDER_H

#include <cassert>
#include <vector>
#include <inttypes.h>

#include "utap/expressionbuilder.h"
#include "utap/utap.h"

namespace UTAP
{
    /**
     * This class constructs a TimedAutomataSystem. It avoids as much
     * type checking as possible - type checking should be done with
     * the TypeChecker class. However some checks are more convenient
     * to do in SystemBuilder:
     *
     *  - states are not both committed and urgent 
     *  - the source and target of an edge is a state
     *  - the dot operator is applied to a process or a structure.
     *  - functions are not recursive
     *  - only declared functions are called (ExpressionBuilder)
     *  - functions are called with the correct number of arguments
     *  - the initial state of a template is actually declared as a state
     *  - templates in instantiations have been declared
     *  - identifiers are not declared twice in the same scope
     *  - type names do exist and are declared as types
     *  - processes in the system line have been declared
     *
     * Left hand side expressions are assigned the correct type by
     * SystemBuilder; if not it would be difficult to represent
     * dot-expressions.
     *
     * SystemBuilder does not
     *  - check the correctness of variable initialisers, nor
     *    does it complete variable initialisers
     *  - type check expressions
     *  - check if arguments to functions or templates match the
     *    formal parameters
     *  - check if something is a proper left hand side value
     *  - check if things are assignment compatible
     *  - check conflicting use of synchronisations and guards on edge
     *  - handle properties
     *
     * Use TypeChecker for these things.
     */
    class SystemBuilder : public ExpressionBuilder
    {
    protected:
	/** The current channel priority level. */
	int32_t currentChanPriority;

	/** The current process priority level. */
	int32_t currentProcPriority;

	/** 
	 * The \a params frame is used temporarily during parameter
	 * parsing.
	 */
	frame_t params;

	/** The function currently being parsed. */
	function_t *currentFun;

	/** The edge under construction. */
	edge_t *currentEdge;

	/** Stack of nested statement blocks. */
	std::vector<BlockStatement*> blocks; 

	/** The types of a struct. */
	std::vector<type_t> fields;

	/** The labels of a struct. */
	std::vector<std::string> labels;

	//
	// Method for handling types
	//

	declarations_t *getCurrentDeclarationBlock();

    public:
	SystemBuilder(TimedAutomataSystem *);

	virtual void typeArrayOfSize(size_t);
	virtual void typeArrayOfType(size_t);
	virtual void typeStruct(PREFIX, uint32_t fields);
	virtual void structField(const char* name); 
	virtual void declTypeDef(const char* name); 
	virtual void declVar(const char* name, bool init); 
	virtual void declInitialiserList(uint32_t num); 
	virtual void declFieldInit(const char* name); 
	virtual void declProgress(bool);
	virtual void declParameter(const char* name, bool);
	virtual void declFuncBegin(const char* name);
	virtual void declFuncEnd();
	virtual void procBegin(const char* name);
	virtual void procEnd(); 
	virtual void procState(const char* name, bool hasInvariant); 
	virtual void procStateCommit(const char* name); 
	virtual void procStateUrgent(const char* name); 
	virtual void procStateWinning(const char* name);
	virtual void procStateLosing(const char* name); 
	virtual void procStateInit(const char* name); 
        virtual void procEdgeBegin(const char* from, const char* to, const bool control);
	virtual void procEdgeEnd(const char* from, const char* to); 
	virtual void procSelect(const char *id);
	virtual void procGuard();
	virtual void procSync(Constants::synchronisation_t type);
	virtual void procUpdate();
	virtual void blockBegin();
	virtual void blockEnd();
	virtual void emptyStatement();
	virtual void forBegin();
	virtual void forEnd(); 
	virtual void iterationBegin(const char *name);
	virtual void iterationEnd(const char *name);
	virtual void whileBegin();
	virtual void whileEnd();
	virtual void doWhileBegin();
	virtual void doWhileEnd();
	virtual void ifBegin();
	virtual void ifElse();
	virtual void ifEnd(bool); 
	virtual void exprStatement(); 
	virtual void returnStatement(bool);
	virtual void exprCallBegin();
	virtual void instantiationBegin(const char*, size_t, const char*);
	virtual void instantiationEnd(
	    const char *, size_t, const char *, size_t);
	virtual void process(const char*);
	virtual void done();    
	virtual void beforeUpdate();
	virtual void afterUpdate();
	virtual void incProcPriority();
	virtual void incChanPriority();
	virtual void chanPriority();
	virtual void procPriority(const char*);
	virtual void defaultChanPriority();
    };
}
#endif
