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

struct half_synchronization
{
	half_synchronization();
	~half_synchronization();

	struct
	{
		int index;
		int cube;
	} active, passive;
};

struct synchronization_region
{
	synchronization_region();
	~synchronization_region();

	boolean::cover action;
	vector<petri::iterator> nodes;
};

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
	transition(int behavior, boolean::cover action = 1);
	~transition();

	enum
	{
		passive = 0,
		active = 1
	};

	boolean::cover local_action;
	boolean::cover remote_action;
	int behavior;

	transition subdivide(int term);

	static transition merge(int composition, const transition &t0, const transition &t1);
	static bool mergeable(int composition, const transition &t0, const transition &t1);

	bool is_infeasible();
	bool is_vacuous();
};

struct graph : petri::graph<hse::place, hse::transition, hse::reset_token, hse::state>
{
	typedef petri::graph<hse::place, hse::transition, hse::reset_token, hse::state> super;

	graph();
	~graph();

	vector<half_synchronization> synchronizations;
	vector<synchronization_region> regions;

	void post_process(const ucs::variable_set &variables, bool proper_nesting = false);

	// Generated through syntax
	void synchronize();
	void petrify(const ucs::variable_set &variables);

	// Generated through simulation
	void elaborate(const ucs::variable_set &variables, bool report);
	graph to_state_graph(const ucs::variable_set &variables);
	graph to_petri_net();

	void check_variables(const ucs::variable_set &variables);
};

}

#endif
