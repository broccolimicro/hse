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

namespace hse
{

half_synchronization::half_synchronization()
{
	active.index = 0;
	active.cube = 0;
	passive.index = 0;
	passive.cube = 0;
}

half_synchronization::~half_synchronization()
{

}

synchronization_region::synchronization_region()
{

}

synchronization_region::~synchronization_region()
{

}

place::place()
{

}

place::place(boolean::cover predicate)
{
	this->predicate = predicate;
	this->effective = predicate;
}

place::~place()
{

}

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
	return result;
}

transition::transition()
{
	behavior = active;
	local_action = 1;
	remote_action = 1;
}

transition::transition(int behavior, boolean::cover action)
{
	this->behavior = behavior;
	this->local_action = action;
}

transition::~transition()
{

}

transition transition::subdivide(int term)
{
	return transition(behavior, boolean::cover(local_action.cubes[term]));
}

transition transition::merge(int composition, const transition &t0, const transition &t1)
{
	transition result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.local_action = t0.local_action & t1.local_action;
		result.remote_action = t0.remote_action & t1.remote_action;
		result.behavior = t0.behavior;
	}
	else if (composition == petri::choice)
	{
		result.local_action = t0.local_action | t1.local_action;
		result.remote_action = t0.remote_action | t1.remote_action;
		result.behavior = t0.behavior;
	}
	return result;
}

bool transition::mergeable(int composition, const transition &t0, const transition &t1)
{
	return t0.behavior == t1.behavior;
}

bool transition::is_infeasible()
{
	return local_action.is_null();
}

bool transition::is_vacuous()
{
	return local_action.is_tautology();
}

graph::graph()
{
}

graph::~graph()
{

}

void graph::post_process(const ucs::variable_set &variables, bool proper_nesting)
{
	for (int i = 0; i < (int)transitions.size(); i++)
		transitions[i].remote_action = transitions[i].local_action.remote(variables.get_groups());

	// Handle Reset Behavior
	bool change = true;
	while (change)
	{
		super::reduce(proper_nesting);

		change = false;
		for (int i = 0; i < (int)source.size(); i++)
		{
			vector<petri::enabled_transition<term_index> > enabled = this->enabled<reset_token, petri::enabled_transition<term_index> >(source[i].tokens, false);

			change = false;
			for (int j = 0; j < (int)enabled.size() && !change; j++)
			{
				bool firable = transitions[enabled[j].index].local_action.cubes.size() <= 1;
				for (int k = 0; k < (int)enabled[j].tokens.size() && firable; k++)
				{
					for (int l = 0; l < (int)arcs[petri::transition::type].size() && firable; l++)
						if (arcs[petri::transition::type][l].to.index == source[i].tokens[enabled[j].tokens[k]].index)
							firable = false;
					for (int l = 0; l < (int)arcs[petri::place::type].size() && firable; l++)
						if (arcs[petri::place::type][l].from.index == source[i].tokens[enabled[j].tokens[k]].index && arcs[petri::place::type][l].to.index != enabled[j].index)
							firable = false;
				}

				if (firable)
				{
					bool remotable = true;
					sort(enabled[j].tokens.begin(), enabled[j].tokens.end());
					for (int k = (int)enabled[j].tokens.size()-1; k >= 0; k--)
					{
						remotable = remotable && source[i].tokens[enabled[j].tokens[k]].remotable;
						source[i].tokens.erase(source[i].tokens.begin() + enabled[j].tokens[k]);
					}

					vector<int> n = next(transition::type, enabled[j].index);
					for (int k = 0; k < (int)n.size(); k++)
						source[i].tokens.push_back(reset_token(n[k], remotable));

					if (transitions[enabled[j].index].behavior == transition::active)
					{
						source[i].encodings = local_assign(source[i].encodings, transitions[enabled[j].index].local_action, true);
						source[i].encodings = remote_assign(source[i].encodings, transitions[enabled[j].index].remote_action, true);
					}
					else
						source[i].encodings &= transitions[enabled[j].index].local_action;

					change = true;
				}
			}
		}

		for (int i = 0; i < (int)reset.size(); i++)
		{
			vector<petri::enabled_transition<term_index> > enabled = this->enabled<reset_token, petri::enabled_transition<term_index> >(reset[i].tokens, false);

			change = false;
			for (int j = 0; j < (int)enabled.size() && !change; j++)
			{
				bool firable = transitions[enabled[j].index].local_action.cubes.size() <= 1;
				for (int k = 0; k < (int)enabled[j].tokens.size() && firable; k++)
				{
					for (int l = 0; l < (int)arcs[petri::transition::type].size() && firable; l++)
						if (arcs[petri::transition::type][l].to.index == reset[i].tokens[enabled[j].tokens[k]].index)
							firable = false;
					for (int l = 0; l < (int)arcs[petri::place::type].size() && firable; l++)
						if (arcs[petri::place::type][l].from.index == reset[i].tokens[enabled[j].tokens[k]].index && arcs[petri::place::type][l].to.index != enabled[j].index)
							firable = false;
				}

				if (firable)
				{
					bool remotable = true;
					sort(enabled[j].tokens.begin(), enabled[j].tokens.end());
					for (int k = (int)enabled[j].tokens.size()-1; k >= 0; k--)
					{
						remotable = remotable && reset[i].tokens[enabled[j].tokens[k]].remotable;
						reset[i].tokens.erase(reset[i].tokens.begin() + enabled[j].tokens[k]);
					}

					vector<int> n = next(transition::type, enabled[j].index);
					for (int k = 0; k < (int)n.size(); k++)
						reset[i].tokens.push_back(reset_token(n[k], remotable));

					if (transitions[enabled[j].index].behavior == transition::active)
					{
						reset[i].encodings = local_assign(reset[i].encodings, transitions[enabled[j].index].local_action, true);
						reset[i].encodings = remote_assign(reset[i].encodings, transitions[enabled[j].index].remote_action, true);
					}
					else
						reset[i].encodings &= transitions[enabled[j].index].local_action;

					change = true;
				}
			}
		}
	}

	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++)
	{
		for (petri::iterator j = begin(petri::transition::type); j < end(petri::transition::type); j++)
			if (is_reachable(j, i))
				places[i.index].mask = places[i.index].mask.combine_mask(transitions[j.index].local_action.mask());
		places[i.index].mask = places[i.index].mask.flip();
	}

	if (reset.size() == 0)
		reset = source;
}

