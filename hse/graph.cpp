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
#include <common/math.h>
#include <interpret_boolean/export.h>
#include <petri/simulator.h>
#include <petri/state.h>

namespace hse
{

place::place()
{
	arbiter = false;
	synchronizer = false;
}

place::place(boolean::cover predicate)
{
	this->predicate = predicate;
	this->effective = predicate;
	arbiter = false;
	synchronizer = false;
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
	result.synchronizer = (p0.synchronizer or p1.synchronizer);
	return result;
}

transition::transition(boolean::cover assume, boolean::cover guard, boolean::cover local_action, boolean::cover remote_action)
{
	this->assume = assume;
	this->guard = guard;
	this->local_action = local_action;
	this->remote_action = remote_action;
	this->ghost = 1;
}

transition::~transition()
{

}

transition transition::subdivide(int term) const
{
	if (term < (int)local_action.cubes.size() && term < (int)remote_action.cubes.size())
		return transition(assume, guard, boolean::cover(local_action.cubes[term]), boolean::cover(remote_action.cubes[term]));
	else if (term < (int)local_action.cubes.size())
		return transition(assume, guard, boolean::cover(local_action.cubes[term]));
	else
		return transition(assume, guard);
}

transition transition::merge(int composition, const transition &t0, const transition &t1)
{
	transition result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.assume = t0.assume & t1.assume;
		result.guard = t0.guard & t1.guard;
		result.local_action = t0.local_action & t1.local_action;
		result.remote_action = t0.remote_action & t1.remote_action;
	}
	else if (composition == petri::choice)
	{
		result.assume = t0.assume | t1.assume;
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
	return (t0.guard == t1.guard and t0.assume == t1.assume) or
				 (t0.local_action == t1.local_action);
}

// Is this transition actually ever able to fire? If not, then it is
// infeasible. This function is conservative. Like the is_vacuous() function,
// this only answers either TRUE or MAYBE.
bool transition::is_infeasible() const
{
	return guard.is_null() or local_action.is_null();
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
	return assume.is_tautology() and guard.is_tautology() and local_action.is_tautology();
}

graph::graph()
{
}

graph::~graph()
{

}

hse::transition &graph::at(term_index idx) {
	return transitions[idx.index];
}

boolean::cube &graph::term(term_index idx) {
	return transitions[idx.index].local_action[idx.term];
}

boolean::cover graph::predicate(vector<petri::iterator> pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
		}
	}
	result.hide(ghost_nets);
	
	return result;
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
	pred.hide(ghost_nets);
	return pred;
}

boolean::cover graph::implicant(vector<petri::iterator> pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
			result &= transitions[i->index].guard;
		}
	}
	result.hide(ghost_nets);
	
	return result;
}

boolean::cover graph::effective_implicant(vector<petri::iterator> pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
			for (auto arc = arcs[i->type].begin(); arc != arcs[i->type].end(); arc++) {
				if (arc->from.index == i->index) {
					result &= ~transitions[arc->to.index].guard;
				}
			}
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
			result &= transitions[i->index].guard & ~transitions[i->index].local_action;
		}
	}
	result.hide(ghost_nets);
	return result;
}

boolean::cover graph::filter_vacuous(petri::iterator i, boolean::cover encoding, boolean::cube action) const
{
	// I need to select encodings that are either affected by a transition that
	// wouldn't otherwise happen, or that enable an interfering transition.
	// action is not vacuous and action is not covered by equal actions in the output transition
	// or action conflicts with opposing actions in the output transition
	if (i.type == petri::transition::type) {
		// No guards to contend with for transitions
		boolean::cube other = transitions[i.index].local_action.subcube();
		if (interfere(action, other).is_null()) {
			// First, check if action conflicts with opposing actions in the output transition
			// if so, then return all encodings
			return encoding;
		}

		if (other.is_subset_of(action)) {
			// Then, check if action is covered by equal actions in the output transition
			// if so, then return no encodings
			return boolean::cover(0);
		}
		
		// Then, filter out all encodings for which action is vacuous by anding against not_action
		return encoding & ~action;
	}

	boolean::cover result(0);
	// This is a place, so we need to check all of the output transitions
	boolean::cover wait = encoding & ~action;
	for (auto arc = arcs[i.type].begin(); arc != arcs[i.type].end(); arc++) {
		if (arc->from.index == i.index) {
			boolean::cover predicate = encoding & transitions[arc->to.index].guard;
			result |= filter_vacuous(arc->to, predicate, action);
			wait &= ~transitions[arc->to.index].guard;
		}
	}
	result |= wait;
	return result;
}

