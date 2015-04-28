/*
 * simulator.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "simulator.h"
#include "common/text.h"
#include "common/message.h"

namespace hse
{

simulator::simulator()
{
	base = NULL;
}

simulator::simulator(graph *base)
{
	this->base = base;
	for (int i = 0; i < (int)base->source.size(); i++)
		local.tokens.push_back(local_token(base->source[i].index, base->reset[0]));
}

simulator::~simulator()
{

}

/** enabled()
 *
 * Returns a vector of indices representing the transitions
 * that this marking enabled and the term of each transition
 * that's enabled.
 */
int simulator::enabled(bool sorted)
{
	if (!sorted)
		sort(local.tokens.begin(), local.tokens.end());

	vector<enabled_transition> result;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of local at the input places
	for (int i = 0; i < (int)base->arcs[place::type].size(); i++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		if (find(disabled.begin(), disabled.end(), base->arcs[place::type][i].to.index) == disabled.end())
		{
			// Find the index of this transition (if any) in the result pool
			int k;
			for (k = 0; k < (int)result.size() && result[k].index != base->arcs[place::type][i].to.index; k++);

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)local.tokens.size() && !found; j++)
				if (base->arcs[place::type][i].from.index == local.tokens[j].index && (k >= (int)result.size() || find(result[k].local_tokens.begin(), result[k].local_tokens.end(), j) == result[k].local_tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (k >= (int)result.size())
						result.push_back(enabled_transition(base->arcs[place::type][i].to.index));

					result[k].local_tokens.push_back(j);
				}

			// If we didn't find a token at the input place, then we know that this transition can't
			// be enabled. So lets remove this from the list of possibly enabled transitions and
			// remember as much in the disabled list.
			if (!found)
			{
				disabled.push_back(base->arcs[place::type][i].to.index);
				if (k < (int)result.size())
					result.erase(result.begin() + k);
			}
		}
	}

	// Now we have a list of transitions that have enough local to consume. We need to figure out their state
	for (int i = 0; i < (int)result.size(); i++)
		for (int j = 0; j < (int)result[i].local_tokens.size(); j++)
			result[i].state &= local.tokens[result[i].local_tokens[j]].state;

	// Now we need to check all of the terms against the state
	for (int i = (int)result.size()-1; i >= 0; i--)
	{
		for (int j = base->transitions[result[i].index].action.size()-1; j > 0; j--)
			if (base->transitions[result[i].index].behavior == transition::active || (base->transitions[result[i].index].action[j] & result[i].state) != 0)
			{
				result.push_back(result[i]);
				result.back().term = j;
			}

		if (base->transitions[result[i].index].action.size() > 0 && (base->transitions[result[i].index].behavior == transition::active || (base->transitions[result[i].index].action[0] & result[i].state) != 0))
			result[i].term = 0;
		else
			result.erase(result.begin() + i);
	}

	sort(result.begin(), result.end());

	// Check for interfering transitions
	for (int i = 0; i < (int)local.ready.size(); i++)
		if (base->transitions[local.ready[i].index].behavior == transition::active)
			for (int j = i+1; j < (int)local.ready.size(); j++)
				if (base->transitions[local.ready[j].index].behavior == transition::active &&
					boolean::are_mutex(base->transitions[local.ready[i].index].action[local.ready[i].term], base->transitions[local.ready[j].index].action[local.ready[j].term]))
					error("", "interfering transitions T" + to_string(local.ready[i].index) + "." + to_string(local.ready[i].term) + " and T" + to_string(local.ready[j].index) + "." + to_string(local.ready[j].term), __FILE__, __LINE__);

	// Check for unstable transitions
	for (int i = 0, j = 0; i < (int)local.ready.size() && j < (int)result.size();)
	{
		if (local.ready[i] < result[j])
		{
			error("", "unstable transition T" + to_string(local.ready[i].index) + "." + to_string(local.ready[i].term), __FILE__, __LINE__);
			i++;
		}
		else if (result[j] < local.ready[i])
			j++;
		else if (local.ready[i] == result[j])
		{
			i++;
			j++;
		}
	}

	local.ready = result;
	return local.ready.size();
}

void simulator::fire(int index)
{
	enabled_transition t = local.ready[index];

	// Update the tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.local_tokens.rbegin(), t.local_tokens.rend());
	for (int i = 0; i < (int)t.local_tokens.size(); i++)
		local.tokens.erase(local.tokens.begin() + t.local_tokens[i]);

	// Update the state
	if (base->transitions[t.index].behavior == transition::active)
	{
		t.state = boolean::local_transition(t.state, base->transitions[t.index].action[t.term]);
		for (int i = 0; i < (int)local.tokens.size(); i++)
			local.tokens[i].state = boolean::remote_transition(local.tokens[i].state, base->transitions[t.index].action[t.term]);
	}
	else if (base->transitions[t.index].behavior == transition::passive)
		t.state &= base->transitions[t.index].action[t.term];

	for (int i = 0; i < (int)next.size(); i++)
		local.tokens.push_back(local_token(next[i], t.state));

	// disable any transitions that were dependent on at least one of the same tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)local.ready.size()-1; i >= 0; i--)
		if (vector_intersection_size(local.ready[i].local_tokens, t.local_tokens) > 0)
			local.ready.erase(local.ready.begin() + i);
}

int simulator::enabled_global(bool sorted)
{


}

int simulator::disabled_global(bool sorted)
{

}

void simulator::begin(int index)
{

}

void simulator::end(int index)
{

}

}
