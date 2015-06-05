/*
 * simulator.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "simulator.h"
#include <common/text.h>
#include <common/message.h>
#include <boolean/variable.h>
#include <interpret_boolean/export.h>

namespace hse
{

simulator::simulator()
{
	base = NULL;
	variables = NULL;
}

simulator::simulator(const graph *base, const boolean::variable_set *variables, int i, bool environment)
{
	this->base = base;
	this->variables = variables;
	if (base != NULL)
	{
		int j = i;
		i = 0;
		while (i < (int)base->source.size() && j > (int)base->source[i].encodings.cubes.size())
		{
			j -= (int)base->source[i].encodings.cubes.size();
			i++;
		}

		encoding = base->source[i].encodings[j];
		global = base->source[i].encodings[j];
		for (int k = 0; k < (int)base->source[i].tokens.size(); k++)
		{
			if (environment && base->source[i].tokens[k].remotable)
			{
				//cout << "Remote " << base->source[i][j].index << endl;
				remote.head.push_back(base->source[i].tokens[k]);
				remote.tail.push_back(base->source[i].tokens[k]);
			}
			else
				local.tokens.push_back(base->source[i].tokens[k]);
		}
	}
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
	if (base == NULL || local.tokens.size() == 0)
		return 0;

	if (!sorted)
		sort(local.tokens.begin(), local.tokens.end());

	vector<enabled_transition> potential;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of local at the input places
	potential.reserve(local.tokens.size()*2);
	disabled.reserve(base->transitions.size());
	for (vector<arc>::const_iterator a = base->arcs[place::type].begin(); a != base->arcs[place::type].end(); a++)
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

	// Now we have a list of transitions that have enough tokens to consume. We need to figure out their state and their guarding transitions
	for (int i = 0; i < (int)potential.size(); i++)
	{
		for (int j = 0; j < (int)potential[i].tokens.size(); j++)
			potential[i].guard.insert(potential[i].guard.end(), local.tokens[potential[i].tokens[j]].guard.begin(), local.tokens[potential[i].tokens[j]].guard.end());

		sort(potential[i].guard.begin(), potential[i].guard.end());
		potential[i].guard.resize(unique(potential[i].guard.begin(), potential[i].guard.end()) - potential[i].guard.begin());
	}

	// Now we need to check all of the terms against the state and the guards
	vector<enabled_transition> result;
	result.reserve(potential.size()*2);
	for (int i = 0; i < (int)potential.size(); i++)
	{
		for (int j = 0; j < base->transitions[potential[i].index].local_action.size(); j++)
		{
			bool pass = true;
			bool vacuous = false;

			if (base->transitions[potential[i].index].behavior == transition::active)
			{
				// If this transition is vacuous then its enabled even if its incoming guards are not firing
				// This is global and not encoding for the following reason: if there are two equivalent assignments
				// on the same net: x'0+ and x'1+ such that x'0+ happens first and x'1+ happens before it can observe
				// the value on x'0, is x'1+ vacuous? I assume yes because the two assignments are not differentiable in
				// the event rule system.
				vacuous = (local_transition(global, base->transitions[potential[i].index].local_action.cubes[j]) == global);

				// Now we need to check all of the guards leading up to this transition
				for (vector<int>::iterator l = potential[i].guard.begin(); l != potential[i].guard.end() && pass && !vacuous; l++)
				{
					pass = false;
					for (int m = 0; m < (int)base->transitions[*l].local_action.cubes.size() && !pass; m++)
						pass = (remote_transition(base->transitions[*l].local_action.cubes[m], global) == base->transitions[*l].local_action.cubes[m]);
				}
			}
			else
				pass = !are_mutex(base->transitions[potential[i].index].local_action[j], encoding);

			if (pass)
			{
				result.push_back(potential[i]);
				result.back().term = j;
				result.back().vacuous = vacuous;
			}
		}
	}

	// Check for interfering transitions
	for (int i = 0; i < (int)result.size(); i++)
		if (base->transitions[result[i].index].behavior == transition::active)
			for (int j = i+1; j < (int)result.size(); j++)
				if (base->transitions[result[j].index].behavior == transition::active &&
					boolean::are_mutex(base->transitions[result[i].index].local_action[result[i].term], base->transitions[result[j].index].local_action[result[j].term]))
				{
					interference err(result[i], result[j]);
					vector<interference>::iterator loc = lower_bound(interfering.begin(), interfering.end(), err);
					if (loc == interfering.end() || *loc != err)
					{
						interfering.insert(loc, err);
						error("", err.to_string(*base, *variables), __FILE__, __LINE__);
					}
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
				{
					unstable.insert(loc, err);
					error("", err.to_string(*base, *variables), __FILE__, __LINE__);
				}
			}
			else if (base->transitions[local.ready[i].index].behavior == hse::transition::passive)
			{
				instability err = instability(local.ready[i], local.ready[i].history);
				vector<instability>::iterator loc = lower_bound(unstable.begin(), unstable.end(), err);
				if (loc == unstable.end() || *loc != err)
				{
					unstable.insert(loc, err);
					warning("", err.to_string(*base, *variables), __FILE__, __LINE__);
				}
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

boolean::cube simulator::fire(int index)
{
	if (base == NULL || index >= (int)local.ready.size())
		return boolean::cube();

	enabled_transition t = local.ready[index];
	//cout << "  T" << t.index << "." << t.term << endl;

	// Update the local.tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.tokens.rbegin(), t.tokens.rend());
	bool remotable = true;
	for (int i = 0; i < (int)t.tokens.size(); i++)
	{
		remotable = remotable && local.tokens[t.tokens[i]].remotable;
		local.tokens.erase(local.tokens.begin() + t.tokens[i]);
	}

	// Update the state
	if (base->transitions[t.index].behavior == transition::active)
	{
		encoding = local_transition(encoding, base->transitions[t.index].local_action[t.term]);
		if (t.term < base->transitions[t.index].remote_action.size())
		{
			global = local_transition(encoding, base->transitions[t.index].remote_action[t.term]);
			encoding = remote_transition(encoding, global);
		}
		t.guard.clear();
	}
	else if (base->transitions[t.index].behavior == transition::passive)
		encoding = remote_transition(encoding & base->transitions[t.index].local_action[t.term], global);

	t.guard.push_back(t.index);

	sort(t.guard.begin(), t.guard.end());
	t.guard.resize(unique(t.guard.begin(), t.guard.end()) - t.guard.begin());

	for (int i = 0; i < (int)next.size(); i++)
		local.tokens.push_back(local_token(next[i], t.guard, remotable));

	// disable any transitions that were dependent on at least one of the same local.tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)local.ready.size()-1; i >= 0; i--)
		if (vector_intersection_size(local.ready[i].tokens, t.tokens) > 0)
			local.ready.erase(local.ready.begin() + i);

	for (int i = 0; i < (int)local.ready.size(); i++)
		local.ready[i].history.push_back(t);

	last = t;
	return base->transitions[t.index].local_action[t.term];
}

int simulator::possible(bool sorted)
{
	if (base == NULL || remote.head.size() == 0)
		return 0;

	if (!sorted)
		sort(remote.head.begin(), remote.head.end());

	vector<enabled_environment> potential;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of remote at the input places
	potential.reserve(remote.head.size()*2);
	disabled.reserve(base->transitions.size());
	for (vector<arc>::const_iterator a = base->arcs[place::type].begin(); a != base->arcs[place::type].end(); a++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		vector<int>::iterator d = lower_bound(disabled.begin(), disabled.end(), a->to.index);
		bool d_invalid = (d == disabled.end() || *d != a->to.index);

		if (d_invalid)
		{
			// Find the index of this transition (if any) in the potential pool
			vector<enabled_environment>::iterator e = lower_bound(potential.begin(), potential.end(), term_index(a->to.index, 0));
			bool e_invalid = (e == potential.end() || e->index != a->to.index);

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)remote.head.size() && !found; j++)
				if (a->from.index == remote.head[j].index &&
					(e_invalid || find(e->tokens.begin(), e->tokens.end(), j) == e->tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (e_invalid)
						e = potential.insert(e, enabled_environment(a->to.index));

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

	// Now we need to check all of the terms against the state and the guard
	vector<enabled_environment> result;
	result.reserve(potential.size()*2);
	for (int i = 0; i < (int)potential.size(); i++)
	{
		for (int j = 0; j < base->transitions[potential[i].index].local_action.size(); j++)
		{
			if (base->transitions[potential[i].index].behavior == transition::active ||
				!are_mutex(base->transitions[potential[i].index].local_action[j], encoding))
			{
				result.push_back(potential[i]);
				result.back().term = j;

				if (find(remote.body.begin(), remote.body.end(), result.back()) != remote.body.end())
				{
					term_index err = remote.body[0];
					vector<term_index>::iterator loc = lower_bound(unacknowledged.begin(), unacknowledged.end(), err);
					if (loc == unacknowledged.end() || *loc != err)
					{
						unacknowledged.insert(loc, err);
						error("", "unacknowledged transition " + remote.body[0].to_string(*base, *variables), __FILE__, __LINE__);
					}
				}
			}
		}
	}

	remote.ready = result;
	return remote.ready.size();
}

void simulator::begin(int index)
{
	if (base == NULL || index >= (int)remote.ready.size())
		return;

	enabled_environment t = remote.ready[index];

	// Update the tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.tokens.rbegin(), t.tokens.rend());
	for (int i = 0; i < (int)t.tokens.size(); i++)
		remote.head.erase(remote.head.begin() + t.tokens[i]);

	for (int i = 0; i < (int)next.size(); i++)
		remote.head.push_back(remote_token(next[i]));

	remote.body.push_back(t);

	/*cout << "+ T" << t.index << "." << t.term << " {";
	for (int i = 0; i < (int)remote.head.size(); i++)
		cout << "P" << remote.head[i].index << " ";
	cout << "} -> {";
	for (int i = 0; i < (int)remote.body.size(); i++)
		cout << "T" << remote.body[i].index << "." << remote.body[i].term << " ";
	cout << "} -> {";
	for (int i = 0; i < (int)remote.tail.size(); i++)
		cout << "P" << remote.tail[i].index << " ";
	cout << "}" << endl;*/
}