boolean::cover graph::exclusion(int index) const {
	boolean::cover result;
	vector<int> p = prev(transition::type, index);
	
	for (int i = 0; i < (int)p.size(); i++) {
		vector<int> n = next(place::type, p[i]);
		if (n.size() > 1) {
			for (int j = 0; j < (int)n.size(); j++) {
				if (n[j] != index) {
					result |= transitions[n[j]].guard;
				}
			}
		}
	}
	return result;
}

boolean::cover graph::arbitration(int index) const {
	boolean::cover result;
	vector<int> p = prev(transition::type, index);
	
	for (int i = 0; i < (int)p.size(); i++) {
		if (places[p[i]].arbiter or places[p[i]].synchronizer) {
			vector<int> n = next(place::type, p[i]);
			if (n.size() > 1) {
				for (int j = 0; j < (int)n.size(); j++) {
					if (n[j] != index) {
						result |= transitions[n[j]].local_action;
					}
				}
			}
		}
	}
	return ~result;
}


// assumes all vector<int> inputs are sorted
bool graph::common_arbiter(petri::iterator a, petri::iterator b) const
{
	vector<petri::iterator> left, right;
	if (a.type == petri::place::type) {
		/*if (places[a.index].arbiter) {
			left.push_back(a);
		} else {*/
			return false;
		//}
	}
 
	if (b.type == petri::place::type) {
		/*if (places[b.index].arbiter) {
			right.push_back(b);
		} else {*/
			return false;
		//}
	}

	if (a.type == petri::transition::type) {
		vector<petri::iterator> p = prev(a);
		for (int i = 0; i < (int)p.size(); i++) {
			if (places[p[i].index].arbiter or places[p[i].index].synchronizer) {
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
			if (places[p[i].index].arbiter or places[p[i].index].synchronizer) {
				right.push_back(p[i]);
			}
		}
	}
	if (right.size() == 0) {
		return false;
	}

	return vector_intersection_size(left, right) > 0;
}

void graph::update_masks() {
	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++) {
		places[i.index].mask = 1;
	}

	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++)
	{
		for (petri::iterator j = begin(petri::transition::type); j < end(petri::transition::type); j++) {
			if (is_reachable(j, i)) {
				places[i.index].mask = places[i.index].mask.combine_mask(transitions[j.index].assume.mask()).combine_mask(transitions[j.index].guard.mask()).combine_mask(transitions[j.index].local_action.mask()).combine_mask(transitions[j.index].ghost.mask());
			}
		}
		places[i.index].mask = places[i.index].mask.flip();
	}
}

