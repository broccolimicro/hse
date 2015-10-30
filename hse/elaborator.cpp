/*
 * elaborator.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: nbingham
 */

#include "elaborator.h"
#include <common/text.h>

namespace hse
{

void elaborate(graph &g, const ucs::variable_set &variables, bool report_progress)
{
	g.parallel_nodes.clear();

	for (int i = 0; i < (int)g.places.size(); i++)
	{
		g.places[i].predicate = boolean::cover();
		g.places[i].effective = boolean::cover();
	}

	hashtable<state, 10000> states;
	vector<simulator> simulations;
	vector<deadlock> deadlocks;
	simulations.reserve(2000);

	if (g.reset.size() > 0)
	{
		for (int i = 0; i < (int)g.reset.size(); i++)
		{
			simulator sim(&g, &variables, g.reset[i]);
			sim.enabled();

			if (states.insert(sim.get_state()))
				simulations.push_back(sim);
		}
	}
	else
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			simulator sim(&g, &variables, g.source[i]);
			sim.enabled();

			if (states.insert(sim.get_state()))
				simulations.push_back(sim);
		}
	}

	int count = 0;
	while (simulations.size() > 0)
	{
		simulator sim = simulations.back();
		simulations.pop_back();
		//simulations.back().merge_errors(sim);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulations.size()) + " " + ::to_string(states.max_bucket_size()) + "/" + ::to_string(states.count) + " " + ::to_string(sim.ready.size()), __FILE__, __LINE__);

		simulations.reserve(simulations.size() + sim.ready.size());
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			simulations.push_back(sim);

			simulations.back().fire(i);
			simulations.back().enabled();

			if (!states.insert(simulations.back().get_state()))
				simulations.pop_back();
		}

		if (sim.ready.size() == 0)
		{
			deadlock d = sim.get_state();
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().merge_errors(sim);
		}

		// Figure out which nodes are in parallel
		vector<vector<int> > tokens(1, vector<int>());
		vector<vector<int> > visited;
		for (int i = 0; i < (int)sim.tokens.size(); i++)
			if (sim.tokens[i].cause < 0)
				tokens[0].push_back(i);

		while (tokens.size() > 0)
		{
			vector<int> curr = tokens.back();
			sort(curr.begin(), curr.end());
			tokens.pop_back();

			vector<vector<int> >::iterator loc = lower_bound(visited.begin(), visited.end(), curr);
			if (loc == visited.end())
			{
				vector<int> enabled_loaded;
				for (int i = 0; i < (int)sim.loaded.size(); i++)
				{
					bool ready = true;
					for (int j = 0; j < (int)sim.loaded[i].tokens.size() && ready; j++)
						if (find(curr.begin(), curr.end(), sim.loaded[i].tokens[j]) == curr.end())
							ready = false;

					if (ready)
						enabled_loaded.push_back(i);
				}

				for (int i = 0; i < (int)curr.size(); i++)
				{
					for (int j = i+1; j < (int)curr.size(); j++)
					{
						if (sim.tokens[curr[i]].index <= sim.tokens[curr[j]].index)
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[i]].index), petri::iterator(petri::place::type, sim.tokens[curr[j]].index)));
						else
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[j]].index), petri::iterator(petri::place::type, sim.tokens[curr[i]].index)));
					}

					for (int j = 0; j < (int)enabled_loaded.size(); j++)
						if (find(sim.loaded[enabled_loaded[j]].tokens.begin(), sim.loaded[enabled_loaded[j]].tokens.end(), curr[i]) == sim.loaded[enabled_loaded[j]].tokens.end())
							g.parallel_nodes.push_back(pair<petri::iterator, petri::iterator>(petri::iterator(petri::place::type, sim.tokens[curr[i]].index), petri::iterator(petri::transition::type, sim.loaded[enabled_loaded[j]].index)));
				}

				for (int i = 0; i < (int)enabled_loaded.size(); i++)
				{
					tokens.push_back(curr);
					for (int j = (int)tokens.back().size()-1; j >= 0; j--)
						if (find(sim.loaded[enabled_loaded[i]].tokens.begin(), sim.loaded[enabled_loaded[i]].tokens.end(), tokens.back()[j]) != sim.loaded[enabled_loaded[i]].tokens.end())
							tokens.back().erase(tokens.back().begin() + j);

					tokens.back().insert(tokens.back().end(), sim.loaded[enabled_loaded[i]].output_marking.begin(), sim.loaded[enabled_loaded[i]].output_marking.end());
				}

				visited.insert(loc, curr);
			}
		}

		/* The effective predicate represents the state encodings that don't have duplicates
		 * in later states.
		 *
		 * I have to loop through all of the enabled transitions, and then loop through all orderings
		 * of their dependent guards, saving the state
		 */
		vector<set<int> > en_in(sim.tokens.size(), set<int>());
		vector<set<int> > en_out(sim.tokens.size(), set<int>());

		bool change = true;
		while (change)
		{
			change = false;
			for (int i = 0; i < (int)sim.loaded.size(); i++)
			{
				set<int> total_in;
				for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++)
					total_in.insert(en_in[sim.loaded[i].tokens[j]].begin(), en_in[sim.loaded[i].tokens[j]].end());
				total_in.insert(sim.loaded[i].index);

				for (int j = 0; j < (int)sim.loaded[i].output_marking.size(); j++)
				{
					set<int> old_in = en_in[sim.loaded[i].output_marking[j]];
					en_in[sim.loaded[i].output_marking[j]].insert(total_in.begin(), total_in.end());
					if (en_in[sim.loaded[i].output_marking[j]] != old_in)
						change = true;
				}
			}
		}

		for (int i = 0; i < (int)sim.loaded.size(); i++)
			for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++)
				en_out[sim.loaded[i].tokens[j]].insert(sim.loaded[i].index);

		for (int i = 0; i < (int)sim.tokens.size(); i++)
		{
			boolean::cover en = 1;
			boolean::cover dis = 1;
			for (set<int>::iterator j = en_in[i].begin(); j != en_in[i].end(); j++)
				en &= g.transitions[*j].local_action;

			for (set<int>::iterator j = en_out[i].begin(); j != en_out[i].end(); j++)
				dis &= ~g.transitions[*j].local_action;

			g.places[sim.tokens[i].index].predicate |= (sim.encoding.xoutnulls() & en).flipped_mask(g.places[sim.tokens[i].index].mask);
			g.places[sim.tokens[i].index].effective |= (sim.encoding.xoutnulls() & en & dis).flipped_mask(g.places[sim.tokens[i].index].mask);
		}

		count++;
	}

	if (report_progress)
		done_progress();

	sort(g.parallel_nodes.begin(), g.parallel_nodes.end());
	g.parallel_nodes.resize(unique(g.parallel_nodes.begin(), g.parallel_nodes.end()) - g.parallel_nodes.begin());
	g.parallel_nodes_ready = true;

	for (int i = 0; i < (int)g.places.size(); i++)
	{
		if (report_progress)
			progress("", "Espresso " + ::to_string(i) + "/" + ::to_string(g.places.size()), __FILE__, __LINE__);

		g.places[i].effective.espresso();
		sort(g.places[i].effective.cubes.begin(), g.places[i].effective.cubes.end());
		g.places[i].predicate.espresso();
	}

	if (report_progress)
		done_progress();
}

