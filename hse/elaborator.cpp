/*
 * elaborator.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: nbingham
 */

#include "elaborator.h"
#include <common/text.h>
#include <interpret_boolean/export.h>

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
		if (simulations.size() > 0)
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

struct firing
{
	boolean::cube guard;
	term_index action;
	state loc;
};

bool operator==(firing f0, firing f1)
{
	return f0.guard == f1.guard && f0.action == f1.action && f0.loc == f1.loc;
}

struct cycle
{
	vector<firing> firings;
};

bool operator==(cycle f0, cycle f1)
{
	if (f0.firings.size() != f1.firings.size())
		return false;

	for (int i = 0; i < (int)f0.firings.size(); i++)
	{
		bool equal = true;
		for (int j = 0; j < (int)f0.firings.size() && equal; j++)
			equal = (f0.firings[(i+j)%(int)f0.firings.size()].action == f1.firings[j].action && f0.firings[(i+j)%(int)f0.firings.size()].guard == f1.firings[j].guard);

		if (equal)
			return true;
	}

	return false;
}

struct frame
{
	frame() {}
	frame(simulator sim)
	{
		this->sim = sim;
		if (sim.variables != NULL)
			this->indices.resize(sim.variables->nodes.size(), 0);
	}
	~frame() {}

	cycle part;
	vector<int> indices;
	simulator sim;
};

