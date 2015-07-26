/*
 * simulator.h
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <ucs/variable.h>
#include "state.h"

#ifndef hse_simulator_h
#define hse_simulator_h

namespace hse
{

struct graph;

struct instability : enabled_transition
{
	instability();
	instability(const enabled_transition &cause);
	~instability();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

struct interference : pair<enabled_transition, enabled_transition>
{
	interference();
	interference(const enabled_transition &first, const enabled_transition &second);
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
	deadlock(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings);
	deadlock(vector<local_token> tokens, deque<enabled_environment> environment, boolean::cover encodings);
	~deadlock();

	string to_string(const ucs::variable_set &v);
};

struct simulator
{
	simulator();
	simulator(const graph *base, const ucs::variable_set *variables, state initial, int index, bool environment);
	~simulator();

	vector<instability> instability_errors;
	vector<interference> interference_errors;
	vector<mutex> mutex_errors;
	vector<term_index> unacknowledged;
	term_index last;

	const graph *base;
	const ucs::variable_set *variables;

	boolean::cube encoding;
	boolean::cube global;

	struct
	{
		vector<local_token> tokens;
		vector<enabled_transition> ready;
	} local;

	struct
	{
		// structures for handling the environment
		// indexes for places in the graph
		vector<int> tail;
		vector<remote_token> head;

		// indices for transitions in the graph between the head and the tail
		deque<enabled_environment> body;

		vector<enabled_environment> ready;
	} remote;

	int enabled(bool sorted = false);
	enabled_transition fire(int index);

	int possible(bool sorted = false);
	enabled_environment begin(int index);
	void end();
	void environment();

	void merge_errors(const simulator &sim);
	state get_state();

	vector<pair<int, int> > get_vacuous_choices();
};

}

#endif
