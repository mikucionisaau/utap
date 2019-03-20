// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-

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

#ifndef UTAP_TYPESYS_HH
#define UTAP_TYPESYS_HH

#include <vector>
#include <utility>

#include <inttypes.h>

namespace UTAP
{
    typedef int32_t type_t;

    class TypeSystem
    {
    public:
    
	static const type_t VOID = 0;
	static const type_t CLOCK = 1;
	static const type_t INT = 2;
	static const type_t LOCATION = 4;
	static const type_t CLOCATION = 5;
	static const type_t ULOCATION = 6;
	static const type_t CHANNEL = 7;
	static const type_t UCHANNEL = 8;
	static const type_t BCHANNEL = 9;
	static const type_t UBCHANNEL = 10;
	static const type_t TEMPLATE = 11;
	static const type_t FUNCTION = 12;
	static const type_t ARRAY = 13;
	static const type_t RECORD = 14;
	static const type_t NTYPE = 15;
  
	static const type_t DIFF = 16;
	static const type_t INVARIANT = 17;
	static const type_t GUARD = 18;
	static const type_t CONSTRAINT = 19;

	static const type_t PROCESS = 20;

    protected:
	static const int NO_PREDEFINED = 22;
	TypeSystem(const TypeSystem &){};
  
    public:

	type_t addInteger(int min, int max);
	type_t addArray(type_t size, type_t type);
	type_t addRecord(const std::vector<std::pair<char*, type_t> >&);
	type_t addFunction(type_t domain, type_t range);
	type_t addTemplate(type_t);
	type_t addNamedType(type_t);

	type_t makeReference(type_t);
	type_t makeConstant(type_t);
	type_t makeSideEffectFree(type_t);

	bool isReference(type_t) const;
	bool isConstant(type_t) const;
	bool isSideEffectFree(type_t) const;
	type_t getClass(type_t) const;

	const std::pair<int,int> &getIntegerRange(type_t) const;
	const std::vector<std::pair<char*, type_t> > &getRecord(type_t);

	type_t getFirstSubType(type_t) const;
	type_t getSecondSubType(type_t) const;

	type_t clearFlags(type_t type) const {
	    return (type | FLAG_CONST | FLAG_REF | FLAG_SEFREE) ^ FLAG_CONST ^ FLAG_REF ^ FLAG_SEFREE;
	}

	TypeSystem();
	~TypeSystem();

    protected:
	static const int FLAG_CONST = 1<<30;
	static const int FLAG_REF = 1<<29;
	static const int FLAG_SEFREE = 1<<28;

	typedef struct typeinfo_t
	{
	    const type_t type;
	    std::pair<int, int> range;
	    std::pair<type_t, type_t> subtype;
	    std::vector<std::pair<char*, type_t> > *record; // a pointer to a copy

	    typeinfo_t()
		: type(VOID), subtype(std::make_pair(VOID, VOID)), record(NULL)
		{ };

	    typeinfo_t(type_t type)
		: type(type), subtype(std::make_pair(VOID, VOID)), record(NULL)
		{ };

	    typeinfo_t(type_t type, int firstSubType, int secondSubType)
		: type(type),
		  subtype(std::make_pair(firstSubType, secondSubType)), record(NULL)
		{ };

	    typeinfo_t(type_t type, std::vector<std::pair<char*, int> >* rec)
		: type(type), subtype(std::make_pair(VOID, VOID)), record(rec) 
		{ };

	    ~typeinfo_t(){
		if (record != NULL) {
		    while (!record->empty()) {
			free(record->back().first); // after strdup in addRecord();
			record->pop_back();
		    }
		    delete record;
		}
	    }
	};

	// dynamic allocation with new: no operator= for const
	std::vector<typeinfo_t*> types; 

    };

}

#endif