void simulator::end()
{
	if (base == NULL)
		return;

	sort(remote.tail.begin(), remote.tail.end());

	vector<int> history;
	// We need to be ware of the case where the head loops back around and meets up with the tail.
	// If this were to happen, it would look like there might be a choice in the rollback of the tail,
	// which should never happen.
	deque<term_index>::iterator iter = remote.body.begin();
	vector<int> p;
	if (iter != remote.body.end())
	{
		p = base->prev(hse::transition::type, iter->index);
		sort(p.begin(), p.end());
	}

	// We know that we have reached one of these cases if we reach a transition in the body which
	// would create such a choice with another transition earlier in the body that has not yet
	// fired. At which point, we stop looking.
	while (iter != remote.body.end() && vector_intersection_size(history, p) == 0)
	{
		vector<int> tokens;
		bool enabled = true;
		for (int j = 0, k = 0; k < (int)p.size() && enabled;)
		{
			if (j >= (int)remote.tail.size() || remote.tail[j].index > p[k])
				enabled = false;
			else if (remote.tail[j].index < p[k])
				j++;
			else if (remote.tail[j].index == p[k])
			{
				tokens.push_back(j);
				j++;
				k++;
			}
		}

		//cout << "Checking T" << iter->index << "." << iter->term << " " << encoding << " " << base->transitions[iter->index].action[iter->term] << " " << local_transition(encoding, base->transitions[iter->index].action[iter->term]) << endl;
		// The tail moves up only if it must.
		// TODO this is wrong... I need to somehow collapse the state
		if (enabled &&
			((base->transitions[iter->index].behavior == transition::active && local_transition(global, base->transitions[iter->index].local_action[iter->term]) == global) ||
			 (base->transitions[iter->index].behavior == transition::passive && (global & base->transitions[iter->index].local_action[iter->term]) == global)))
		{
			/*cout << "- T" << iter->index << "." << iter->term << " {";
			for (int i = 0; i < (int)remote.head.size(); i++)
				cout << "P" << remote.head[i].index << " ";
			cout << "} -> {";
			for (int i = 0; i < (int)remote.body.size(); i++)
				if (i != iter - remote.body.begin())
					cout << "T" << remote.body[i].index << "." << remote.body[i].term << " ";
			cout << "} -> {";
			for (int i = 0; i < (int)remote.tail.size(); i++)
				cout << "P" << remote.tail[i].index << " ";
			cout << "}" << endl;*/

			for (int j = (int)tokens.size()-1; j >= 0; j--)
				remote.tail.erase(remote.tail.begin() + tokens[j]);

			vector<int> n = base->next(hse::transition::type, iter->index);
			for (int j = 0; j < (int)n.size(); j++)
				remote.tail.push_back(remote_token(n[j]));
			sort(remote.tail.begin(), remote.tail.end());

			iter = remote.body.erase(iter);
		}
		else
		{
			history.insert(history.end(), p.begin(), p.end());
			sort(history.begin(), history.end());
			iter++;
		}

		if (iter != remote.body.end())
		{
			p = base->prev(hse::transition::type, iter->index);
			sort(p.begin(), p.end());
		}
	}
}

