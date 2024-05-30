/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"
#include "simulator.h"
#include <common/message.h>
#include <common/text.h>
#include <interpret_boolean/export.h>
#include <petri/simulator.h>
#include <petri/state.h>

namespace hse
{

place::place()
{
	arbiter = false;
}

place::place(boolean::cover predicate)
{
	this->predicate = predicate;
	this->effective = predicate;
	arbiter = false;
}

place::~place()
{

}

// Merge two places and combine the predicate and effective predicate.
// composition can be one of:
// 1. petri::parallel
// 2. petri::choice
// 3. petri::sequence
// See haystack/lib/petri/petri/graph.h for their definitions.
place place::merge(int composition, const place &p0, const place &p1)
{
	place result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.effective = p0.effective & p1.effective;
		result.predicate = p0.predicate & p1.predicate;
	}
	else if (composition == petri::choice)
	{
		result.effective = p0.effective | p1.effective;
		result.predicate = p0.predicate | p1.predicate;
	}
	result.arbiter = (p0.arbiter or p1.arbiter);
	return result;
}

transition::transition(boolean::cover guard, boolean::cover local_action, boolean::cover remote_action)
{
	this->guard = guard;
	this->local_action = local_action;
	this->remote_action = remote_action;
}

transition::~transition()
{

}

transition transition::subdivide(int term) const
{
	if (term < (int)local_action.cubes.size() && term < (int)remote_action.cubes.size())
		return transition(guard, boolean::cover(local_action.cubes[term]), boolean::cover(remote_action.cubes[term]));
	else if (term < (int)local_action.cubes.size())
		return transition(guard, boolean::cover(local_action.cubes[term]));
	else
		return transition(guard);
}

transition transition::merge(int composition, const transition &t0, const transition &t1)
{
	transition result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.guard = t0.guard & t1.guard;
		result.local_action = t0.local_action & t1.local_action;
		result.remote_action = t0.remote_action & t1.remote_action;
	}
	else if (composition == petri::choice)
	{
		result.guard = t0.guard | t1.guard;
		result.local_action = t0.local_action | t1.local_action;
		result.remote_action = t0.remote_action | t1.remote_action;
	}
	return result;
}

bool transition::mergeable(int composition, const transition &t0, const transition &t1)
{
	if (composition == petri::sequence) {
		return t0.local_action.is_tautology();
	}

	// choice or parallel
	return (t0.guard == t1.guard) ||
				 (t0.local_action == t1.local_action);
}

// Is this transition actually ever able to fire? If not, then it is
// infeasible. This function is conservative. Like the is_vacuous() function,
// this only answers either TRUE or MAYBE.
bool transition::is_infeasible() const
{
	return guard.is_null() || local_action.is_null();
}

// Vacuous transitions are transitions that do not have any effect on the state
// of the circuit or on knowledge about the state of the circuit. This function
// does not actually answer that question correctly, because you need to know
// the current state of the circuit to properly answer that question. Instead,
// this function is conservative. We know that this transition is definitely
// vacuous if both the guard and action are 1 because, by definition, they will
// not have any effect on the state or our knowledge of it. So this function
// really returns either TRUE or MAYBE.
bool transition::is_vacuous() const
{
	return guard.is_tautology() && local_action.is_tautology();
}

graph::graph()
{
}

graph::~graph()
{

}

boolean::cover graph::predicate(petri::iterator i, vector<petri::iterator> *prev) const
{
	if (i.type == petri::place::type) {
		return places[i.index].predicate;
	}
	
	boolean::cover pred = 1;
	for (auto arc = arcs[1-i.type].begin(); arc != arcs[1-i.type].end(); arc++) {
		if (arc->to.index == i.index) {
			pred &= places[arc->from.index].predicate;
			if (prev != nullptr) {
				prev->push_back(arc->from);
			}
		}
	}
	return pred;
}

boolean::cover graph::effective(petri::iterator i, vector<petri::iterator> *prev) const
{
	if (i.type == petri::place::type) {
		return places[i.index].effective;
	}
	
	boolean::cover pred = 1;
	for (auto arc = arcs[1-i.type].begin(); arc != arcs[1-i.type].end(); arc++) {
		if (arc->to.index == i.index) {
			pred &= places[arc->from.index].predicate;
			if (prev != nullptr) {
				prev->push_back(arc->from);
			}
		}
	}
	pred &= ~transitions[i.index].guard & ~transitions[i.index].local_action;
	return pred;
}