struct simulation
{
	simulation() {}
	simulation(simulator exec, petri::iterator node)
	{
		this->exec = exec;
		this->node = node;
	}
	~simulation() {}

	simulator exec;
	petri::iterator node;
};

/** to_state_graph()
 *
 * This converts a given graph to the fully expanded state space through simulation. It systematically
 * simulates all possible transition orderings and determines all of the resulting state information.
 */
graph to_state_graph(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result;
	hashmap<state, petri::iterator, 10000> states;
	vector<simulation> simulations;
	vector<deadlock> deadlocks;

	if (g.reset.size() > 0)
	{
		for (int i = 0; i < (int)g.reset.size(); i++)
		{
			simulator sim(&g, &variables, g.reset[i]);
			sim.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(sim.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));

				// Set up the initial state which is determined by the reset behavior
				result.reset.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.reset[i].encodings));

				// Set up the first simulation that starts at the reset state
				simulations.push_back(simulation(sim, loc->second));
			}
		}
	}
	else
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			simulator sim(&g, &variables, g.source[i]);
			sim.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(sim.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));

				// Set up the initial state which is determined by the reset behavior
				result.reset.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.source[i].encodings));

				// Set up the first simulation that starts at the reset state
				simulations.push_back(simulation(sim, loc->second));
			}
		}
	}

	int count = 0;
	while (simulations.size() > 0)
	{
		simulation sim = simulations.back();
		simulations.pop_back();
		simulations.back().exec.merge_errors(sim.exec);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulations.size()) + " " + ::to_string(states.max_bucket_size()) + "/" + ::to_string(states.count) + " " + ::to_string(sim.exec.ready.size()), __FILE__, __LINE__);

		simulations.reserve(simulations.size() + sim.exec.ready.size());
		for (int i = 0; i < (int)sim.exec.ready.size(); i++)
		{
			simulations.push_back(sim);
			int index = simulations.back().exec.loaded[simulations.back().exec.ready[i].first].index;
			int term = simulations.back().exec.ready[i].second;

			simulations.back().exec.fire(i);
			simulations.back().exec.enabled();

			map<state, petri::iterator>::iterator loc;
			if (states.insert(simulations.back().exec.get_key(), petri::iterator(), &loc))
			{
				loc->second = result.create(place(loc->first.encodings));
				petri::iterator trans = result.create(g.transitions[index].subdivide(term));
				result.connect(simulations.back().node, trans);
				result.connect(trans, loc->second);
				simulations.back().node = loc->second;
			}
			else
			{
				petri::iterator trans = result.create(g.transitions[index].subdivide(term));
				result.connect(simulations.back().node, trans);
				result.connect(trans, loc->second);
				simulations.pop_back();
			}
		}

		if (sim.exec.ready.size() == 0)
		{
			deadlock d = sim.exec.get_state();
			result.sink.push_back(state(vector<petri::token>(1, petri::token(sim.node.index)), d.encodings));
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulations.back().exec.merge_errors(sim.exec);
		}

		count++;
	}

	for (int i = 0; i < (int)g.source.size(); i++)
	{
		simulator sim(&g, &variables, g.source[i]);
		sim.enabled();

		map<state, petri::iterator>::iterator loc;
		if (states.insert(sim.get_key(), petri::iterator(), &loc))
			loc->second = result.create(place(loc->first.encodings));

		// Set up the initial state which is determined by the reset behavior
		result.source.push_back(state(vector<petri::token>(1, petri::token(loc->second.index)), g.source[i].encodings));
	}

	if (report_progress)
		done_progress();

	return result;
}