void graph::post_process(ucs::variable_set &variables, bool proper_nesting, bool aggressive)
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

		// Remove skips
		change = false;
		for (petri::iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++) {
			if (transitions[i.index].local_action.is_tautology()) {
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

		// If there is a guard at the end of a conditional branch, then we unzip
		// the conditional merge by one transition (make copies of the next
		// transition on each branch and move the merge down the sequence). This
		// allows us to merge that guard at the end of the conditional branch into
		// the transition.
		for (petri::iterator i(place::type, 0); i < (int)places.size() && !change; i++) {
			vector<petri::iterator> p = prev(i);
			vector<petri::iterator> active, passive;
			for (int k = 0; k < (int)p.size(); k++) {
				if (transitions[p[k].index].local_action.is_tautology()) {
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
						copies.push_back(create(places[i.index]));
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

	annotate_conditional_branches(variables);

	// Determine the actual starting location of the tokens given the state information
	update_masks();
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

			vars = transitions[j].assume.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				read.push_back(j);
			}
		}

		if (written.size() == 0 && read.size() > 0)
			warning("", variables.nodes[i].to_string() + " never assigned", __FILE__, __LINE__);
		else if (written.size() == 0 && read.size() == 0 and variables.nodes[i].to_string().rfind(ghost_prefix, 0) == string::npos)
			warning("", "unused variable " + variables.nodes[i].to_string(), __FILE__, __LINE__);
	}
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
		for (int i = 0; i < (int)curr.size() and not relevant; i++) {
			relevant = (is_reachable(curr[i], j) or is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			relevant = (j != curr[i] and not common_arbiter(curr[i], j));
		}

		relevant = relevant and not is(parallel, vector<petri::iterator>(1, j), curr);

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			for (int k = 0; k < (int)arcs[transition::type].size() and curr[i].type == transition::type and relevant; k++) {
				relevant = (arcs[transition::type][k].from.index != curr[i].index or arcs[transition::type][k].to.index != j.index);
			}
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
		for (int i = 0; i < (int)curr.size() and not relevant; i++) {
			relevant = (is_reachable(curr[i], j) or is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			relevant = (j != curr[i] and not common_arbiter(curr[i], j));
		}
		
		relevant = relevant and not is(parallel, vector<petri::iterator>(1, j), curr);

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			for (int k = 0; k < (int)arcs[place::type].size() and curr[i].type == place::type and relevant; k++) {
				relevant = (arcs[place::type][k].from.index != curr[i].index or arcs[place::type][k].to.index != j.index);
			}
		}

		if (relevant) {
			result.push_back(j);
		}
	}

	return result;
}

void graph::annotate_conditional_branches(ucs::variable_set &variables) {
	if (not ghost_nets.empty()) {
		for (petri::iterator i = begin(petri::transition::type); i != end(petri::transition::type); i++) {
			transitions[i.index].ghost = 1;
		}
		ghost_nets.clear();
	}

	for (petri::iterator i = begin(petri::place::type); i != end(petri::place::type); i++) {
		places[i.index].ghost_nets.clear();

		vector<petri::iterator> n = next(i);
		if ((int)n.size() > 1) {
			int count = log2i((int)n.size());
			count += ((int)n.size() > (1<<count));
			for (int j = 0; j < count; j++) {
				string name = ghost_prefix + to_string(i.index) + "_" + to_string(places[i.index].ghost_nets.size());
				int uid = variables.define(name);
				if (uid < 0) {
					// If this branch was previously annotated, reuse the name
					uid = variables.find(name);
				}
				ghost_nets.push_back(uid);
				places[i.index].ghost_nets.push_back(uid);
			}

			boolean::cover total;
			for (int j = 0; j < (int)n.size(); j++) {
				if (j == (int)n.size()-1) {
					transitions[n[j].index].ghost &= ~total;
				} else {
					boolean::cover ghost = boolean::encode_binary(j, places[i.index].ghost_nets);
					transitions[n[j].index].ghost &= ghost;
					total |= ghost;
				}
			}
		}
	}
}

vector<petri::iterator> graph::normalize_cut(vector<petri::iterator> cut, int type, int forward) {
	// Step 1: back out all places to their associated transitions
	vector<petri::iterator> p;
	for (int i = cut.size()-1; i >= 0; i--) {
		if (cut[i].type != type) {
			p.push_back(cut[i]);
			cut.erase(cut.begin()+i);
		}
	}
	p = forward == 0 ? prev(p) : next(p);
	cut.insert(cut.end(), p.begin(), p.end());
	sort(cut.begin(), cut.end());
	cut.erase(unique(cut.begin(), cut.end()), cut.end());
	return cut;	
}

// insert a collection of tranistions connecting "from" to "to"
// enforce parallel composition, then remove redundant places/transitions/arcs
// 
void graph::insert_assign(vector<petri::iterator> from, vector<petri::iterator> to, boolean::cube assign) {
	if (assign.is_tautology()) {
		// This should never happen
		cout << "error: both sides of transition group are empty" << endl;
		return;
	}

	// Assumptions:
	//   1. "from" and "to" must be in parallel or sequence, they cannot be only
	//   composed conditionally.

	// if (not (is(parallel, from, to) or not is(choice, from, to))) {
	// 	return;
	// }

	//   2. Nodes in "from" must be composed in parallel to eachother and nodes
	//   in "to" must be composed in parallel to eachother.
	

	// TODO(edward.bingham) Insert assignments in parallel composition to nodes between from -> to
	// On the inputs:
	// 1. conditional cliques of parallel nodes [DONE]
	// 2. conditional support of degenerative cliques
	// 3. replication of conditional structures
	// On the outputs:
	// 1. conditional support of mismatched branches
	// 2. replication of conditional structures
	// For inserted transitions
	// 1. add reset marking for from -> to that reverses index priority
	// 2. parallel / sequential structure to respect cmos implementability
	// How do I handle:
	// 1. guards?
	// 2. ghosts?
	// 3. State update for inserted places?

	// Step 1: back out all places to their associated transitions
	// Step 2: break that cut into conditional groups of parallel transitions
	// Step 3: complete the dangling parallel groups using the conditional splits that differentiate the two groups
	from = normalize_cut(transition::type, 0, from);
	// separate nodes into conditional groups (sometimes conditional)
	vector<vector<petri::iterator> > p = select(parallel, from, false, true);
	// group subgroups that are sometimes in parallel while keeping subgroups in
	// place for the sometimes conditional
	p = group(parallel, p, false, false);
	// deconflict subgroups from overlapping groups
	p = complete(parallel, p);	









	// Step 4: add places after each transition and map to groups
	map<petri::iterator, petri::iterator> places;
	int up = 0;
	int dn = 0; 
	for (auto g = p.begin(); g != p.end(); g++) {
		for (auto n = g->begin(); n != g->end(); n++) {
			// If an input transition is downgoing, then the next one should be
			// upgoing and visa versa
			// Down -> Up; Down -> Up
			dn += transitions[n->index].local_action.count(1);
			up += transitions[n->index].local_action.count(0);

			auto pos = places.insert({*n, petri::iterator()});
			if (pos.second) {
				pos.first->second = connect(*n, create(place::type));
			}
			*n = pos.first->second;
		}
	}
	places.clear();

	// Grab the guards from the outgoing transitions
	boolean::cover guard = 1;
	for (auto i = to.begin(); i != to.end(); i++) {
		if (i->type == transition::type) {
			guard &= transitions[i->index].guard;
			transitions[i->index].guard = 1;
		}
	}
	int guardsense = (guard.count(1) >= guard.count(0));

	// TODO(edward.bingham) how possible is it to factor this guard G into two
	// expressions P and N such that G=P&N and the number of negative literals in
	// P and the number of positive literals in N are minimized?

	to = normalize_cut(transition::type, 1, to);
	for (auto n = to.begin(); n != to.end(); n++) {
		// Down -> Up; Down -> Up
		up += transitions[n->index].local_action.count(1);
		dn += transitions[n->index].local_action.count(0);

		auto pos = places.insert({*n, petri::iterator()});
		if (pos.second) {
			pos.first->second = connect(create(place::type), *n);
		}
		*n = pos.first->second;
	}
	int sense = (up >= dn);

	// This will group state variable transitions by their insertion location.
	// When multiple state variables are inserted at the same location, we have
	// to be careful about the composition of those transitions. Given a
	// downgoing output transition a&~b->x-, we need to insert state variables
	// like so: [a]; (v0-,v1-); [~b]; (v2+,v3+); x-. For upgoing a&~b->x+, its
	// inverted: [~b]; (v2+,v3+); [a]; (v0-,v1-); x+. If the guard has multiple
	// terms with different senses, then this becomes problematic. For example,
	// a&~b|~a&b->x-.

	// Step 5: insert single transitions into each of the locations
	// Step 6: break inserted transition apart based on cmos implementability
	for (int i = 0; i < (int)p.size(); i++) {
		bool guardHandled = false;
		if (assign.has(sense)) {
			vector<int> v = assign.mask(sense).vars();
			p[i] = connect(p[i], create(transition(), (int)v.size()));
			for (int j = 0; j < (int)p[i].size(); j++) {
				if (guardsense == 1-sense) {
					transitions[p[i][j].index].guard = guard;
				}
				transitions[p[i][j].index].local_action = boolean::cube(v[j], sense);
				transitions[p[i][j].index].remote_action = transitions[p[i][j].index].local_action;
			}
			guardHandled = (guardsense == 1-sense);
		}

		if (assign.has(1-sense)) {
			vector<int> v = assign.mask(1-sense).vars();
			p[i] = connect(p[i], create(transition(), (int)v.size()));
			for (int j = 0; j < (int)p[i].size(); j++) {
				if (not guardHandled) {
					transitions[p[i][j].index].guard = guard;
				}
				transitions[p[i][j].index].local_action = boolean::cube(v[j], 1-sense);
				transitions[p[i][j].index].remote_action = transitions[p[i][j].index].local_action;
			}
		}

		connect(p[i], to);
	}

	update_masks();
}

}
