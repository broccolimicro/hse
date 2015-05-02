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
	simulator(graph *base);
	~simulator();

	struct
	{
		vector<local_token> tokens;
		vector<enabled_transition> ready;
	} local;

	struct
	{
		// structures for handling the environment
		// indexes for places in the graph
		vector<global_token> tail;
		vector<global_token> head;

		// indices for transitions in the graph between the head and the tail
		deque<term_index> body;

		vector<enabled_transition> ready_head;
		vector<enabled_transition> ready_tail;
	} global;

	graph *base;

	int enabled(bool sorted = false);
	void fire(int index);

	int enabled_global(bool sorted = false);
	int disabled_global(bool sorted = false);
	void begin(int index);
	void end(int index);

	struct state
	{
		vector<int> tokens;
		boolean::cube encoding;
	};

	state get_state();
	vector<int> get_choice(int i);
};

bool operator<(simulator::state s1, simulator::state s2);
bool operator>(simulator::state s1, simulator::state s2);
bool operator<=(simulator::state s1, simulator::state s2);
bool operator>=(simulator::state s1, simulator::state s2);
bool operator==(simulator::state s1, simulator::state s2);
bool operator!=(simulator::state s1, simulator::state s2);

}

#endif