void graph::synchronize()
{
	half_synchronization sync;
	for (sync.passive.index = 0; sync.passive.index < (int)transitions.size(); sync.passive.index++)
		if (transitions[sync.passive.index].behavior == transition::passive && !transitions[sync.passive.index].local_action.is_tautology())
			for (sync.active.index = 0; sync.active.index < (int)transitions.size(); sync.active.index++)
				if (transitions[sync.active.index].behavior == transition::active && !transitions[sync.active.index].local_action.is_tautology())
					for (sync.passive.cube = 0; sync.passive.cube < (int)transitions[sync.passive.index].local_action.cubes.size(); sync.passive.cube++)
						for (sync.active.cube = 0; sync.active.cube < (int)transitions[sync.active.index].local_action.cubes.size(); sync.active.cube++)
							if (similarity_g0(transitions[sync.active.index].local_action.cubes[sync.active.cube], transitions[sync.passive.index].local_action.cubes[sync.passive.cube]))
								synchronizations.push_back(sync);
}

void graph::petrify(const ucs::variable_set &variables)
{
	// Petri nets don't support internal parallelism or choice, so we have to expand those transitions.
	for (petri::iterator i(petri::transition::type, transitions.size()-1); i >= 0; i--)
	{
		if (transitions[i.index].behavior == transition::active && transitions[i.index].local_action.cubes.size() > 1)
		{
			vector<petri::iterator> k = duplicate(petri::choice, i, transitions[i.index].local_action.cubes.size(), false);
			for (int j = 0; j < (int)k.size(); j++)
				transitions[k[j].index].local_action = boolean::cover(transitions[k[j].index].local_action.cubes[j]);
		}
	}

	for (petri::iterator i(petri::transition::type, transitions.size()-1); i >= 0; i--)
	{
		if (transitions[i.index].behavior == transition::active && transitions[i.index].local_action.cubes.size() == 1)
		{
			vector<int> vars = transitions[i.index].local_action.cubes[0].vars();
			if (vars.size() > 1)
			{
				vector<petri::iterator> k = duplicate(petri::parallel, i, vars.size(), false);
				for (int j = 0; j < (int)k.size(); j++)
					transitions[k[j].index].local_action.cubes[0] = boolean::cube(vars[j], transitions[k[j].index].local_action.cubes[0].get(vars[j]));
			}
		}
	}

	// Find all of the guards
	vector<petri::iterator> passive;
	for (petri::iterator i(petri::transition::type, transitions.size()-1); i >= 0; i--)
		if (transitions[i.index].behavior == transition::passive)
			passive.push_back(i);

	synchronize();

	/* Implement each guard in the petri net. As of now, this is done
	 * syntactically meaning that a few assumptions are made. First, if
	 * there are two transitions that affect the same variable in a guard,
	 * then they must be composed in condition. If they are not, then the
	 * net will not be 1-bounded. Second, you cannot have two guards on
	 * separate conditional branches that both have a cube which checks the
	 * same value for the same variable. Once again, if you do, the net
	 * will not be one bounded. There are probably other issues that I haven't
	 * uncovered yet, but just beware.
	 */
	for (int i = 0; i < (int)passive.size(); i++)
	{
		if (!transitions[passive[i].index].local_action.is_tautology())
		{
			// Find all of the half synchronization local_actions for this guard and group them accordingly:
			// outside group is by disjunction, middle group is by conjunction, and inside group is by value and variable.
			vector<vector<vector<petri::iterator> > > sync;
			sync.resize(transitions[passive[i].index].local_action.cubes.size());
			for (int j = 0; j < (int)synchronizations.size(); j++)
				if (synchronizations[j].passive.index == passive[i].index)
				{
					bool found = false;
					for (int l = 0; l < (int)sync[synchronizations[j].passive.cube].size(); l++)
						if (similarity_g0(transitions[sync[synchronizations[j].passive.cube][l][0].index].local_action.cubes[0], transitions[synchronizations[j].active.index].local_action.cubes[0]))
						{
							found = true;
							sync[synchronizations[j].passive.cube][l].push_back(petri::iterator(petri::transition::type, synchronizations[j].active.index));
						}

					if (!found)
						sync[synchronizations[j].passive.cube].push_back(vector<petri::iterator>(1, petri::iterator(petri::transition::type, synchronizations[j].active.index)));
				}

			/* Finally implement the connections. The basic observation being that a guard affects when the next
			 * transition can fire.
			 */
			vector<petri::iterator> n = next(passive[i]);
			vector<petri::iterator> g = create(place(), n.size());
			for (int j = 0; j < (int)n.size(); j++)
				connect(g[j], next(n[j]));

			for (int j = 0; j < (int)sync.size(); j++)
			{
				petri::iterator t = create(transition());
				connect(t, g);
				for (int k = 0; k < (int)sync[j].size(); k++)
				{
					petri::iterator p = create(place());
					connect(p, t);
					connect(sync[j][k], p);
				}
			}
		}
	}

	// Remove the guards from the graph
	vector<petri::iterator> n = next(passive), p = prev(passive);
	passive.insert(passive.end(), n.begin(), n.end());
	passive.insert(passive.end(), p.begin(), p.end());
	sort(passive.begin(), passive.end());
	passive.resize(unique(passive.begin(), passive.end()) - passive.begin());

	erase(passive);

	synchronizations.clear();

	// Clean up
	post_process(variables);
}

