/*
 * marking.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "marking.h"
#include "graph.h"

namespace hse
{
token::token()
{
	index = -1;
	state = 0;
}

token::token(int index, boolean::cube state)
{
	this->index = index;
	this->state = state;
}

token::~token()
{

}

bool token::operator<(token t) const
{
	return (index < t.index);
}

enabled_transition::enabled_transition()
{
	this->state = 1;
	this->index = -1;
	this->term = -1;
}

enabled_transition::enabled_transition(int index)
{
	this->state = 1;
	this->index = index;
	this->term = -1;
}

enabled_transition::~enabled_transition()
{

}

marking::marking()
{
	base = NULL;
}

marking::~marking()
{
}

/** enabled()
 *
 * Returns a vector of indices representing the transitions
 * that this marking enabled and the term of each transition
 * that's enabled.
 */
vector<enabled_transition> marking::enabled(bool sorted)
{
	if (!sorted)
		sort(tokens.begin(), tokens.end());

	vector<enabled_transition> result;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of tokens at the input places
	for (int i = 0; i < (int)base->arcs[graph::place].size(); i++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		if (find(disabled.begin(), disabled.end(), base->arcs[graph::place][i].to.index) == disabled.end())
		{
			// Find the index of this transition (if any) in the result pool
			int k;
			for (k = 0; k < (int)result.size() && result[k].index != base->arcs[graph::place][i].to.index; k++);

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)tokens.size() && !found; j++)
				if (base->arcs[graph::place][i].from.index == tokens[j].index && (k >= (int)result.size() || find(result[k].tokens.begin(), result[k].tokens.end(), j) == result[k].tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (k >= (int)result.size())
						result.push_back(enabled_transition(base->arcs[graph::place][i].to.index));

					result[k].tokens.push_back(j);
				}

			// If we didn't find a token at the input place, then we know that this transition can't
			// be enabled. So lets remove this from the list of possibly enabled transitions and
			// remember as much in the disabled list.
			if (!found)
			{
				disabled.push_back(base->arcs[graph::place][i].to.index);
				if (k < (int)result.size())
					result.erase(result.begin() + k);
			}
		}
	}

	// Now we have a list of transitions that have enough tokens to consume. We need to figure out their state
	for (int i = 0; i < (int)result.size(); i++)
		for (int j = 0; j < (int)result[i].tokens.size(); j++)
			result[i].state &= tokens[result[i].tokens[j]].state;

	// Now we need to check all of the terms against the state
	for (int i = (int)result.size()-1; i >= 0; i--)
	{
		for (int j = base->transitions[result[i].index].action.size()-1; j > 0; j--)
			if (base->transitions[result[i].index].type == transition::active || (base->transitions[result[i].index].action[j] & result[i].state) != 0)
			{
				result.push_back(result[i]);
				result.back().term = j;
			}

		if (base->transitions[result[i].index].action.size() > 0 && (base->transitions[result[i].index].type == transition::active || (base->transitions[result[i].index].action[0] & result[i].state) != 0))
			result[i].term = 0;
		else
			result.erase(result.begin() + i);
	}

	return result;
}

void marking::fire(enabled_transition t)
{
	if (base->transitions[t.index].type == transition::active)
		t.state = boolean::transition(t.state, base->transitions[t.index].action[t.term]);
	else if (base->transitions[t.index].type == transition::passive)
		t.state &= base->transitions[t.index].action[t.term];

	vector<int> next = base->next(graph::transition, t.index);

	sort(t.tokens.rbegin(), t.tokens.rend());
	for (int i = 0; i < (int)tokens.size(); i++)
		tokens.erase(tokens.begin() + t.tokens[i]);

	for (int i = 0; i < (int)next.size(); i++)
		tokens.push_back(token(next[i], t.state));
}

}
