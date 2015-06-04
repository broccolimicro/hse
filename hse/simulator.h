/*
 * simulator.h
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "token.h"
#include "graph.h"
#include "boolean/variable.h"

#ifndef hse_simulator_h
#define hse_simulator_h

namespace hse
{

struct graph;

struct simulator
{
	simulator();
	simulator(const graph *base, const boolean::variable_set *variables, int i, bool environment);
	~simulator();

	vector<instability> unstable;
	vector<interference> interfering;
	vector<term_index> unacknowledged;
	term_index last;

	const graph *base;
	const boolean::variable_set *variables;

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
		vector<remote_token> tail;
		vector<remote_token> head;

		// indices for transitions in the graph between the head and the tail
		deque<term_index> body;

		vector<enabled_environment> ready;
	} remote;

	int enabled(bool sorted = false);
	boolean::cube fire(int index);

	int possible(bool sorted = false);
	void begin(int index);
	void end();
	void environment();

	void merge_errors(const simulator &sim);
	state get_state();

	vector<pair<int, int> > get_vacuous_choices();
};

}

#endif