/* to_petri_net()
 *
 * Converts the HSE into a petri net using index-priority simulation.
 */
graph to_petri_net(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result;
	map<pair<int, int>, int> transitions;
	hashtable<state, 10000> states;
	vector<simulator> simulators;
	vector<deadlock> deadlocks;

	if (g.reset.size() > 0)
	{
		for (int i = 0; i < (int)g.reset.size(); i++)
		{
			simulator sim(&g, &variables, g.reset[i]);
			sim.enabled();

			// Set up the first simulator that starts at the reset state
			if (states.insert(sim.get_state()))
				simulators.push_back(simulator(sim));
		}
	}
	else
	{
		for (int i = 0; i < (int)g.source.size(); i++)
		{
			simulator sim(&g, &variables, g.source[i]);
			sim.enabled();

			// Set up the first simulator that starts at the reset state
			if (states.insert(sim.get_state()))
				simulators.push_back(simulator(sim));
		}
	}

	int count = 0;
	while (simulators.size() > 0)
	{
		simulator sim = simulators.back();
		simulators.pop_back();
		simulators.back().merge_errors(sim);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulators.size()) + " " + ::to_string(sim.ready.size()), __FILE__, __LINE__);

		simulators.reserve(simulators.size() + sim.ready.size());
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			simulators.push_back(sim);
			vector<int> guard = simulators.back().loaded[simulators.back().ready[i].first].guard_action.vars();
			int index = simulators.back().loaded[simulators.back().ready[i].first].index;
			int term = simulators.back().ready[i].second;

			simulators.back().fire(i);
			simulators.back().enabled();

			pair<int, int> tid = pair<int, int>(sim.loaded[sim.ready[i].first].index, sim.ready[i].second);
			map<pair<int, int>, int>::iterator loc = transitions.find(tid);
			if (loc == transitions.end())
				loc = transitions.insert(pair<pair<int, int>, int>(tid, result.create(g.transitions[tid.first].subdivide(tid.second)).index)).first;

			vector<petri::iterator> p = result.prev(petri::iterator(transition::type, loc->second));
			vector<vector<petri::iterator> > pt;
			for (int j = 0; j < (int)p.size(); j++)
				pt.push_back(result.prev(p));

			vector<pair<int, int> > matches;
			for (map<pair<int, int>, int>::iterator tids = transitions.begin(); tids != transitions.end(); tids++)
				if ((g.transitions[tid.first].remote_action[tid.second].inverse() & sim.loaded[tid.first].guard_action).is_null())
				{
					bool found = false;
					for (int j = 0; j < (int)pt.size() && !found; j++)
						for (int k = 0; k < (int)pt[j].size() && !found; k++)
							if (pt[j][k].index == tids->second)
							{
								matches.push_back(pair<int, int>(j, tids->second));
								found = true;
							}
				}

			if (matches.size() > 0)
			{
				sort(matches.begin(), matches.end());
				bool duplicate = false;
				for (int j = 1; j < (int)matches.size(); )
				{
					if (matches[j].first == matches[j-1].first)
					{
						duplicate = true;
						matches.erase(matches.begin() + j);
					}
					else if (duplicate)
					{
						matches.erase(matches.begin() + j-1);
						duplicate = false;
					}
					else
						j++;
				}
			}

			for (int j = (int)matches.size()-1; j >= 0; j--)
				p.erase(p.begin() + matches[j].first);

			if (p.size() == 0)
			{

			}

			if (!states.insert(sim.get_state()))
				simulators.pop_back();
		}

		if (sim.ready.size() == 0)
		{
			deadlock d = sim.get_state();
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}

			simulators.back().merge_errors(sim);
		}

		count++;
	}

	if (report_progress)
		done_progress();

	return result;
}

}
