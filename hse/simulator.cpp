/*
 * simulator.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "simulator.h"
#include "graph.h"
#include <common/text.h>
#include <common/message.h>
#include <ucs/variable.h>


namespace hse
{

instability::instability()
{
}

instability::instability(const enabled_transition &cause) : enabled_transition(cause)
{
}

instability::~instability()
{

}

string instability::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	string result;
	if (g.transitions[index].behavior == hse::transition::active)
		result = "unstable assignment " + enabled_transition::to_string(g, v);
	else
		result = "unstable guard " + enabled_transition::to_string(g, v);

	result += " cause: {";

	for (int j = 0; j < (int)history.size(); j++)
	{
		if (j != 0)
			result += "; ";

		result += history[j].to_string(g, v);
	}
	result += "}";
	return result;
}

interference::interference()
{

}

interference::interference(const term_index &first, const term_index &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

interference::~interference()
{

}

string interference::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	return "interfering assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

mutex::mutex()
{

}

mutex::mutex(const enabled_transition &first, const enabled_transition &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

mutex::~mutex()
{

}

string mutex::to_string(const hse::graph &g, const ucs::variable_set &v)
{
	return "non-exclusive guards in deterministic selection for assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

deadlock::deadlock()
{

}

deadlock::deadlock(const state &s) : state(s)
{

}

deadlock::deadlock(vector<token> tokens, boolean::cube encodings) : state(tokens, encodings)
{

}

deadlock::~deadlock()
{

}

string deadlock::to_string(const ucs::variable_set &v)
{
	return "deadlock detected at state " + state::to_string(v);
}

simulator::simulator()
{
	base = NULL;
	variables = NULL;
}

simulator::simulator(const graph *base, const ucs::variable_set *variables, state initial)
{
	//cout << "Reset" << endl;
	this->base = base;
	this->variables = variables;
	if (base != NULL)
	{
		encoding = initial.encodings;
		global = initial.encodings;
		for (int k = 0; k < (int)initial.tokens.size(); k++)
			tokens.push_back(initial.tokens[k]);
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
	if (base == NULL)
	{
		internal("", "NULL pointer to simulator::base", __FILE__, __LINE__);
		return 0;
	}

	// Do a pre-screen
	for (int i = (int)tokens.size()-1; i >= 0; i--)
		if (tokens[i].cause >= 0)
			tokens.erase(tokens.begin() + i);

	if (tokens.size() == 0)
		return 0;

	// Get the list of transitions have have a sufficient number of local at the input places
	vector<enabled_transition> preload;
	vector<enabled_transition> potential;
	vector<int> global_disabled;

	/*for (int i = 0; i < (int)loaded.size(); i++)
	{
		cout << "Preload " << i << ": " << loaded[i].index << " " << (loaded[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)loaded[i].tokens.size(); j++)
			cout << loaded[i].tokens[j] << " ";
		cout << "} {";
		for (int j = 0; j < (int)loaded[i].output_marking.size(); j++)
			cout << loaded[i].output_marking[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/

	int preload_size = 0;
	do
	{
		vector<int> disabled = global_disabled;
		disabled.reserve(max(0, (int)base->transitions.size() - (int)preload.size()));
		preload_size = preload.size();
		for (vector<petri::arc>::const_iterator a = base->arcs[place::type].begin(); a != base->arcs[place::type].end(); a++)
		{
			// Check to see if we haven't already determined that this transition can't be enabled
			vector<int>::iterator d = lower_bound(disabled.begin(), disabled.end(), a->to.index);
			if (d == disabled.end() || *d != a->to.index)
			{
				// Find the index of this transition (if any) in the loaded pool
				bool loaded_found = false;
				for (int i = (int)preload.size()-1; i >= 0; i--)
				{
					if (preload[i].index == a->to.index)
					{
						loaded_found = true;

						// Check to make sure that this enabled transition isn't from a previous iteration.
						if (i >= preload_size)
						{
							// Check to see if there is any token at the input place of this arc and make sure that
							// this token has not already been consumed by this particular transition
							// Also since we only need one token per arc, we can stop once we've found a token
							vector<int> matching_tokens;
							for (int j = 0; j < (int)tokens.size(); j++)
							{
								if (a->from.index == tokens[j].index)
								{
									// We have to implement a recursive...ish algorithm here
									// to check to see if this token has already been used by
									// any of the transitions in the chain
									bool used = false;
									vector<int> test = preload[i].tokens;
									for (int k = 0; k < (int)test.size() && !used; k++)
									{
										used = (test[k] == j);
										if (tokens[test[k]].cause >= 0)
											test.insert(test.end(), preload[tokens[test[k]].cause].tokens.begin(), preload[tokens[test[k]].cause].tokens.end());
									}

									if (!used)
										matching_tokens.push_back(j);
								}
							}

							// there might be more than one matching token
							for (int j = (int)matching_tokens.size()-1; j >= 1; j--)
							{
								preload.push_back(preload[i]);
								preload.back().tokens.push_back(matching_tokens[j]);
							}

							if ((int)matching_tokens.size() > 0)
								preload[i].tokens.push_back(matching_tokens[0]);
							else
							{
								// If we didn't find a token at the input place, then we know that this transition can't
								// be enabled. So lets remove this from the list of possibly enabled transitions and
								// remember as much in the disabled list.
								disabled.insert(d, a->to.index);
								preload.erase(preload.begin() + i);
							}
						}
					}
				}

				// If we didn't find this transition in the loaded list, then this is our
				// first encounter with this transition being possibly enabled for all of the
				// previous iterations. We need to add it to the loaded list.
				if (!loaded_found)
				{
					bool token_found = false;
					for (int j = 0; j < (int)tokens.size(); j++)
						if (a->from.index == tokens[j].index)
						{
							token_found = true;
							preload.push_back(enabled_transition(a->to.index));
							preload.back().tokens.push_back(j);
						}

					if (!token_found)
						disabled.insert(d, a->to.index);
				}
			}
		}

		// Now we need to check the guards of all of the loaded transitions against the state
		for (int i = (int)preload.size()-1; i >= preload_size; i--)
		{
			// If this transition is a guard, we check to see if it passes (either stable or unstable)
			// if it is an assignment, then we check to see if it is vacuous. If either of these hold
			// true, then we can keep going.

			// If this transition is an assignment, then we need to check to make sure all of the
			// previous guards pass before we can execute this assignment.
			for (int j = 0; j < (int)preload[i].tokens.size(); j++)
				preload[i].guard &= tokens[preload[i].tokens[j]].guard;

			if (base->transitions[preload[i].index].behavior == transition::passive)
				preload[i].guard &= base->transitions[preload[i].index].local_action;

			// Check for unstable transitions
			bool previously_enabled = false;
			for (int j = 0; j < (int)loaded.size() && !previously_enabled; j++)
				if (loaded[j].index == preload[i].index)
				{
					preload[i].history = loaded[j].history;
					previously_enabled = true;
				}

			// Now we check to see if the current state passes the guard
			int ready = boolean::passes_guard(encoding, global, preload[i].guard, &preload[i].guard_action);
			if (ready < 0 && previously_enabled)
				ready = 0;

			preload[i].stable = (ready > 0);
			if (base->transitions[preload[i].index].behavior == transition::active)
			{
				preload[i].vacuous = boolean::vacuous_assign(global, base->transitions[preload[i].index].remote_action, preload[i].stable);
				preload[i].stable = preload[i].stable || preload[i].vacuous;
			}

			if (!preload[i].vacuous)
			{
				if (ready >= 0)
					potential.push_back(preload[i]);

				for (int j = 0; j < (int)tokens.size(); j++)
					if (tokens[j].cause > i)
						tokens[j].cause--;

				global_disabled.push_back(preload[i].index);
				sort(global_disabled.begin(), global_disabled.end());
				preload.erase(preload.begin() + i);
			}
			else
			{
				vector<int> output = base->next(transition::type, preload[i].index);
				bool loop = true;
				for (int j = 0; j < (int)output.size() && loop; j++)
				{
					loop = false;
					for (int k = 0; k < (int)tokens.size() && !loop; k++)
						if (tokens[k].index == output[j])
							loop = true;
				}

				if (!loop)
				{
					boolean::cover guard = preload[i].guard;
					if (base->transitions[preload[i].index].behavior == transition::active && base->transitions[preload[i].index].local_action != 1)
						guard = base->transitions[preload[i].index].local_action; //TODO assumption about prs guards

					for (int j = 0; j < (int)output.size(); j++)
					{
						preload[i].output_marking.push_back((int)tokens.size());
						tokens.push_back(token(output[j], guard, i));
					}
				}
				else
				{
					for (int j = 0; j < (int)tokens.size(); j++)
						if (tokens[j].cause > i)
							tokens[j].cause--;

					global_disabled.push_back(preload[i].index);
					sort(global_disabled.begin(), global_disabled.end());
					preload.erase(preload.begin() + i);
				}
			}
		}
	} while ((int)preload.size() != preload_size);

	for (int i = 0; i < (int)potential.size(); i++)
	{
		// Get the output marking of the potential
		boolean::cover guard = potential[i].guard;
		if (base->transitions[potential[i].index].behavior == transition::active && base->transitions[potential[i].index].local_action != 1)
			guard = base->transitions[potential[i].index].local_action; //TODO assumption about prs guards

		vector<int> output = base->next(transition::type, potential[i].index);
		for (int j = 0; j < (int)output.size(); j++)
		{
			potential[i].output_marking.push_back((int)tokens.size());
			tokens.push_back(token(output[j], guard, preload.size()));
		}

		preload.push_back(potential[i]);
	}

	loaded = preload;
	ready.clear();

	for (int i = 0; i < (int)loaded.size(); i++)
		if (!loaded[i].vacuous)
			for (int j = 0; j < (int)base->transitions[loaded[i].index].local_action.cubes.size(); j++)
				ready.push_back(pair<int, int>(i, j));

	if (last.index >= 0)
		for (int i = 0; i < (int)loaded.size(); i++)
			if (loaded[i].stable)
				loaded[i].history.push_back(last);

	/*for (int i = 0; i < (int)tokens.size(); i++)
		cout << "Token " << i << ": " << tokens[i].index << " " << tokens[i].cause << " " << tokens[i].guard << endl;

	cout << endl;

	for (int i = 0; i < (int)loaded.size(); i++)
	{
		cout << "Loaded " << i << ": " << loaded[i].index << " " << (loaded[i].vacuous ? "vacuous" : "") << " {";
		for (int j = 0; j < (int)loaded[i].tokens.size(); j++)
			cout << loaded[i].tokens[j] << " ";
		cout << "} {";
		for (int j = 0; j < (int)loaded[i].output_marking.size(); j++)
			cout << loaded[i].output_marking[j] << " ";
		cout << "}" << endl;
	}

	cout << endl;*/

	// Now in the production rule simulator, here is where I would automatically execute all of the
	// vacuous transitions, but that leads to some really strange (incorrect?) behavior in an HSE.

	// For example, we might take both paths of a conditional. Then I would have to add in a bunch of extra stuff
	// to handle conditional merges. Like if two tokens happen to be on the same place, then they are merged
	// into one. Luckily, since most conditional splits are guarded, this is unlikely to happen.

	// Unless... I would have to check the next assignment, skipping past any guards. since the assumption is the
	// execution of the assignment stores all of the information about any previous guards. (this is the weakest
	// assumption I could make about state encoding. If I made no assumption, about what is needed to encode
	// a state, then I would not be able to simulate instability). The thing is that most HSE will be resistant
	// to this because they don't use the previous assignment to entirely encode the next state.

	// So for now, I'm going to just mark them vacuous and let whoever is managing the simulator handle them.
	// That might be to execute all of the vacuous firings first in a random order (this would prevent taking
	// two branches of a conditional simultaneously). Or it could be to try and handle it correctly.

	// TODO I don't know how I would treat the case when the execution enters a state that is not part of the HSE.
	// In this case, all of the transitions in the entire HSE are vacuous (possibly unstable) and the tokens
	// could just zoom around the HSE making no changes to the state information.

	return ready.size();
}

