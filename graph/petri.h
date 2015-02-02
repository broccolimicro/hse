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
struct petri : marked_graph
{
	struct transition : node
	{
		boolean::cover action;
	};

	struct place : node
	{
		boolean::cover predicate;
	};

	enum
	{
		places = 0,
		transitions = 1
	};

	iterator create_transition(boolean::cover action);
	vector<iterator> create_transitions(vector<boolean::cover> actions);
	iterator create_place();
	vector<iterator> create_places(int num);

};
}

#endif