void graph::elaborate(const ucs::variable_set &variables, bool report)
{
	parallel_nodes.clear();

	for (int i = 0; i < (int)places.size(); i++)
	{
		places[i].predicate = boolean::cover();
		places[i].effective = boolean::cover();
	}

	hashtable<state, 100000> states;
	vector<simulator> simulations;
	vector<deadlock> deadlocks;
	simulations.reserve(20000);

	// Set up the first simulation that starts at the reset state
	for (int i = 0; i < (int)source.size(); i++)
		for (int j = 0; j < (int)source[i].encodings.size(); j++)
			simulations.push_back(simulator(this, &variables, source[i], j, true));

	int count = 0;
	while (simulations.size() > 0)
	{
		simulator sim = simulations.back();
		simulations.pop_back();

		int enabled = sim.possible();
		if (enabled > 0)
		{
			for (int i = 0; i < (int)sim.remote.ready.size(); i++)
			{
				if (vector_intersection_size(sim.remote.ready[0].tokens, sim.remote.ready[i].tokens) > 0)
				{
					simulations.push_back(sim);
					simulations.back().begin(i);
					simulations.back().end();
					simulations.back().environment();
				}
			}
		}
		else
		{
			// Look the see if the resulting state is already in the state graph. If it is, then we are done with this simulation,
			// and if it is not, then we need to put the state in and continue on our way.
			state s0 = sim.get_state();
			state *s1 = NULL;
			s0.encodings.cubes[0] = s0.encodings.cubes[0].xoutnulls();

			bool found = !states.insert(s0, &s1);
			if (found && s1 != NULL && !s0.encodings.is_subset_of(s1->encodings))
			{
				*s1 = state::merge(petri::choice, *s1, s0);
				found = false;
			}

			if (found && simulations.size() > 0)
				simulations.back().merge_errors(sim);
			else if (!found)
			{
				int enabled = sim.enabled();
				vector<pair<int, int> > vacuous_choices = sim.get_vacuous_choices();
				int index = -1;
				for (int i = 0; i < (int)sim.local.ready.size() && index == -1; i++)
					if (sim.local.ready[i].vacuous)
					{
						index = i;
						for (int j = 0; j < (int)vacuous_choices.size() && index != -1; j++)
							if (vacuous_choices[j].first == i || vacuous_choices[j].second == i)
								index = -1;
					}

				if (index != -1)
				{
					simulations.push_back(sim);
					simulations.back().fire(index);
				}
				else if (vacuous_choices.size() > 0)
				{
					for (int i = 0; i < (int)sim.local.ready.size(); i++)
						if (sim.local.ready[i].vacuous)
						{
							simulations.push_back(sim);
							enabled_transition e = simulations.back().local.ready[i];

							// Fire the transition in the simulation
							simulations.back().fire(i);
						}
				}
				else
				{
					// Handle the local tokens
					if (report)
						progress("", to_string(count) + " " + to_string(simulations.size()) + " " + to_string(states.max_bucket_size()) + "/" + to_string(states.count) + " " + to_string(enabled), __FILE__, __LINE__);

					for (int i = 0; i < (int)sim.local.ready.size(); i++)
					{
						simulations.push_back(sim);
						enabled_transition e = simulations.back().local.ready[i];

						for (int j = 0; j < (int)e.tokens.size(); j++)
							places[simulations.back().local.tokens[e.tokens[j]].index].predicate |= simulations.back().encoding.flipped_mask(places[simulations.back().local.tokens[e.tokens[j]].index].mask);

						// Fire the transition in the simulation
						simulations.back().fire(i);
					}

					if (sim.local.ready.size() == 0)
					{
						deadlock d = sim.get_state();
						vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
						if (dloc == deadlocks.end() || *dloc != d)
						{
							error("", d.to_string(variables), __FILE__, __LINE__);
							deadlocks.insert(dloc, d);
						}

						if (simulations.size() > 0)
							simulations.back().merge_errors(sim);
					}

					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
					{
						for (int j = i+1; j < (int)sim.local.tokens.size(); j++)
							parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.local.tokens[i].index), petri::iterator(petri::place::type, sim.local.tokens[j].index)));
						for (int j = 0; j < (int)sim.local.ready.size(); j++)
							if (find(sim.local.ready[j].tokens.begin(), sim.local.ready[j].tokens.end(), i) == sim.local.ready[j].tokens.end())
								parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.local.tokens[i].index), petri::iterator(petri::transition::type, sim.local.ready[j].index)));
					}

					/* The effective predicate represents the state encodings that don't have duplicates
					 * in later states.
					 *
					 * TODO check the wchb
					 */
					boolean::cover en = 1;
					for (int i = 0; i < (int)sim.local.ready.size(); i++)
					{
						if (transitions[sim.local.ready[i].index].behavior == transition::active)
							en &= ~transitions[sim.local.ready[i].index].local_action.cubes[sim.local.ready[i].term];
						else
							en &= ~transitions[sim.local.ready[i].index].local_action;
					}

					for (int i = 0; i < (int)sim.local.tokens.size(); i++)
						places[sim.local.tokens[i].index].effective |= (sim.encoding.xoutnulls() & en).flipped_mask(places[sim.local.tokens[i].index].mask);//acting[i].second);

					count++;
				}
			}
		}
	}

	if (report)
		done_progress();

	sort(parallel_nodes.begin(), parallel_nodes.end());
	parallel_nodes.resize(unique(parallel_nodes.begin(), parallel_nodes.end()) - parallel_nodes.begin());
	parallel_nodes_ready = true;

	for (int i = 0; i < (int)places.size(); i++)
	{
		places[i].effective.espresso();
		sort(places[i].effective.cubes.begin(), places[i].effective.cubes.end());
		places[i].predicate.espresso();
	}
}

