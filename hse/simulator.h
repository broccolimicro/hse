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

	boolean::cube global;

	const graph *base;
	const boolean::variable_set *variables;

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

		bool is_active()
		{
			return (head.size() > 0);
		}
	} remote;

	int enabled(bool sorted = false);
	void fire(int index);

	int possible(bool sorted = false);
	void begin(int index);
	void end();
	void environment();

	struct state
	{
		vector<int> tokens;
		vector<term_index> environment;
		vector<boolean::cover> encodings;

		void merge(const state &s);
		bool is_subset_of(const state &s);
	};

	void merge_errors(const simulator &sim);
	state get_state();

	vector<pair<int, int> > get_vacuous_choices();
};

bool operator<(simulator::state s1, simulator::state s2);
bool operator>(simulator::state s1, simulator::state s2);
bool operator<=(simulator::state s1, simulator::state s2);
bool operator>=(simulator::state s1, simulator::state s2);
bool operator==(simulator::state s1, simulator::state s2);
bool operator!=(simulator::state s1, simulator::state s2);

}

#endif