vector<cycle> get_cycles(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	vector<cycle> result;
	vector<frame> frames;
	//hashtable<state, 200> states;
	if (g.reset.size() > 0)
		for (int i = 0; i < (int)g.reset.size(); i++)
			frames.push_back(frame(simulator(&g, &variables, g.reset[i])));
	else
		for (int i = 0; i < (int)g.source.size(); i++)
			frames.push_back(frame(simulator(&g, &variables, g.source[i])));

	int iteration = -1;
	while (frames.size() > 0)
	{
		iteration++;
		cout << "Iteration " << iteration << ": " << frames.size() << " frames left" << endl;
		frame curr = frames.back();
		frames.pop_back();

		curr.sim.enabled();
		vector<pair<int, int> > choices = curr.sim.get_choices();


		cout << "Enabled Assignments: ";
		for (int i = 0; i < (int)curr.sim.ready.size(); i++)
		{
			for (int j = i+1; j < (int)curr.sim.ready.size(); j++)
				if (find(choices.begin(), choices.end(), pair<int, int>(curr.sim.ready[i].first, curr.sim.ready[j].first)) != choices.end())
				{
					if (i != 0 || j != 1)
						cout << " ";
					cout << "(" << i << ", " << j << ")";
				}
		}
		cout << endl;
		for (int i = 0; i < (int)curr.sim.ready.size(); i++)
			cout << "(" << i << ")\t" << export_expression(curr.sim.loaded[curr.sim.ready[i].first].guard_action, variables).to_string() << " -> " << export_composition(g.transitions[curr.sim.loaded[curr.sim.ready[i].first].index].local_action.cubes[curr.sim.ready[i].second], variables).to_string() << endl;


		vector<int> excluded;
		// tindices each element in tindices will have three elements
		// tindices[i][0] = the calculated index for the transition as in index priority simulation
		// tindices[i][1] = the number of transitions that are mutually exclusive from this one due to choice
		// tindices[i][2] = the index in the list of ready transitions in curr.sim
		vector<vector<int> > tindices;

		// figure out the index for each transition in the list of ready transitions in curr.sim
		for (int i = 0; i < (int)curr.sim.ready.size(); i++)
		{
			// initialize the excluded list while we are at it
			excluded.push_back(i);

			// set up the initial indices values
			tindices.push_back(vector<int>());
			// we need to take the max, so we need to start out with something thats lower than any possible index. indices start at 0
			tindices.back().push_back(-1);
			// start without any information about choices
			tindices.back().push_back(0);
			// this tindex represents ready[i]
			tindices.back().push_back(i);

			// For transitions that affect multiple variables, take the max of the indices of those variables
			vector<int> tvars = g.transitions[curr.sim.loaded[curr.sim.ready[i].first].index].local_action[curr.sim.ready[i].second].vars();
			for (int j = 0; j < (int)tvars.size(); j++)
				if (curr.indices[tvars[j]] > tindices.back()[0])
					tindices.back()[0] = curr.indices[tvars[j]];
		}

		// Now we need to choose multiple transitions to fire. The fewer the better.
		// So we have two sorting criteria: maximize their index, and minimize the number of transitions they exclude from choice
		//int chosen = -1;
		while (excluded.size() > 0 && tindices.size() > 0)
		{
			// store the old list of excluded transitions, we'll use these to populate the new list
			vector<int> old = excluded;
			excluded.clear();

			// the list of possibly excluded transitions for each tindex
			vector<vector<int> > possible(curr.sim.ready.size(), vector<int>());
			for (int i = 0; i < (int)tindices.size(); i++)
			{
				for (int j = 0; j < (int)old.size(); j++)
					if ((curr.sim.ready[tindices[i][2]].first < curr.sim.ready[old[j]].first && find(choices.begin(), choices.end(), pair<int, int>(curr.sim.ready[tindices[i][2]].first, curr.sim.ready[old[j]].first)) != choices.end()) ||
						(curr.sim.ready[old[j]].first < curr.sim.ready[tindices[i][2]].first && find(choices.begin(), choices.end(), pair<int, int>(curr.sim.ready[old[j]].first, curr.sim.ready[tindices[i][2]].first)) != choices.end()))
						possible[tindices[i][2]].push_back(old[j]);

				// minimizing the number of excluded transitions is the same as maximizing the number of included ones
				tindices[i][1] = tindices.size() - possible[tindices[i][2]].size();
			}

			sort(tindices.begin(), tindices.end());

			cout << "Indices:" << endl;
			for (int i = 0; i < (int)tindices.size(); i++)
			{
				cout << tindices[i][0] << " " << tindices[i][1] << " " << tindices[i][2] << " (";
				for (int j = 0; j < (int)possible[tindices[i][2]].size(); j++)
				{
					if (j != 0)
						cout << " ";
					cout << possible[tindices[i][2]][j];
				}
				cout << ")" << endl;
			}

			// find the next tindex that we haven't already fired.
			vector<int> index = tindices.back();

			// TODO check to make sure this assumption about minimal cycles with "chosen" actually holds true
			if (index.size() == 3/* && index[0] >= chosen*/)
			{
				//chosen = index[0];

				firing t;
				t.guard = curr.sim.loaded[curr.sim.ready[index[2]].first].guard_action;
				t.action = term_index(curr.sim.loaded[curr.sim.ready[index[2]].first].index, curr.sim.ready[index[2]].second);
				t.loc = curr.sim.get_state();

				vector<firing>::iterator loc = find(curr.part.firings.begin(), curr.part.firings.end(), t);
				if (loc != curr.part.firings.end())
				{
					curr.part.firings.erase(curr.part.firings.begin(), loc);
					if (find(result.begin(), result.end(), curr.part) == result.end())
						result.push_back(curr.part);
				}
				else// if (!states.contains(t.loc))
				{
					//states.insert(t.loc);
					excluded = possible[index[2]];
					frames.push_back(curr);
					frames.back().part.firings.push_back(t);
					frames.back().sim.fire(index[2]);

					vector<int> tvars = g.transitions[t.action.index].local_action.cubes[t.action.term].vars();
					for (int i = 0; i < (int)tvars.size(); i++)
						frames.back().indices[tvars[i]]++;

					for (int i = tindices.size()-1; i >= 0; i--)
						if (find(excluded.begin(), excluded.end(), tindices[i][2]) == excluded.end())
							tindices.erase(tindices.begin() + i);
				}
			}
		}
		cout << endl << endl;
	}

	return result;
}

/* to_petri_net()
 *
 * Converts the HSE into a petri net using index-priority simulation.
 */