boolean::cover graph::implicant(petri::iterator i, vector<petri::iterator> *prev) const
{
	if (i.type == petri::place::type) {
		return places[i.index].predicate;
	}
	
	boolean::cover pred = 1;
	for (auto arc = arcs[1-i.type].begin(); arc != arcs[1-i.type].end(); arc++) {
		if (arc->to.index == i.index) {
			pred &= places[arc->from.index].predicate;
			if (prev != nullptr) {
				prev->push_back(arc->from);
			}
		}
	}
	pred &= transitions[i.index].guard;
	return pred;
}

boolean::cover graph::effective_implicant(petri::iterator i, vector<petri::iterator> *prev) const
{
	if (i.type == petri::place::type) {
		return places[i.index].effective;
	}
	
	boolean::cover pred = 1;
	for (auto arc = arcs[1-i.type].begin(); arc != arcs[1-i.type].end(); arc++) {
		if (arc->to.index == i.index) {
			pred &= places[arc->from.index].predicate;
			if (prev != nullptr) {
				prev->push_back(arc->from);
			}
		}
	}
	pred &= transitions[i.index].guard & ~transitions[i.index].local_action;
	return pred;
}

// assumes all vector<int> inputs are sorted
bool graph::common_arbiter(petri::iterator a, petri::iterator b) const
{
	vector<petri::iterator> left, right;
	if (a.type == petri::place::type) {
		if (places[a.index].arbiter) {
			left.push_back(a);
		} else {
			return false;
		}
	}
 
	if (b.type == petri::place::type) {
		if (places[b.index].arbiter) {
			right.push_back(b);
		} else {
			return false;
		}
	}

	if (a.type == petri::transition::type) {
		vector<petri::iterator> p = prev(a);
		for (int i = 0; i < (int)p.size(); i++) {
			if (places[p[i].index].arbiter) {
				left.push_back(p[i]);
			}
		}
	}
	if (left.size() == 0) {
		return false;
	}

	if (b.type == petri::transition::type) {
		vector<petri::iterator> p = prev(b);
		for (int i = 0; i < (int)p.size(); i++) {
			if (places[p[i].index].arbiter) {
				right.push_back(p[i]);
			}
		}
	}
	if (right.size() == 0) {
		return false;
	}

	return vector_intersection_size(left, right) > 0;
}