enabled_transition simulator::fire(int index)
{
	if (base == NULL)
	{
		internal("", "NULL pointer to simulator::base", __FILE__, __LINE__);
		return enabled_transition();
	}

	enabled_transition t = loaded[ready[index].first];
	int term = ready[index].second;

	// We need to go through the potential list of transitions and flatten their input and output tokens.
	// Since we know that no transition can use the same token twice, we can simply mush them all
	// into one big list and then remove duplicates.
	// assumes that a transition in the loaded array only depends upon transitions before it in the array
	vector<int> visited(1, ready[index].first);
	for (int i = 0; i < (int)t.tokens.size(); i++)
		if (tokens[t.tokens[i]].cause >= 0 && tokens[t.tokens[i]].cause < ready[index].first)
		{
			t.tokens.insert(t.tokens.end(), loaded[tokens[t.tokens[i]].cause].tokens.begin(), loaded[tokens[t.tokens[i]].cause].tokens.end());
			t.output_marking.insert(t.output_marking.end(), loaded[tokens[t.tokens[i]].cause].output_marking.begin(), loaded[tokens[t.tokens[i]].cause].output_marking.end());
			visited.push_back(tokens[t.tokens[i]].cause);
		}

	sort(t.tokens.begin(), t.tokens.end());
	t.tokens.resize(unique(t.tokens.begin(), t.tokens.end()) - t.tokens.begin());
	sort(t.output_marking.begin(), t.output_marking.end());
	t.output_marking.resize(unique(t.output_marking.begin(), t.output_marking.end()) - t.output_marking.begin());
	sort(visited.begin(), visited.end());

	for (int i = 0; i < (int)loaded.size(); i++)
	{
		if (find(visited.begin(), visited.end(), i) == visited.end())
		{
			for (int j = 0; j < (int)loaded[i].tokens.size(); j++)
				if (tokens[loaded[i].tokens[j]].cause >= 0 && tokens[loaded[i].tokens[j]].cause < i && find(visited.begin(), visited.end(), tokens[loaded[i].tokens[j]].cause) == visited.end())
				{
					loaded[i].tokens.insert(loaded[i].tokens.end(), loaded[tokens[loaded[i].tokens[j]].cause].tokens.begin(), loaded[tokens[loaded[i].tokens[j]].cause].tokens.end());
					loaded[i].output_marking.insert(loaded[i].output_marking.end(), loaded[tokens[loaded[i].tokens[j]].cause].output_marking.begin(), loaded[tokens[loaded[i].tokens[j]].cause].output_marking.end());
				}

			sort(loaded[i].tokens.begin(), loaded[i].tokens.end());
			loaded[i].tokens.resize(unique(loaded[i].tokens.begin(), loaded[i].tokens.end()) - loaded[i].tokens.begin());
			sort(loaded[i].output_marking.begin(), loaded[i].output_marking.end());
			loaded[i].output_marking.resize(unique(loaded[i].output_marking.begin(), loaded[i].output_marking.end()) - loaded[i].output_marking.begin());
		}
	}

	// disable any transitions that were dependent on at least one of the same local tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)loaded.size()-1, j = ready.size()-1; i >= 0; i--)
	{
		vector<int> intersect = vector_intersection(loaded[i].tokens, t.tokens);
		if (intersect.size() > 0)
		{
			// assumes the ready array is sorted in ascending order
			bool is_ready = false;
			for (; j >= 0 && !is_ready; j--)
				if (ready[j].first == i)
					is_ready = true;

			bool determ = true;
			for (int j = 0; j < (int)intersect.size() && determ; j++)
				if (find(base->arbiters.begin(), base->arbiters.end(), tokens[intersect[j]].index) != base->arbiters.end())
					determ = false;

			if (is_ready && determ && loaded[i].index != t.index)
			{
				mutex err = mutex(t, loaded[i]);
				vector<mutex>::iterator loc = lower_bound(mutex_errors.begin(), mutex_errors.end(), err);
				if (loc == mutex_errors.end() || *loc != err)
				{
					mutex_errors.insert(loc, err);
					error("", err.to_string(*base, *variables), __FILE__, __LINE__);
				}
			}

			loaded.erase(loaded.begin() + i);
		}
	}

	ready.clear();

	// take the set symmetric difference, but leave the two sets separate.
	for (int j = 0, k = 0; j < (int)t.tokens.size() && k < (int)t.output_marking.size(); )
	{
		if (t.tokens[j] == t.output_marking[k])
		{
			t.tokens.erase(t.tokens.begin() + j);
			t.output_marking.erase(t.output_marking.begin() + k);
		}
		else if (t.tokens[j] > t.output_marking[k])
			k++;
		else if (t.tokens[j] < t.output_marking[k])
			j++;
	}

	// Check to see if this transition is unstable
	if (!t.stable)
	{
		instability err = instability(t);
		vector<instability>::iterator loc = lower_bound(instability_errors.begin(), instability_errors.end(), err);
		if (loc == instability_errors.end() || *loc != err)
		{
			instability_errors.insert(loc, err);
			error("", err.to_string(*base, *variables), __FILE__, __LINE__);
		}
	}

	// Update the tokens
	for (int i = 0; i < (int)t.output_marking.size(); i++)
		tokens[t.output_marking[i]].cause = -1;

	sort(t.tokens.begin(), t.tokens.end());
	for (int i = t.tokens.size()-1; i >= 0; i--)
		tokens.erase(tokens.begin() + t.tokens[i]);

	for (int i = tokens.size()-1; i >= 0; i--)
		if (tokens[i].cause >= 0)
			tokens.erase(tokens.begin() + i);

	// Check for interfering transitions. Interfering transitions are the active transitions that have fired since this
	// active transition was enabled.
	boolean::cube local_action = base->transitions[t.index].local_action[term];
	boolean::cube remote_action = base->transitions[t.index].remote_action[term];
	if (base->transitions[t.index].behavior == transition::active)
		for (int j = 0; j < (int)t.history.size(); j++)
			if (base->transitions[t.history[j].index].behavior == transition::active)
			{
				if (boolean::are_mutex(base->transitions[t.index].remote_action[term], base->transitions[t.history[j].index].local_action[t.history[j].term]))
				{
					interference err(term_index(t.index, term), t.history[j]);
					vector<interference>::iterator loc = lower_bound(interference_errors.begin(), interference_errors.end(), err);
					if (loc == interference_errors.end() || *loc != err)
					{
						interference_errors.insert(loc, err);
						error("", err.to_string(*base, *variables), __FILE__, __LINE__);
					}
				}

				local_action = boolean::interfere(local_action, base->transitions[t.history[j].index].remote_action[t.history[j].term]);
				remote_action = boolean::interfere(remote_action, base->transitions[t.history[j].index].remote_action[t.history[j].term]);
			}

	// Update the state
	if (t.stable)
	{
		global &= t.guard_action;
		encoding &= t.guard_action;
	}

	global = local_assign(global, remote_action, t.stable);
	encoding = remote_assign(local_assign(encoding, local_action, t.stable), global, true);

	last = term_index(t.index, term);
	return t;
}

