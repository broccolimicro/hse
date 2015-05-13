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

simulator::simulator(graph *base, int i)
{
	this->base = base;
	tokens = base->source[i];
	for (int j = 0; j < (int)tokens.size(); j++)
		global &= tokens[j].state;
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
		sort(tokens.begin(), tokens.end());

	vector<enabled_transition> result;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of local at the input places
	for (int i = 0; i < (int)base->arcs[place::type].size(); i++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		if (find(disabled.begin(), disabled.end(), base->arcs[place::type][i].to.index) == disabled.end())
		{
			// Find the index of this transition (if any) in the result pool
			int k = 0;
			while (k < (int)result.size() && result[k].index != base->arcs[place::type][i].to.index)
				k++;

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)tokens.size() && !found; j++)
				if (base->arcs[place::type][i].from.index == tokens[j].index &&
					(k >= (int)result.size() || find(result[k].tokens.begin(), result[k].tokens.end(), j) == result[k].tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (k >= (int)result.size())
						result.push_back(enabled_transition(base->arcs[place::type][i].to.index));

					result[k].tokens.push_back(j);
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

	// Now we have a list of transitions that have enough tokens to consume. We need to figure out their state
	for (int i = 0; i < (int)result.size(); i++)
		for (int j = 0; j < (int)result[i].tokens.size(); j++)
			result[i].state &= tokens[result[i].tokens[j]].state;

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
	for (int i = 0; i < (int)ready.size(); i++)
		if (base->transitions[ready[i].index].behavior == transition::active)
			for (int j = i+1; j < (int)ready.size(); j++)
				if (base->transitions[ready[j].index].behavior == transition::active &&
					boolean::are_mutex(base->transitions[ready[i].index].action[ready[i].term], base->transitions[ready[j].index].action[ready[j].term]))
					error("", "interfering transitions T" + to_string(ready[i].index) + "." + to_string(ready[i].term) + " and T" + to_string(ready[j].index) + "." + to_string(ready[j].term), __FILE__, __LINE__);

	// Check for unstable transitions
	for (int i = 0, j = 0; i < (int)ready.size() && j < (int)result.size();)
	{
		if (ready[i] < result[j])
		{
			error("", "unstable transition T" + to_string(ready[i].index) + "." + to_string(ready[i].term), __FILE__, __LINE__);
			i++;
		}
		else if (result[j] < ready[i])
			j++;
		else if (ready[i] == result[j])
		{
			i++;
			j++;
		}
	}

	ready = result;
	return ready.size();
}

void simulator::fire(int index)
{
	enabled_transition t = ready[index];

	// Update the tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.tokens.rbegin(), t.tokens.rend());
	for (int i = 0; i < (int)t.tokens.size(); i++)
		tokens.erase(tokens.begin() + t.tokens[i]);

	// Update the state
	if (base->transitions[t.index].behavior == transition::active)
	{
		t.state = boolean::local_transition(t.state, base->transitions[t.index].action[t.term]);
		global = boolean::local_transition(global, base->transitions[t.index].action[t.term]);
		for (int i = 0; i < (int)tokens.size(); i++)
			tokens[i].state = boolean::remote_transition(tokens[i].state, base->transitions[t.index].action[t.term]);
	}
	else if (base->transitions[t.index].behavior == transition::passive)
	{
		boolean::cube temp = t.state & base->transitions[t.index].action[t.term];
		t.state = boolean::remote_transition(temp, global);

		if (temp != t.state)
			warning("", "unstable guard T" + to_string(t.index) + "." + to_string(t.term), __FILE__, __LINE__);
	}

	for (int i = 0; i < (int)next.size(); i++)
		tokens.push_back(token(next[i], t.state));

	// disable any transitions that were dependent on at least one of the same tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)ready.size()-1; i >= 0; i--)
		if (vector_intersection_size(ready[i].tokens, t.tokens) > 0)
			ready.erase(ready.begin() + i);
}

simulator::state simulator::get_state()
{
	simulator::state result;

	result.encoding = global;
	for (int i = 0; i < (int)tokens.size(); i++)
		result.tokens.push_back(tokens[i].index);

	sort(result.tokens.begin(), result.tokens.end());

	return result;
}

bool operator<(simulator::state s1, simulator::state s2)
{
	return (s1.tokens < s2.tokens) || (s1.tokens == s2.tokens && s1.encoding < s2.encoding);
}

bool operator>(simulator::state s1, simulator::state s2)
{
	return (s1.tokens > s2.tokens) || (s1.tokens == s2.tokens && s1.encoding > s2.encoding);
}

bool operator<=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens < s2.tokens) || (s1.tokens == s2.tokens && s1.encoding <= s2.encoding);
}

bool operator>=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens > s2.tokens) || (s1.tokens == s2.tokens && s1.encoding >= s2.encoding);
}

bool operator==(simulator::state s1, simulator::state s2)
{
	return (s1.tokens == s2.tokens && s1.encoding == s2.encoding);
}

bool operator!=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens != s2.tokens || s1.encoding != s2.encoding);
}

}
