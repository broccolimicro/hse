/*
 * graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cover.h>
#include <ucs/variable.h>
#include <petri/graph.h>

#include "state.h"

#include "simulator.h"

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{

using petri::iterator;
using petri::parallel;
using petri::choice;
using petri::sequence;

struct place : petri::place
{
	place();
	place(boolean::cover predicate);
	~place();

	boolean::cover predicate;
	boolean::cover effective;
	boolean::cube mask;

	static place merge(int composition, const place &p0, const place &p1);
};

struct transition : petri::transition
{
	transition();
	transition(int behavior, boolean::cover local_action = 1, boolean::cover remote_action = 1);
	~transition();

	enum
	{
		passive = 0,
		active = 1
	};

	boolean::cover local_action;
	boolean::cover remote_action;
	int behavior;

	transition subdivide(int term) const;

	static transition merge(int composition, const transition &t0, const transition &t1);
	static bool mergeable(int composition, const transition &t0, const transition &t1);

	bool is_infeasible();
	bool is_vacuous();
};

struct graph : petri::graph<hse::place, hse::transition, petri::token, hse::state>
{
	typedef petri::graph<hse::place, hse::transition, petri::token, hse::state> super;

	graph();
	~graph();

	void post_process(const ucs::variable_set &variables, bool proper_nesting = false);
	void check_variables(const ucs::variable_set &variables);
};

}

#endif