/** to_state_graph()
 *
 * This converts a given graph to the fully expanded state space through simulation. It systematically
 * simulates all possible transition orderings and determines all of the resulting state information.
 */
graph graph::to_state_graph(const ucs::variable_set &variables)
{
	graph result;
	vector<pair<state, petri::iterator> > states;
	vector<pair<simulator, petri::iterator> > simulations;
	vector<deadlock> deadlocks;
	vector<pair<int, int> > to_merge;

	for (int i = 0; i < (int)source.size(); i++)
	{
		for (int j = 0; j < (int)source[i].encodings.cubes.size(); j++)
		{
			// Set up the initial state which is determined by the reset behavior
			petri::iterator init = result.create(place(source[i].encodings.cubes[j]));
			result.source.push_back(state(vector<reset_token>(1, reset_token(init.index, false)), vector<term_index>(), source[i].encodings.cubes[j]));

			// Set up the first simulation that starts at the reset state
			simulations.push_back(pair<simulator, petri::iterator>(simulator(this, &variables, source[i], j, false), init));

			// Record the reset state in our map of visited states
			state s = simulations.back().first.get_state();
			vector<pair<state, petri::iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<state, petri::iterator>(s, petri::iterator()));
			if (loc != states.end() && loc->first == s)
				loc->first = state::merge(petri::choice, loc->first, s);
			else
				states.insert(loc, pair<state, petri::iterator>(s, init));
		}
	}

	while (simulations.size() > 0)
	{
		pair<simulator, petri::iterator> sim = simulations.back();
		simulations.pop_back();

		int enabled = sim.first.enabled();
		simulations.reserve(simulations.size() + enabled);
		for (int i = 0; i < enabled; i++)
		{
			simulations.push_back(sim);
			enabled_transition e = simulations.back().first.local.ready[i];

			if (transitions[e.index].behavior == transition::active)
			{
				// Look to see if this transition has already been drawn in the state graph
				// We know by construction that every transition in the state graph will have exactly one cube
				// and that every transition will be active
				petri::iterator n(petri::place::type, -1);
				for (int j = 0; j < (int)result.arcs[petri::place::type].size() && n.index == -1; j++)
					if (result.arcs[petri::place::type][j].from == simulations.back().second &&
						result.transitions[result.arcs[petri::place::type][j].to.index].local_action.cubes[0] == transitions[e.index].local_action.cubes[e.term])
					{
						simulations.back().second = result.arcs[petri::place::type][j].to;
						for (int k = 0; k < (int)result.arcs[petri::transition::type].size() && n == -1; k++)
							if (result.arcs[petri::transition::type][k].from == simulations.back().second)
								n = result.arcs[petri::transition::type][k].to;
					}

				// Since it isn't already in the state graph, we need to draw it
				if (n.index == -1)
					simulations.back().second = result.push_back(simulations.back().second, transitions[e.index].subdivide(e.term));

				// Fire the transition in the simulation
				simulations.back().first.fire(i);

				// Look the see if the resulting state is already in the state graph. If it is, then we are done with this simulation,
				// and if it is not, then we need to put the state in and continue on our way.
				state s = simulations.back().first.get_state();
				vector<pair<state, petri::iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<state, petri::iterator>(s, petri::iterator()));
				if (loc != states.end() && loc->first == s)
				{
					if (simulations.size() > 1)
						simulations[simulations.size()-2].first.merge_errors(simulations.back().first);

					if (n.index == -1)
						result.connect(simulations.back().second, loc->second);
					else if (n.index != -1)
					{
						// If we find duplicate records of the same state, then we'll need to merge them later.
						if (loc->second.index > n.index)
							to_merge.push_back(pair<int, int>(loc->second.index, n.index));
						else if (n.index > loc->second.index)
							to_merge.push_back(pair<int, int>(n.index, loc->second.index));
					}

					simulations.pop_back();
				}
				else if ((loc == states.end() || loc->first != s) && n.index == -1)
				{
					simulations.back().second = result.push_back(simulations.back().second, place(s.encodings));
					states.insert(loc, pair<state, petri::iterator>(s, simulations.back().second));
				}
				else
				{
					simulations.back().second = n;
					states.insert(loc, pair<state, petri::iterator>(s, simulations.back().second));
				}
			}
			else
			{
				simulations.back().first.fire(i);
				state s = simulations.back().first.get_state();
				vector<pair<state, petri::iterator> >::iterator loc = lower_bound(states.begin(), states.end(), pair<state, petri::iterator>(s, petri::iterator()));
				if (loc != states.end() && loc->first == s)
				{
					if (simulations.size() > 1)
						simulations[simulations.size()-2].first.merge_errors(simulations.back().first);

					// If we find duplicate records of the same state, then we'll need to merge them later.
					if (loc->second.index > simulations.back().second.index)
						to_merge.push_back(pair<int, int>(loc->second.index, simulations.back().second.index));
					else if (simulations.back().second.index > loc->second.index)
						to_merge.push_back(pair<int, int>(simulations.back().second.index, loc->second.index));

					simulations.pop_back();
				}
				else
					states.insert(loc, pair<state, petri::iterator>(s, simulations.back().second));
			}
		}

		if (enabled == 0)
		{
			deadlock d = sim.first.get_state();
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().first.merge_errors(sim.first);
		}
	}

	sort(to_merge.rbegin(), to_merge.rend());
	to_merge.resize(unique(to_merge.begin(), to_merge.end()) - to_merge.begin());
	for (int i = 0; i < (int)to_merge.size(); i++)
	{
		for (int j = 0; j < (int)result.arcs[petri::place::type].size(); j++)
		{
			if (result.arcs[petri::place::type][j].from.index == to_merge[i].first)
				result.arcs[petri::place::type][j].from.index = to_merge[i].second;
			else if (result.arcs[petri::place::type][j].from.index > to_merge[i].first)
				result.arcs[petri::place::type][j].from.index--;
		}
		for (int j = 0; j < (int)result.arcs[petri::transition::type].size(); j++)
		{
			if (result.arcs[petri::transition::type][j].to.index == to_merge[i].first)
				result.arcs[petri::transition::type][j].to.index = to_merge[i].second;
			else if (result.arcs[petri::transition::type][j].to.index > to_merge[i].first)
				result.arcs[petri::transition::type][j].to.index--;
		}
		for (int j = 0; j < (int)result.source.size(); j++)
			for (int k = 0; k < (int)result.source[j].tokens.size(); k++)
			{
				if (result.source[j].tokens[k].index == to_merge[i].first)
					result.source[j].tokens[k].index = to_merge[i].second;
				else if (result.source[j].tokens[k].index > to_merge[i].first)
					result.source[j].tokens[k].index--;
			}
		for (int j = 0; j < (int)result.sink.size(); j++)
			for (int k = 0; k < (int)result.sink[j].tokens.size(); k++)
			{
				if (result.sink[j].tokens[k].index == to_merge[i].first)
					result.sink[j].tokens[k].index = to_merge[i].second;
				else if (result.sink[j].tokens[k].index > to_merge[i].first)
					result.sink[j].tokens[k].index--;
			}

		result.places.erase(result.places.begin() + to_merge[i].first);
	}

	return result;
}

/* to_petri_net()
 *
 * Converts the HSE into a petri net using index-priority simulation.
 */
graph graph::to_petri_net()
{
	graph result;


	return result;
}

void graph::check_variables(const ucs::variable_set &variables)
{
	for (int i = 0; i < (int)variables.nodes.size(); i++)
	{
		vector<int> written;
		vector<int> read;

		for (int j = 0; j < (int)transitions.size(); j++)
		{
			vector<int> vars = transitions[j].remote_action.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end())
			{
				if (transitions[j].behavior == hse::transition::active)
					written.push_back(j);
				else
					read.push_back(j);
			}
		}

		if (written.size() == 0 && read.size() > 0)
			warning("", variables.nodes[i].to_string() + " never assigned", __FILE__, __LINE__);
		else if (written.size() == 0 && read.size() == 0)
			warning("", "unused variable " + variables.nodes[i].to_string(), __FILE__, __LINE__);
	}
}

}
