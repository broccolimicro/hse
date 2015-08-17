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
			vector<petri::enabled_transition> enabled = this->enabled<petri::token, petri::enabled_transition>(source[i].tokens, false);

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
					sort(enabled[j].tokens.begin(), enabled[j].tokens.end());
					for (int k = (int)enabled[j].tokens.size()-1; k >= 0; k--)
						source[i].tokens.erase(source[i].tokens.begin() + enabled[j].tokens[k]);

					vector<int> n = next(transition::type, enabled[j].index);
					for (int k = 0; k < (int)n.size(); k++)
						source[i].tokens.push_back(petri::token(n[k]));

					for (int k = (int)transitions[enabled[j].index].local_action.size()-1; k >= 0; k--)
					{
						if (k > 0)
						{
							source.push_back(source[i]);
							if (transitions[enabled[j].index].behavior == transition::active)
							{
								source.back().encodings = local_assign(source.back().encodings, transitions[enabled[j].index].local_action.cubes[k], true);
								source.back().encodings = remote_assign(source.back().encodings, transitions[enabled[j].index].remote_action.cubes[k], true);
							}
							else
								source.back().encodings &= transitions[enabled[j].index].local_action.cubes[k];
						}
						else
						{
							if (transitions[enabled[j].index].behavior == transition::active)
							{
								source[i].encodings = local_assign(source[i].encodings, transitions[enabled[j].index].local_action.cubes[0], true);
								source[i].encodings = remote_assign(source[i].encodings, transitions[enabled[j].index].remote_action.cubes[0], true);
							}
							else
								source[i].encodings &= transitions[enabled[j].index].local_action.cubes[0];
						}
					}

					change = true;
				}
			}
		}

		for (int i = 0; i < (int)reset.size(); i++)
		{
			vector<petri::enabled_transition> enabled = this->enabled<petri::token, petri::enabled_transition>(reset[i].tokens, false);

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
					sort(enabled[j].tokens.begin(), enabled[j].tokens.end());
					for (int k = (int)enabled[j].tokens.size()-1; k >= 0; k--)
						reset[i].tokens.erase(reset[i].tokens.begin() + enabled[j].tokens[k]);

					vector<int> n = next(transition::type, enabled[j].index);
					for (int k = 0; k < (int)n.size(); k++)
						reset[i].tokens.push_back(petri::token(n[k]));

					for (int k = (int)transitions[enabled[j].index].local_action.size()-1; k >= 0; k--)
					{
						if (k > 0)
						{
							reset.push_back(reset[i]);
							if (transitions[enabled[j].index].behavior == transition::active)
							{
								reset.back().encodings = local_assign(reset.back().encodings, transitions[enabled[j].index].local_action.cubes[k], true);
								reset.back().encodings = remote_assign(reset.back().encodings, transitions[enabled[j].index].remote_action.cubes[k], true);
							}
							else
								reset.back().encodings &= transitions[enabled[j].index].local_action.cubes[k];
						}
						else
						{
							if (transitions[enabled[j].index].behavior == transition::active)
							{
								reset[i].encodings = local_assign(reset[i].encodings, transitions[enabled[j].index].local_action.cubes[0], true);
								reset[i].encodings = remote_assign(reset[i].encodings, transitions[enabled[j].index].remote_action.cubes[0], true);
							}
							else
								reset[i].encodings &= transitions[enabled[j].index].local_action.cubes[0];
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

}
