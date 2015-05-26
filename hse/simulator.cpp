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
	local.tokens = base->source[i];
	for (int j = 0; j < (int)local.tokens.size(); j++)
		global &= local.tokens[j].state;
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

	vector<enabled_transition> potential;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of local at the input places
	potential.reserve(local.tokens.size()*2);
	disabled.reserve(base->transitions.size());
	for (vector<arc>::iterator a = base->arcs[place::type].begin(); a != base->arcs[place::type].end(); a++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		vector<int>::iterator d = lower_bound(disabled.begin(), disabled.end(), a->to.index);
		bool d_invalid = (d == disabled.end() || *d != a->to.index);

		if (d_invalid)
		{
			// Find the index of this transition (if any) in the potential pool
			vector<enabled_transition>::iterator e = lower_bound(potential.begin(), potential.end(), term_index(a->to.index, 0));
			bool e_invalid = (e == potential.end() || e->index != a->to.index);

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)local.tokens.size() && !found; j++)
				if (a->from.index == local.tokens[j].index &&
					(e_invalid || find(e->tokens.begin(), e->tokens.end(), j) == e->tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (e_invalid)
						e = potential.insert(e, enabled_transition(a->to.index));

					e->tokens.push_back(j);
				}

			// If we didn't find a token at the input place, then we know that this transition can't
			// be enabled. So lets remove this from the list of possibly enabled transitions and
			// remember as much in the disabled list.
			if (!found)
			{
				disabled.insert(d, a->to.index);
				if (!e_invalid)
					potential.erase(e);
			}
		}
	}

	// Now we have a list of transitions that have enough local.tokens to consume. We need to figure out their state and their guarding transitions
	for (int i = 0; i < (int)potential.size(); i++)
	{
		for (int j = 0; j < (int)potential[i].tokens.size(); j++)
		{
			potential[i].state &= local.tokens[potential[i].tokens[j]].state;
			potential[i].guard.insert(potential[i].guard.end(), local.tokens[potential[i].tokens[j]].guard.begin(), local.tokens[potential[i].tokens[j]].guard.end());
		}

		sort(potential[i].guard.begin(), potential[i].guard.end());
		potential[i].guard.resize(unique(potential[i].guard.begin(), potential[i].guard.end()) - potential[i].guard.begin());
	}

	// Now we need to check all of the terms against the state and the guard
	vector<enabled_transition> result;
	result.reserve(potential.size()*2);
	for (int i = 0; i < (int)potential.size(); i++)
	{
		for (int j = 0; j < base->transitions[potential[i].index].action.size(); j++)
		{
			bool pass = true;

			if (base->transitions[potential[i].index].behavior == transition::active)
			{
				// If this transition is vacuous then its enabled even if its incoming guards are not firing
				bool vacuous = are_mutex(global, ~base->transitions[potential[i].index].action.cubes[j]);

				// Now we need to check all of the guards leading up to this transition
				for (vector<term_index>::iterator l = potential[i].guard.begin(); l != potential[i].guard.end() && pass && !vacuous; l++)
					pass = (boolean::remote_transition(base->transitions[l->index].action.cubes[l->term], global) == base->transitions[l->index].action.cubes[l->term]);
			}
			else
				pass = !are_mutex(base->transitions[potential[i].index].action[j], potential[i].state);

			if (pass)
			{
				result.push_back(potential[i]);
				result.back().term = j;
			}
		}
	}

	// Check for interfering transitions
	for (int i = 0; i < (int)local.ready.size(); i++)
		if (base->transitions[local.ready[i].index].behavior == transition::active)
			for (int j = i+1; j < (int)local.ready.size(); j++)
				if (base->transitions[local.ready[j].index].behavior == transition::active &&
					boolean::are_mutex(base->transitions[local.ready[i].index].action[local.ready[i].term], base->transitions[local.ready[j].index].action[local.ready[j].term]))
				{
					interference err(local.ready[i], local.ready[j]);
					vector<interference>::iterator loc = lower_bound(interfering.begin(), interfering.end(), err);
					if (loc == interfering.end() || *loc != err)
						interfering.insert(loc, err);
				}

	// Check for unstable transitions
	for (int i = 0, j = 0; i < (int)local.ready.size();)
	{
		if (j >= (int)result.size() || local.ready[i] < result[j])
		{
			if (base->transitions[local.ready[i].index].behavior == hse::transition::active)
			{
				instability err = instability(local.ready[i], local.ready[i].history);
				vector<instability>::iterator loc = lower_bound(unstable.begin(), unstable.end(), err);
				if (loc == unstable.end() || *loc != err)
					unstable.insert(loc, err);
			}

			i++;
		}
		else if (result[j] < local.ready[i])
		{
			if (last.index >= 0 && last.term >= 0)
				result[j].history.push_back(last);
			j++;
		}
		else if (local.ready[i] == result[j])
		{
			result[j].history = local.ready[i].history;
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

	// Update the local.tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.tokens.rbegin(), t.tokens.rend());
	for (int i = 0; i < (int)t.tokens.size(); i++)
		local.tokens.erase(local.tokens.begin() + t.tokens[i]);

	// Update the state
	if (base->transitions[t.index].behavior == transition::active)
	{
		t.state = boolean::local_transition(t.state, base->transitions[t.index].action[t.term]);
		global = boolean::local_transition(global, base->transitions[t.index].action[t.term]);
		for (int i = 0; i < (int)local.tokens.size(); i++)
			local.tokens[i].state = boolean::remote_transition(local.tokens[i].state, global);

		t.guard.clear();
	}
	else if (base->transitions[t.index].behavior == transition::passive)
	{
		boolean::cube temp = t.state & base->transitions[t.index].action[t.term];
		t.state = boolean::remote_transition(temp, global);
	}

	t.guard.push_back(t);

	sort(t.guard.begin(), t.guard.end());
	t.guard.resize(unique(t.guard.begin(), t.guard.end()) - t.guard.begin());

	for (int i = 0; i < (int)next.size(); i++)
		local.tokens.push_back(local_token(next[i], t.state, t.guard));

	// disable any transitions that were dependent on at least one of the same local.tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)local.ready.size()-1; i >= 0; i--)
		if (vector_intersection_size(local.ready[i].tokens, t.tokens) > 0)
			local.ready.erase(local.ready.begin() + i);

	for (int i = 0; i < (int)local.ready.size(); i++)
		local.ready[i].history.push_back(t);

	last = t;
}

simulator::state simulator::get_state()
{
	simulator::state result;

	result.encoding = global;
	for (int i = 0; i < (int)local.tokens.size(); i++)
		result.tokens.push_back(local.tokens[i].index);

	sort(result.tokens.begin(), result.tokens.end());

	return result;
}

pair<vector<int>, boolean::cover>simulator::state::to_pair()
{
	return pair<vector<int>, boolean::cover>(tokens, boolean::cover(encoding));
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