void simulator::merge_errors(const simulator &sim)
{
	if (instability_errors.size() > 0)
	{
		vector<instability> old_instability_errors;
		swap(instability_errors, old_instability_errors);
		instability_errors.resize(old_instability_errors.size() + sim.instability_errors.size());
		merge(sim.instability_errors.begin(), sim.instability_errors.end(), old_instability_errors.begin(), old_instability_errors.end(), instability_errors.begin());
		instability_errors.resize(unique(instability_errors.begin(), instability_errors.end()) - instability_errors.begin());
	}
	else
		instability_errors = sim.instability_errors;

	if (interference_errors.size() > 0)
	{
		vector<interference> old_interference_errors;
		swap(interference_errors, old_interference_errors);
		interference_errors.resize(sim.interference_errors.size() + old_interference_errors.size());
		merge(sim.interference_errors.begin(), sim.interference_errors.end(), old_interference_errors.begin(), old_interference_errors.end(), interference_errors.begin());
		interference_errors.resize(unique(interference_errors.begin(), interference_errors.end()) - interference_errors.begin());
	}
	else
		interference_errors = sim.interference_errors;

	if (mutex_errors.size() > 0)
	{
		vector<mutex> old_mutex_errors;
		swap(mutex_errors, old_mutex_errors);
		mutex_errors.resize(sim.mutex_errors.size() + old_mutex_errors.size());
		merge(sim.mutex_errors.begin(), sim.mutex_errors.end(), old_mutex_errors.begin(), old_mutex_errors.end(), mutex_errors.begin());
		mutex_errors.resize(unique(mutex_errors.begin(), mutex_errors.end()) - mutex_errors.begin());
	}
	else
		mutex_errors = sim.mutex_errors;
}

state simulator::get_state()
{
	state result;
	result.encodings = encoding;
	for (int i = 0; i < (int)tokens.size(); i++)
		if (tokens[i].cause < 0)
			result.tokens.push_back(tokens[i]);
	sort(result.tokens.begin(), result.tokens.end());
	return result;
}

state simulator::get_key()
{
	state result;
	result.encodings = encoding;
	for (int i = 0; i < (int)ready.size(); i++)
		result.tokens.push_back(petri::token(loaded[ready[i].first].index));
	sort(result.tokens.begin(), result.tokens.end());
	result.tokens.resize(unique(result.tokens.begin(), result.tokens.end()) - result.tokens.begin());
	return result;
}

}
