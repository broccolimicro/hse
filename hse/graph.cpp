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

transition::transition(int behavior, boolean::cover local_action, boolean::cover remote_action)
{
	this->behavior = behavior;
	this->local_action = local_action;
	this->remote_action = remote_action;
}

transition::~transition()
{

}

transition transition::subdivide(int term) const
{
	if (term < (int)local_action.cubes.size() && term < (int)remote_action.cubes.size())
		return transition(behavior, boolean::cover(local_action.cubes[term]), boolean::cover(remote_action.cubes[term]));
	else if (term < (int)local_action.cubes.size())
		return transition(behavior, boolean::cover(local_action.cubes[term]));
	else
		return transition(behavior);
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

map<petri::iterator, petri::iterator> graph::merge(int composition, const graph &g)
{
	map<petri::iterator, petri::iterator> result = super::merge(composition, g);

	for (int i = 0; i < (int)g.arbiters.size(); i++)
	{

		map<petri::iterator, petri::iterator>::iterator first_loc = result.find(petri::iterator(hse::transition::type, g.arbiters[i].first));
		map<petri::iterator, petri::iterator>::iterator second_loc = result.find(petri::iterator(hse::transition::type, g.arbiters[i].second));

		if (first_loc != result.end() && second_loc != result.end())
			arbiters.push_back(pair<int, int>(first_loc->second.index, second_loc->second.index));
	}

	return result;
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
			petri::simulator<hse::place, hse::transition, petri::token, hse::state> sim(this, source[i]);
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

				if (firable)
				{
					petri::enabled_transition t = sim.fire(j);
					source[i].tokens = sim.tokens;
					for (int k = (int)transitions[t.index].local_action.size()-1; k >= 0; k--)
					{
						if (k > 0)
						{
							source.push_back(source[i]);
							if (transitions[t.index].behavior == transition::active)
							{
								source.back().encodings = local_assign(source.back().encodings, transitions[t.index].local_action.cubes[k], true);
								source.back().encodings = remote_assign(source.back().encodings, transitions[t.index].remote_action.cubes[k], true);
							}
							else
								source.back().encodings &= transitions[t.index].local_action.cubes[k];
						}
						else
						{
							if (transitions[t.index].behavior == transition::active)
							{
								source[i].encodings = local_assign(source[i].encodings, transitions[t.index].local_action.cubes[0], true);
								source[i].encodings = remote_assign(source[i].encodings, transitions[t.index].remote_action.cubes[0], true);
							}
							else
								source[i].encodings &= transitions[t.index].local_action.cubes[0];
						}
					}

					change = true;
				}
			}
		}

		for (int i = 0; i < (int)reset.size(); i++)
		{
			petri::simulator<hse::place, hse::transition, petri::token, hse::state> sim(this, reset[i]);
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

				if (firable)
				{
					petri::enabled_transition t = sim.fire(j);
					reset[i].tokens = sim.tokens;

					for (int k = (int)transitions[t.index].local_action.size()-1; k >= 0; k--)
					{
						if (k > 0)
						{
							reset.push_back(reset[i]);
							if (transitions[t.index].behavior == transition::active)
							{
								reset.back().encodings = local_assign(reset.back().encodings, transitions[t.index].local_action.cubes[k], true);
								reset.back().encodings = remote_assign(reset.back().encodings, transitions[t.index].remote_action.cubes[k], true);
							}
							else
								reset.back().encodings &= transitions[t.index].local_action.cubes[k];
						}
						else
						{
							if (transitions[t.index].behavior == transition::active)
							{
								reset[i].encodings = local_assign(reset[i].encodings, transitions[t.index].local_action.cubes[0], true);
								reset[i].encodings = remote_assign(reset[i].encodings, transitions[t.index].remote_action.cubes[0], true);
							}
							else
								reset[i].encodings &= transitions[t.index].local_action.cubes[0];
						}
					}

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
		if (sim.enabled() > 0)
		{
			for (int i = 0; i < (int)sim.ready.size(); i++)
			{
				if (transitions[sim.ready[i].index].behavior == transition::passive)
				{
					simulators.push_back(sim);
					simulators.back().fire(i);
				}
				else
					result.push_back(sim.ready[i].index);
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
				if (transitions[n[i]].behavior == transition::active)
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

}