void graph::post_process(const ucs::variable_set &variables, bool proper_nesting, bool aggressive)
{
	for (int i = 0; i < (int)transitions.size(); i++)
		transitions[i].remote_action = transitions[i].local_action.remote(variables.get_groups());

	// Handle Reset Behavior
	bool change = true;
	while (change)
	{
		super::reduce(proper_nesting, aggressive);

		for (int i = 0; i < (int)source.size(); i++)
		{
			simulator::super sim(this, source[i]);
			sim.enabled();

			change = false;
			for (int j = 0; j < (int)sim.ready.size() && !change; j++)
			{
				bool firable = transitions[sim.ready[j].index].local_action.cubes.size() <= 1;
				for (int k = 0; k < (int)sim.ready[j].tokens.size() && firable; k++)
				{
					for (int l = 0; l < (int)arcs[petri::transition::type].size() && firable; l++)
						if (arcs[petri::transition::type][l].to.index == sim.tokens[sim.ready[j].tokens[k]].index)
							firable = false;
					for (int l = 0; l < (int)arcs[petri::place::type].size() && firable; l++)
						if (arcs[petri::place::type][l].from.index == sim.tokens[sim.ready[j].tokens[k]].index && arcs[petri::place::type][l].to.index != sim.ready[j].index)
							firable = false;
				}

				if (firable) {
					petri::enabled_transition t = sim.fire(j);
					source[i].tokens = sim.tokens;
					for (int k = (int)transitions[t.index].local_action.size()-1; k >= 0; k--) {
						for (int l = (int)transitions[t.index].guard.size()-1; l >= 0; l--) {
							int idx = i;
							if (k > 0 || l > 0) {
								idx = source.size();
								source.push_back(source[i]);
							}

							source[idx].encodings &= transitions[t.index].guard.cubes[l];
							source[idx].encodings = local_assign(source[idx].encodings, transitions[t.index].local_action.cubes[k], true);
							source[idx].encodings = remote_assign(source[idx].encodings, transitions[t.index].remote_action.cubes[k], true);
						}
					}

					change = true;
				}
			}
		}

		for (int i = 0; i < (int)reset.size(); i++)
		{
			simulator::super sim(this, reset[i]);
			sim.enabled();

			change = false;
			for (int j = 0; j < (int)sim.ready.size() && !change; j++)
			{
				bool firable = transitions[sim.ready[j].index].local_action.cubes.size() <= 1;
				for (int k = 0; k < (int)sim.ready[j].tokens.size() && firable; k++)
				{
					for (int l = 0; l < (int)arcs[petri::transition::type].size() && firable; l++)
						if (arcs[petri::transition::type][l].to.index == sim.tokens[sim.ready[j].tokens[k]].index)
							firable = false;
					for (int l = 0; l < (int)arcs[petri::place::type].size() && firable; l++)
						if (arcs[petri::place::type][l].from.index == sim.tokens[sim.ready[j].tokens[k]].index && arcs[petri::place::type][l].to.index != sim.ready[j].index)
							firable = false;
				}

				if (firable) {
					petri::enabled_transition t = sim.fire(j);
					reset[i].tokens = sim.tokens;
					for (int k = (int)transitions[t.index].local_action.size()-1; k >= 0; k--) {
						for (int l = (int)transitions[t.index].guard.size()-1; l >= 0; l--) {
							int idx = i;
							if (k > 0 || l > 0) {
								idx = reset.size();
								reset.push_back(reset[i]);
							}

							reset[idx].encodings &= transitions[t.index].guard.cubes[l];
							reset[idx].encodings = local_assign(reset[idx].encodings, transitions[t.index].local_action.cubes[k], true);
							reset[idx].encodings = remote_assign(reset[idx].encodings, transitions[t.index].remote_action.cubes[k], true);
						}
					}

					change = true;
				}
			}
		}
	}

	change = true;
	while (change) {
		super::reduce(proper_nesting, aggressive);

		change = false;
		for (petri::iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++) {
			if (transitions[i.index].local_action == 1) {
				vector<petri::iterator> n = next(i); // places
				if (n.size() > 1) {
					vector<petri::iterator> p = prev(i); // places
					vector<vector<petri::iterator> > pp;
					for (int j = 0; j < (int)p.size(); j++) {
						pp.push_back(prev(p[j]));
					}

					for (int k = (int)arcs[petri::transition::type].size()-1; k >= 0; k--) {
						if (arcs[petri::transition::type][k].from == i) {
							disconnect(petri::iterator(petri::transition::type, k));
						}
					}

					vector<petri::iterator> copies;
					copies.push_back(i);
					for (int k = 0; k < (int)n.size(); k++) {
						if (k > 0) {
							copies.push_back(copy(i));
							for (int l = 0; l < (int)p.size(); l++) {
								petri::iterator x = copy(p[l]);
								connect(pp[l], x);
								connect(x, copies.back());
							}
						}
						connect(copies.back(), n[k]);
					}
					change = true;
				}
			}
		}
		if (change)
			continue;

		for (petri::iterator i(place::type, 0); i < (int)places.size() && !change; i++) {
			vector<petri::iterator> p = prev(i);
			vector<petri::iterator> active, passive;
			for (int k = 0; k < (int)p.size(); k++) {
				if (transitions[p[k].index].local_action == 1) {
					passive.push_back(p[k]);
				} else {
					active.push_back(p[k]);
				}
			}

			if (passive.size() > 1 || (passive.size() == 1 && active.size() > 0)) {
				vector<petri::iterator> copies;
				if ((int)active.size() == 0) {
					copies.push_back(i);
				}

				vector<petri::iterator> n = next(i);
				vector<vector<petri::iterator> > nn;
				for (int l = 0; l < (int)n.size(); l++) {
					nn.push_back(next(n[l]));
				}
				vector<vector<petri::iterator> > np;
				for (int l = 0; l < (int)n.size(); l++) {
					np.push_back(prev(n[l]));
					np.back().erase(std::remove(np.back().begin(), np.back().end(), i), np.back().end()); 
				}

				for (int k = 0; k < (int)passive.size(); k++) {
					// Disconnect this transition
					for (int l = (int)arcs[petri::transition::type].size()-1; l >= 0; l--) {
						if (arcs[petri::transition::type][l].from == passive[k]) {
							disconnect(petri::iterator(petri::transition::type, l));
						}
					}

					if (k >= (int)copies.size()) {
						copies.push_back(copy(i));
						for (int l = 0; l < (int)n.size(); l++) {
							petri::iterator x = copy(n[l]);
							connect(copies.back(), x);
							connect(x, nn[l]);
							connect(np[l], x);
						}
					}

					connect(passive[k], copies[k]);
				}

				change = true;
			}
		}
		if (change)
			continue;

		for (petri::iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++) {
			if (transitions[i.index].local_action == 1) {
				vector<petri::iterator> nn = next(next(i)); // transitions
				for (int l = 0; l < (int)nn.size(); l++) {
					transitions[nn[l].index] = transition::merge(petri::sequence, transitions[i.index], transitions[nn[l].index]);
				}

				pinch(i);
				change = true;
			}
		}
		if (change)
			continue;
	}

	// Determine the actual starting location of the tokens given the state information
	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++)
	{
		for (petri::iterator j = begin(petri::transition::type); j < end(petri::transition::type); j++) {
			if (is_reachable(j, i)) {
				places[i.index].mask = places[i.index].mask.combine_mask(transitions[j.index].guard.mask()).combine_mask(transitions[j.index].local_action.mask());
			}
		}
		places[i.index].mask = places[i.index].mask.flip();
	}

	if (reset.size() == 0)
		reset = source;
}

