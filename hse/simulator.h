/*
 * simulator.h
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <ucs/variable.h>
#include <petri/state.h>
#include <petri/simulator.h>
#include "graph.h"
#include "state.h"

#ifndef hse_simulator_h
#define hse_simulator_h

namespace hse
{

struct instability : enabled_transition
{
	instability();
	instability(const enabled_transition &cause);
	~instability();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

struct interference : pair<term_index, term_index>
{
	interference();
	interference(const term_index &first, const term_index &second);
	~interference();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

struct mutex : pair<enabled_transition, enabled_transition>
{
	mutex();
	mutex(const enabled_transition &first, const enabled_transition &second);
	~mutex();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

struct deadlock : state
{
	deadlock();
	deadlock(const state &s);
	deadlock(vector<token> tokens, boolean::cube encodings);
	~deadlock();

	string to_string(const ucs::variable_set &v);
};

struct simulator
{
	typedef petri::simulator<hse::place, hse::transition, petri::token, hse::state> super;

	simulator();
	simulator(const graph *base, const ucs::variable_set *variables, state initial);
	~simulator();

	vector<instability> instability_errors;
	vector<interference> interference_errors;
	vector<mutex> mutex_errors;
	vector<term_index> unacknowledged;
	list<pair<boolean::cube, term_index> > history;

	const graph *base;
	const ucs::variable_set *variables;

	boolean::cube encoding;
	boolean::cube global;

	vector<token> tokens;
	vector<enabled_transition> loaded;

	// ready[i].first indexes the "loaded" array
	// ready[i].second indexes the cube in the action of that transition
	vector<pair<int, int> > ready;

	int enabled(bool sorted = false);
	enabled_transition fire(int index);

	void merge_errors(const simulator &sim);
	state get_state();
	state get_key();
	vector<pair<int, int> > get_choices();
};

}

#endif