void simulator::environment()
{
	if (base == NULL)
		return;

	boolean::cube local_action = 1;
	boolean::cube remote_action = 1;
	for (int i = 0; i < (int)remote.body.size(); i++)
		if (base->transitions[remote.body[i].index].behavior == transition::active)
		{
			local_action &= base->transitions[remote.body[i].index].local_action[remote.body[i].term];
			remote_action &= base->transitions[remote.body[i].index].remote_action[remote.body[i].term];
		}
	//cout << "Applying Environment " << encoding << " -> " << action << " = " << remote_transition(encoding, action) << endl;
	encoding = local_transition(encoding, local_action).xoutnulls();
	global = local_transition(global, remote_action);
	encoding = remote_transition(encoding, global);
}

void simulator::merge_errors(const simulator &sim)
{
	vector<instability> old_unstable;
	swap(unstable, old_unstable);
	unstable.resize(old_unstable.size() + sim.unstable.size());
	merge(sim.unstable.begin(), sim.unstable.end(), old_unstable.begin(), old_unstable.end(), unstable.begin());
	unstable.resize(unique(unstable.begin(), unstable.end()) - unstable.begin());

	vector<interference> old_interfering;
	swap(interfering, old_interfering);
	interfering.resize(sim.interfering.size() + old_interfering.size());
	merge(sim.interfering.begin(), sim.interfering.end(), old_interfering.begin(), old_interfering.end(), interfering.begin());
	interfering.resize(unique(interfering.begin(), interfering.end()) - interfering.begin());
}

state simulator::get_state()
{
	vector<term_index> body;
	body.insert(body.end(), remote.body.begin(), remote.body.end());
	sort(body.begin(), body.end());
	body.resize(unique(body.begin(), body.end()) - body.begin());
	return state(local.tokens, body, encoding);
}

vector<pair<int, int> > simulator::get_vacuous_choices()
{
	vector<pair<int, int> > vacuous_choices;
	for (int i = 0; i < (int)local.ready.size(); i++)
		if (local.ready[i].vacuous)
			for (int j = i+1; j < (int)local.ready.size(); j++)
				if (local.ready[j].vacuous && vector_intersection_size(local.ready[i].tokens, local.ready[j].tokens) > 0)
					vacuous_choices.push_back(pair<int, int>(i, j));

	return vacuous_choices;
}



}
