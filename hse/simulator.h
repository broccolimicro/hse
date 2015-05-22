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
	simulator(graph *base, int i);
	~simulator();

	vector<instability> unstable;
	vector<interference> interfering;
	term_index last;

	vector<token> tokens;
	vector<enabled_transition> ready;
	boolean::cube global;
	graph *base;

	int enabled(bool sorted = false);
	void fire(int index);

	struct state
	{
		vector<int> tokens;
		boolean::cube encoding;

		pair<vector<int>, boolean::cover> to_pair();
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
