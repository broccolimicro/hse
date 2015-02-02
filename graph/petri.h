/*
 * petri.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include "marked_graph.h"

#ifndef graph_petri_h
#define graph_petri_h

namespace graph
{
struct petri : graph
{
	enum
	{
		places = 0,
		transitions = 1
	};

	struct transition : node
	{
		boolean::cover action;
	};

	struct place : node
	{
		boolean::cover predicate;
	};

	struct token
	{
		graph::iterator index;
	};

	struct marking
	{
		vector<token*> tokens;
		marked_graph *base;
	};

	marking M0;

	iterator create_transition(boolean::cover action);
	vector<iterator> create_transitions(vector<boolean::cover> actions);
	iterator create_place();
	vector<iterator> create_places(int num);

};
}

#endif
