/*
 * simulator.h
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "token.h"
#include "graph.h"

#ifndef hse_simulator_h
#define hse_simulator_h

namespace hse
{

struct simulator
{
	simulator();
	simulator(graph *base, int i, bool environment);
	~simulator();

	vector<instability> unstable;
	vector<interference> interfering;
	term_index last;

	boolean::cube global;
	graph *base;

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
		vector<boolean::cover> encodings;

		void merge(const state &s);
		bool is_subset_of(const state &s);
	};

	state get_state();
};

bool operator<(simulator::state s1, simulator::state s2);
bool operator>(simulator::state s1, simulator::state s2);
bool operator<=(simulator::state s1, simulator::state s2);
bool operator>=(simulator::state s1, simulator::state s2);
bool operator==(simulator::state s1, simulator::state s2);
bool operator!=(simulator::state s1, simulator::state s2);

}

#endif
