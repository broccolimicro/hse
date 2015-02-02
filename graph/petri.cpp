/*
 * petri.cpp
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include "petri.h"

namespace graph
{
petri::iterator petri::create_transition(boolean::cover action)
{
	petri::iterator n = create_node<petri::transition>(transitions);
	at<petri::transition>(n).action = action;
	return n;
}

vector<petri::iterator> petri::create_transitions(vector<boolean::cover> actions)
{
	vector<petri::iterator> n = create_nodes<petri::transition>((int)actions.size(), transitions);
	for (int i = 0; i < (int)n.size(); i++)
		at<petri::transition>(n[i]).action = actions[i];
	return n;
}

petri::iterator petri::create_place()
{
	return create_node<petri::place>(places);
}

vector<petri::iterator> petri::create_places(int num)
{
	return create_nodes<petri::place>(num, places);
}
}