void graph::check_variables(const ucs::variable_set &variables)
{
	vector<int> vars;
	for (int i = 0; i < (int)variables.nodes.size(); i++) {
		vector<int> written;
		vector<int> read;

		for (int j = 0; j < (int)transitions.size(); j++) {
			vars = transitions[j].remote_action.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				written.push_back(j);
			}

			vars = transitions[j].guard.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				read.push_back(j);
			}
		}

		if (written.size() == 0 && read.size() > 0)
			warning("", variables.nodes[i].to_string() + " never assigned", __FILE__, __LINE__);
		else if (written.size() == 0 && read.size() == 0)
			warning("", "unused variable " + variables.nodes[i].to_string(), __FILE__, __LINE__);
	}
}

vector<int> graph::first_assigns()
{
	vector<int> result;
	typedef petri::state<petri::token> petri_state;
	typedef petri::graph<place, transition, petri::token, petri_state> petri_graph;
	typedef petri::simulator<place, transition, petri::token, petri_state> petri_simulator;

	vector<petri_simulator> simulators;
	for (int i = 0; i < (int)source.size(); i++)
		simulators.push_back(petri_simulator((const petri_graph*)this, petri_state(source[i].tokens)));

	while (simulators.size() > 0)
	{
		petri_simulator sim = simulators.back();
		simulators.pop_back();
		if (sim.enabled() > 0) {
			for (int i = 0; i < (int)sim.ready.size(); i++) {
				if (transitions[sim.ready[i].index].local_action.is_tautology()) {
					simulators.push_back(sim);
					simulators.back().fire(i);
				} else {
					result.push_back(sim.ready[i].index);
				}
			}
		}
	}

	sort(result.begin(), result.end());
	result.resize(unique(result.begin(), result.end()) - result.begin());

	return result;
}

vector<int> graph::associated_assigns(vector<int> tokens)
{
	vector<int> result;
	vector<int> visited;

	while (tokens.size() == 0)
	{
		vector<int>::iterator loc = lower_bound(visited.begin(), visited.end(), tokens.back());
		if (loc == visited.end() || *loc != tokens.back())
		{
			vector<int> n = next(place::type, tokens.back());
			visited.insert(loc, tokens.back());
			tokens.pop_back();

			for (int i = 0; i < (int)n.size(); i++)
			{
				if (!transitions[n[i]].local_action.is_tautology())
					result.push_back(n[i]);
				else
				{
					vector<int> nn = next(transition::type, n[i]);
					tokens.insert(tokens.end(), nn.begin(), nn.end());
				}
			}
			sort(tokens.begin(), tokens.end());
			tokens.resize(unique(tokens.begin(), tokens.end()) - tokens.begin());
		}
		else
			tokens.pop_back();
	}

	sort(result.begin(), result.end());
	result.resize(unique(result.begin(), result.end()) - result.begin());

	return result;
}

vector<petri::iterator> graph::relevant_nodes(vector<petri::iterator> curr)
{
	vector<petri::iterator> result;
	// We're going to check this transition against all of the places in the system
	for (petri::iterator j = begin(petri::place::type); j != end(petri::place::type); j++) {
		// The place is in the same process as the implicant set of states,
		// and its not in parallel with the transition we're checking,
		// and they aren't forced to be mutually exclusive by an arbiter
	
		bool relevant = false;
		for (int i = 0; i < (int)curr.size() && !relevant; i++) {
			relevant = (is_reachable(curr[i], j) || is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() && relevant; i++) {
			relevant = (j != curr[i] &&
					!is_parallel(j, curr[i]) &&
					!common_arbiter(curr[i], j));
		}

		if (relevant) {
			result.push_back(j);
		}
	}

	// check the states inside each transition
	for (petri::iterator j = begin(petri::transition::type); j != end(petri::transition::type); j++) {
		// The place is in the same process as the implicant set of states,
		// and its not in parallel with the transition we're checking,
		// and they aren't forced to be mutually exclusive by an arbiter
		bool relevant = false;
		for (int i = 0; i < (int)curr.size() && !relevant; i++) {
			relevant = (is_reachable(curr[i], j) || is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() && relevant; i++) {
			relevant = (j != curr[i] &&
					!is_parallel(j, curr[i]) &&
					!common_arbiter(curr[i], j));
		}

		if (relevant) {
			result.push_back(j);
		}
	}

	return result;
}

}