graph to_petri_net(const graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result;

	vector<cycle> cycles = get_cycles(g, variables, report_progress);
	for (int i = 0; i < (int)cycles.size(); i++)
	{
		cout << "Cycle " << i << endl;
		for (int j = 0; j < (int)cycles[i].firings.size(); j++)
		{
			cout << "\t(" << cycles[i].firings[j].action.index << " " << cycles[i].firings[j].action.term << ")\t" << export_expression(cycles[i].firings[j].guard, variables).to_string() << " -> " << export_composition(g.transitions[cycles[i].firings[j].action.index].local_action.cubes[cycles[i].firings[j].action.term], variables).to_string() << "\t{";
			for (int k = 0; k < (int)cycles[i].firings[j].loc.tokens.size(); k++)
			{
				if (k != 0)
					cout << " ";
				cout << cycles[i].firings[j].loc.tokens[k].index;
			}
			cout << "} " << export_expression(cycles[i].firings[j].loc.encodings, variables).to_string() << endl;
		}
	}


	/*simulator sim(&g, &variables, g.reset[0]);
	vector<int> indices(variables.nodes.size(), 0);
	map<pair<int, int>, pair<int, bool> > transitions;
	vector<int> record(variables.nodes.size(), -1);
	hse::iterator curr;
	int max_value = 1;

	// find a cycle
	bool done = false;
	while (!done)
	{
		sim.enabled();

		// Get the transition that will change the node that has been changed most recently
		// This will find a cycle quicker
		int index = -1;
		int value = -1;
		vector<int> vindex;
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			vector<int> v = g.transitions[sim.loaded[sim.ready[i].first].index].local_action[sim.ready[i].second].vars();
			int test = -1;
			for (int j = 0; j < (int)v.size(); j++)
				if (indices[v[j]] > test)
					test = indices[v[j]];

			if (test > value)
			{
				index = i;
				value = test;
				vindex = v;
			}
		}*/

		// Get the transition that will change the node that hasn't been changed in the longest amount of time
		// this will be more likely to explore the entire graph
		/*int index = -1;
		int value = max_value;
		vector<int> vindex;
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			vector<int> v = g.transitions[sim.loaded[sim.ready[i].first].index].local_action[sim.ready[i].second].vars();
			int test = max_value;
			for (int j = 0; j < (int)v.size(); j++)
				if (indices[v[j]] < test)
					test = indices[v[j]];

			if (test < value)
			{
				index = i;
				value = test;
				vindex = v;
			}
		}*/

		/*if (index == -1)
			done = true;
		else
		{
			pair<int, int> tid(sim.loaded[sim.ready[index].first].index, sim.ready[index].second);
			cout << "(" << tid.first << " " << tid.second << ") -> ";

			map<pair<int, int>, pair<int, bool> >::iterator loc = transitions.find(tid);
			if (loc != transitions.end())
			{
				curr.type = hse::transition::type;
				curr.index = loc->second.first;
				done = true;
			}
			else
			{
				loc = transitions.insert(pair<pair<int, int>, pair<int, bool> >(tid, pair<int, bool>(result.create(g.transitions[tid.first].subdivide(tid.second)).index, false))).first;
				curr.type = hse::transition::type;
				curr.index = loc->second.first;
			}
			cout << "(" << loc->second.first << " " << loc->second.second << ")\t";
			for (int i = 0; i < (int)vindex.size(); i++)
			{
				if (i != 0)
					cout << ",";
				cout << variables.nodes[vindex[i]].to_string() << (g.transitions[tid.first].local_action[tid.second].get(vindex[i]) == 1 ? "+" : "-");
			}
			cout << endl;

			loc->second.second = true;
			vector<int> v = sim.loaded[sim.ready[index].first].guard_action.vars();
			for (int i = 0; i < (int)v.size(); i++)
			{
				cout << "\t" << variables.nodes[v[i]].to_string() << "->" << record[v[i]] << endl;
				if (record[v[i]] >= 0)
				{
					vector<int> nn = result.next(hse::place::type, result.next(hse::transition::type, record[v[i]]));
					if (find(nn.begin(), nn.end(), curr.index) == nn.end())
						result.connect(hse::iterator(hse::transition::type, record[v[i]]), curr);
				}
				else
					loc->second.second = false;
			}

			for (map<pair<int, int>, pair<int, bool> >::iterator i = transitions.begin(); i != transitions.end() && done; i++)
				done = i->second.second;

			for (int i = 0; i < (int)vindex.size(); i++)
			{
				record[vindex[i]] = curr.index;
				indices[vindex[i]]++;
				if (indices[vindex[i]] >= max_value)
					max_value = indices[vindex[i]]+1;
			}
			sim.fire(index);
		}
	}*/

	return result;
}

}
