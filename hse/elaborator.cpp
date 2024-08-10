/*
 * elaborator.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: nbingham
 */

#include "elaborator.h"
#include <common/text.h>
#include <common/standard.h>
#include <interpret_boolean/export.h>

namespace hse
{

// Do an exhaustive simulation of all of the states in the state space. Record
// the state encodings observed during that simulation into the places in the
// HSE graph.
// 
// This function prunes states to optimize execution time by recognizing
// already visited states. It first does a depth first search, recognizing that
// the state space is a large cycle. This means that the first simulation will
// go as far as possible around that cycle and every successive simulation will
// branch off and recombine. This also keeps the amount of memory required as
// low as possible as it fully explores branches before finding new ones.
void elaborate(graph &g, const ucs::variable_set &variables, bool report_progress)
{
	// Initialize all predicates and effective predicates to false.
	for (int i = 0; i < (int)g.places.size(); i++)
	{
		g.places[i].predicate = boolean::cover();
		g.places[i].effective = boolean::cover();
	}

	// used to trim the simulation tree by identifying revisited simulation states.
	hashtable<state, 10000> states;

	// the set of currently running simulations
	vector<simulator> simulations;
	simulations.reserve(2000);

	// all error states found are stored here.
	vector<deadlock> deadlocks;

	// initialize the list of current simulations with reset
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
	// if there isn't a reset state, look for places with only outgoing arcs.
	// TODO(edward.bingham) This probably won't work because most processes are cycles.
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

	// this is a depth-first search of all states reachable from reset.
	int count = 0;
	while (simulations.size() > 0)
	{
		// grab the simulation at the top of the stack
		simulator sim = simulations.back();
		simulations.pop_back();
		//simulations.back().merge_errors(sim);

		if (report_progress)
			progress("", ::to_string(count) + " " + ::to_string(simulations.size()) + " " + ::to_string(states.max_bucket_size()) + "/" + ::to_string(states.count) + " " + ::to_string(sim.ready.size()), __FILE__, __LINE__);

		// Create a new simulation for each enabled transition in the current
		// simulation and push it to the end of the stack as long as we haven't
		// seen that state before
		simulations.reserve(simulations.size() + sim.ready.size());
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			simulations.push_back(sim);

			// fire the enabled transition
			simulations.back().fire(i);

			// compute the new enabled transitions
			simulations.back().enabled();

			if (!states.insert(simulations.back().get_state()))
				simulations.pop_back();
		}

		// If there aren't any enabled transitions, then record a deadlock state.
		if (sim.ready.size() == 0)
		{
			deadlock d = sim.get_state();
			vector<deadlock>::iterator dloc = lower_bound(deadlocks.begin(), deadlocks.end(), d);
			if (dloc == deadlocks.end() || *dloc != d)
			{
				error("", d.to_string(variables), __FILE__, __LINE__);
				deadlocks.insert(dloc, d);
			}
		}

		// The effective predicate represents the state encodings that don't have
		// duplicates in later states. I have to loop through all of the enabled
		// transitions, and then loop through all orderings of their dependent
		// guards, saving the state
		//vector<set<int> > en_in(sim.tokens.size(), set<int>());
		vector<bool> en_in(sim.tokens.size(), false);
		vector<set<int> > en_out(sim.tokens.size(), set<int>());

		/*bool change = true;
		while (change) {
			change = false;
			for (int i = 0; i < (int)sim.loaded.size(); i++) {
				set<int> total_in;
				for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++) {
					total_in.insert(en_in[sim.loaded[i].tokens[j]].begin(), en_in[sim.loaded[i].tokens[j]].end());
				}
				total_in.insert(sim.loaded[i].index);

				for (int j = 0; j < (int)sim.loaded[i].output_marking.size(); j++) {
					set<int> old_in = en_in[sim.loaded[i].output_marking[j]];
					en_in[sim.loaded[i].output_marking[j]].insert(total_in.begin(), total_in.end());
					change = change or (en_in[sim.loaded[i].output_marking[j]] != old_in);
				}
			}
		}*/

		for (int i = 0; i < (int)sim.loaded.size(); i++)
			for (int j = 0; j < (int)sim.loaded[i].tokens.size(); j++)
				en_out[sim.loaded[i].tokens[j]].insert(i);

		for (int i = 0; i < (int)sim.loaded.size(); i++)
			for (int j = 0; j < (int)sim.loaded[i].output_marking.size(); j++)
				if (not sim.loaded[i].vacuous)
					en_in[sim.loaded[i].output_marking[j]] = true;

		for (int i = 0; i < (int)sim.tokens.size(); i++) {
			bool isOutput = en_out[i].empty() and en_in[i];

			bool isVacuous = false;
			for (set<int>::iterator j = en_out[i].begin(); j != en_out[i].end() and not isVacuous; j++) {
				isVacuous = sim.loaded[*j].vacuous;
			}
	
			if (not isVacuous and not isOutput) {
				// TODO(edward.bingham) only save this state to this place if there is a non-vacuous outgoing transition from this token
				//boolean::cover en = 1;
				boolean::cover dis = 1;
				//for (set<int>::iterator j = en_in[i].begin(); j != en_in[i].end(); j++)
				//	en &= g.transitions[*j].local_action;
				// Not guard because then we'd be in the hidden place in the transition
				// and not action because then we would have passed this transtion entirely
				// whether or not the current encoding passes the guard
				for (set<int>::iterator j = en_out[i].begin(); j != en_out[i].end(); j++)
					dis &= ~g.transitions[sim.loaded[*j].index].guard;

				// Given the current encoding - sim.encoding
				// 1. Ignore unstable signals - xoutnulls()
				// 2. Mask out variables that this process has no visibility for - flipped_mask()
				// 3. OR this into the predicate for that place
				g.places[sim.tokens[i].index].predicate |= (sim.encoding.xoutnulls()).flipped_mask(g.places[sim.tokens[i].index].mask);
				// Same thing as above, but we exclude any state encoding that passes an outgoing guard - & dis
				g.places[sim.tokens[i].index].effective |= (sim.encoding.xoutnulls() & dis).flipped_mask(g.places[sim.tokens[i].index].mask);
			}
		}

		count++;
	}

	if (report_progress)
		done_progress();

	// Clean up the recorded state with Espresso. This will help when rendering
	// the state information to the user and when synthesizing production rules
	// for the final circuit.
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

// This converts a given graph to the fully expanded state space through simulation. It systematically
// simulates all possible transition orderings and determines all of the resulting state information.
graph to_state_graph(graph &g, const ucs::variable_set &variables, bool report_progress)
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

bool operator<(firing f0, firing f1)
{
	return ((f0.action < f1.action) ||
			(f0.action == f1.action && ((f0.guard < f1.guard) ||
										(f0.guard == f1.guard && f0.loc < f1.loc))));
}

bool operator==(firing f0, firing f1)
{
	return f0.guard == f1.guard && f0.action == f1.action && f0.loc == f1.loc;
}

struct cycle
{
	vector<firing> firings;
};

bool operator<(cycle f0, cycle f1)
{
	return f0.firings.size() < f1.firings.size();
}

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
		indices.resize(sim.variables->nodes.size(), 0);
	}
	~frame() {}

	cycle part;
	simulator sim;
	vector<int> indices;
};

cycle get_cycle(graph &g, simulator &sim)
{
	cycle result;
	vector<int> indices(sim.variables->nodes.size(), 0);

	bool done = false;
	while (!done)
	{
		sim.enabled();
		int index = -1;
		int value = -1;
		for (int i = 0; i < (int)sim.ready.size(); i++)
		{
			int test = -1;
			vector<int> tvars = g.transitions[sim.loaded[sim.ready[i].first].index].local_action[sim.ready[i].second].vars();
			for (int j = 0; j < (int)tvars.size(); j++)
				if (indices[tvars[j]] > test)
					test = indices[tvars[j]];

			if (test > value)
			{
				index = i;
				value = test;
			}
		}

		firing t;
		t.guard = sim.loaded[sim.ready[index].first].guard_action;
		t.action = term_index(sim.loaded[sim.ready[index].first].index, sim.ready[index].second);
		t.loc = sim.get_state();

		if (find(result.firings.begin(), result.firings.end(), t) == result.firings.end())
			result.firings.push_back(t);
		else
			done = true;

		sim.fire(index);
	}

	return result;
}

vector<cycle> get_cycles(graph &g, const ucs::variable_set &variables, bool report_progress)
{
	vector<cycle> result;
	list<frame> frames;
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

		/*vector<int> indices(variables.nodes.size(), 0);
		for (list<frame>::iterator i = frames.begin(); i != frames.end(); i++)
			for (int j = 0; j < (int)i->indices.size(); j++)
				indices[j] += i->indices[j];*/

		//cout << "Iteration " << iteration << ": " << frames.size() << " frames left" << endl;
		frame curr = frames.front();
		frames.pop_front();

		/*for (int i = 0; i < (int)curr.indices.size(); i++)
			cout << curr.indices[i] << " ";
		cout << endl;*/

		curr.sim.enabled();
		vector<pair<int, int> > choices = curr.sim.get_choices();


		/*cout << "Enabled Assignments: ";
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
		*/

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
		int chosen = -1;
		while (excluded.size() > 0 && tindices.size() > 0)
		{
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

			/*cout << "Indices:" << endl;
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
			}*/

			// find the next tindex that we haven't already fired.
			vector<int> index = tindices.back();

			// TODO check to make sure this assumption about minimal cycles with "chosen" actually holds true
			if (index.size() == 3 && index[0] >= chosen)
			{
				chosen = index[0];

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
	}

	return result;
}

struct pnode
{
	pnode()
	{
		v = -1;
		i = -1;
		d = -1;
	}
	~pnode(){}

	int v, i, d;

	string to_string(const ucs::variable_set &variables) const
	{
		return variables.nodes[v].to_string() + (d == 0 ? "-" : "+") + ":" + ::to_string(i);
	}
};

bool operator<(pnode a, pnode b)
{
	return (a.i < b.i ||
			(a.i == b.i && (a.v < b.v ||
			(a.v == b.v && (a.d < b.d)))));
}

bool operator==(pnode a, pnode b)
{
	return a.v == b.v && a.i == b.i && a.d == b.d;
}

struct parc
{
	parc() {}
	parc(pnode a, pnode b) {n0 = a; n1 = b;}
	~parc() {}

	pnode n0;
	pnode n1;

	string to_string(const ucs::variable_set &variables) const
	{
		return n0.to_string(variables) + " -> " + n1.to_string(variables);
	}
};

bool operator<(parc a, parc b)
{
	return (a.n1 < b.n1 ||
			(a.n1 == b.n1 && (a.n0 < b.n0)));
}

bool operator==(parc a, parc b)
{
	return a.n0 == b.n0 && a.n1 == b.n1;
}



// Converts the HSE into a petri net using index-priority simulation.
graph to_petri_net(graph &g, const ucs::variable_set &variables, bool report_progress)
{
	graph result;

	vector<cycle> cycles = get_cycles(g, variables, report_progress);
	/*
	for (int i = 0; i < (int)cycles.size(); i++)
		for (int j = 0; j < (int)cycles[i].firings.size(); j++)
			cycles[i].firings[j].guard &= g.transitions[cycles[i].firings[j].action.index].local_action.cubes[cycles[i].firings[j].action.term].inverse();
	*/

	sort(cycles.begin(), cycles.end());
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
			cout << "}" << endl;
		}
	}

	vector<vector<parc> > arcs;
	for (int i = 0; i < (int)cycles.size(); i++)
	{
		vector<parc> cycle_arcs;
		map<pair<int, int>, int> cycle_nodes;
		int iteration = 0;
		for (int j = 0; j < (int)cycles[i].firings.size(); j++)
		{
			boolean::cube guard = cycles[i].firings[j].guard;
			boolean::cube term = g.transitions[cycles[i].firings[j].action.index].local_action[cycles[i].firings[j].action.term];
			vector<int> gv = guard.vars();
			vector<int> fv = term.vars();
			vector<pnode> gn;
			vector<pnode> fn;
			for (int k = 0; k < (int)gv.size(); k++)
			{
				pnode n;
				n.v = gv[k];
				n.d = guard.get(gv[k]);
				map<pair<int, int>, int>::iterator loc = cycle_nodes.find(pair<int, int>(n.v, n.d));
				if (loc == cycle_nodes.end())
					n.i = -1;
				else
					n.i = loc->second;
				gn.push_back(n);
			}

			for (int k = 0; k < (int)fv.size(); k++)
			{
				pnode n;
				n.v = fv[k];
				n.d = term.get(fv[k]);
				n.i = iteration;
				map<pair<int, int>, int>::iterator loc = cycle_nodes.find(pair<int, int>(n.v, n.d));
				if (loc == cycle_nodes.end())
					cycle_nodes.insert(pair<pair<int, int>, int>(pair<int, int>(n.v, n.d), iteration));
				else if (loc->second == iteration)
				{
					loc->second++;
					iteration++;
					n.i++;
				}
				else if (loc->second < iteration)
					loc->second = iteration;

				fn.push_back(n);
			}

			for (int k = 0; k < (int)gn.size(); k++)
				for (int l = 0; l < (int)fn.size(); l++)
					cycle_arcs.push_back(parc(gn[k], fn[l]));
		}

		for (int j = (int)cycle_arcs.size()-1; j >= 0; j--)
		{
			if (cycle_arcs[j].n0.i < 0)
			{
				map<pair<int, int>, int>::iterator loc = cycle_nodes.find(pair<int, int>(cycle_arcs[j].n0.v, cycle_arcs[j].n0.d));
				if (loc != cycle_nodes.end())
					cycle_arcs[j].n0.i = loc->second;
				// If there is no node in this cycle that can feed this
				// node, that means that transition must have been vacuous
				// this is the only time that a pair of parallel transitions
				// are made conditional. So we just erase that requirment
				else
				{
					cycle_arcs.erase(cycle_arcs.begin() + j);
					continue;
				}
			}

			// This should never happen because every 'to' node in every arc
			// has been executed by definition.
			if (cycle_arcs[j].n1.i < 0)
			{
				map<pair<int, int>, int>::iterator loc = cycle_nodes.find(pair<int, int>(cycle_arcs[j].n1.v, cycle_arcs[j].n1.d));
				if (loc != cycle_nodes.end())
					cycle_arcs[j].n1.i = loc->second;
				else
				{
					cycle_arcs.erase(cycle_arcs.begin() + j);
					continue;
				}
			}
		}
		sort(cycle_arcs.begin(), cycle_arcs.end());
		arcs.push_back(cycle_arcs);
	}

	// Add the transitions to the graph
	map<pnode, int> nodes;
	for (int i = 0; i < (int)arcs.size(); i++)
	{
		for (int j = 0; j < (int)arcs[i].size(); j++)
		{
			map<pnode, int>::iterator loc = nodes.find(arcs[i][j].n0);
			if (loc == nodes.end())
			{
				nodes.insert(pair<pnode, int>(arcs[i][j].n0, result.transitions.size()));
				result.transitions.push_back(transition(1, boolean::cover(arcs[i][j].n0.v, arcs[i][j].n0.d)));
			}

			loc = nodes.find(arcs[i][j].n1);
			if (loc == nodes.end())
			{
				nodes.insert(pair<pnode, int>(arcs[i][j].n1, result.transitions.size()));
				result.transitions.push_back(transition(1, boolean::cover(arcs[i][j].n1.v, arcs[i][j].n1.d)));
			}
		}
	}

	// I need to pick the conditional groupings of arcs in a bipartide graph that
	// cover all of the conditions with the fewest number of arcs and repeat.
	// This is a form of greedy algorithm that maps to the knapsack problem.

	// each group represents a bipartide graph
	// each pair in each group indexes to a particular arc in the arcs[][] array
	// groups[i][j].first is the cycle
	// groups[i][j].second is the arc
	vector<map<parc, vector<int> > > groups;
	for (int i = 0; i < (int)arcs.size(); i++)
		for (int j = 0; j < (int)arcs[i].size(); j++)
		{
			vector<int> found;
			for (int k = 0; k < (int)groups.size(); k++)
				for (map<parc, vector<int> >::iterator l = groups[k].begin(); l != groups[k].end() && (found.size() == 0 || found.back() != k); l++)
					if (arcs[i][j].n0 == l->first.n0 ||
						arcs[i][j].n1 == l->first.n1)
						found.push_back(k);

			if (found.size() > 0)
			{
				sort(found.rbegin(), found.rend());
				for (int k = 0; k < (int)found.size()-1; k++)
				{
					for (map<parc, vector<int> >::iterator l = groups[found[k]].begin(); l != groups[found[k]].end(); l++)
					{
						map<parc, vector<int> >::iterator loc = groups[found.back()].find(l->first);
						if (loc == groups[found.back()].end())
							groups[found.back()].insert(*l);
						else
							loc->second.insert(loc->second.end(), l->second.begin(), l->second.end());
					}
					groups.erase(groups.begin() + found[k]);
				}

				map<parc, vector<int> >::iterator loc = groups[found.back()].find(arcs[i][j]);
				if (loc == groups[found.back()].end())
					groups[found.back()].insert(pair<parc, vector<int> >(arcs[i][j], vector<int>(1, i)));
				else
					loc->second.push_back(i);
			}
			else
			{
				groups.resize(groups.size()+1);
				groups.back().insert(pair<parc, vector<int> >(arcs[i][j], vector<int>(1, i)));
			}
		}

	for (int i = 0; i < (int)arcs.size(); i++)
	{
		cout << "Cycle " << i << endl;
		for (int j = 0; j < (int)arcs[i].size(); j++)
			cout << "Arc " << j << ": " << arcs[i][j].to_string(variables) << endl;
	}

	vector<vector<parc> > choices;
	while (groups.size() > 0)
	{
		vector<parc> selected;
		vector<int> covered;
		vector<int> missing;
		for (map<parc, vector<int> >::iterator j = groups[0].begin(); j != groups[0].end(); j++)
			missing.insert(missing.end(), j->second.begin(), j->second.end());
		sort(missing.begin(), missing.end());
		missing.resize(unique(missing.begin(), missing.end()) - missing.begin());

		while (missing.size() > 0)
		{
			cout << to_string(missing) << " " << to_string(covered) << endl;
			map<parc, vector<int> >::iterator loc = groups[0].end();
			int count = 0;

			for (map<parc, vector<int> >::iterator j = groups[0].begin(); j != groups[0].end(); j++)
				if (count < (int)j->second.size() && !vector_intersects(covered, j->second))
				{
					loc = j;
					count = j->second.size();
				}

			if (loc != groups[0].end())
			{
				cout << loc->first.to_string(variables) << endl;
				selected.push_back(loc->first);
				missing = vector_difference(missing, loc->second);
				covered.insert(covered.end(), loc->second.begin(), loc->second.end());
				sort(covered.begin(), covered.end());
			}
		}
		cout << to_string(missing) << " " << to_string(covered) << endl;

		cout << "Group: {";
		for (map<parc, vector<int> >::iterator j = groups[0].begin(); j != groups[0].end(); j++)
		{
			if (j != groups[0].begin())
				cout << " ";
			cout << "(" << j->first.to_string(variables) << " " << to_string(j->second) << ")";
		}
		cout << "}" << endl;

		cout << "Selection: {";
		for (int j = 0; j < (int)selected.size(); j++)
		{
			if (j != 0)
				cout << ", ";
			cout << "(" << selected[j].to_string(variables) << ")";
		}
		cout << "}" << endl;

		choices.push_back(selected);
		for (int j = 0; j < (int)selected.size(); j++)
		{
			map<parc, vector<int> >::iterator k = groups[0].find(selected[j]);
			if (k != groups[0].end())
				groups[0].erase(k);
		}

		// Now that we've taken some of the arcs out of the group, the group may not be
		// fully connected anymore. So we have to split it up and redistribute
		map<parc, vector<int> > group = groups[0];
		groups.erase(groups.begin());
		for (map<parc, vector<int> >::iterator j = group.begin(); j != group.end(); j++)
		{
			vector<int> found;
			for (int k = 0; k < (int)groups.size(); k++)
				for (map<parc, vector<int> >::iterator l = groups[k].begin(); l != groups[k].end() && (found.size() == 0 || found.back() != k); l++)
					if (j->first.n0 == l->first.n0 ||
						j->first.n1 == l->first.n1)
						found.push_back(k);

			if (found.size() > 0)
			{
				sort(found.rbegin(), found.rend());
				for (int k = 0; k < (int)found.size()-1; k++)
				{
					for (map<parc, vector<int> >::iterator l = groups[found[k]].begin(); l != groups[found[k]].end(); l++)
					{
						map<parc, vector<int> >::iterator loc = groups[found.back()].find(l->first);
						if (loc == groups[found.back()].end())
							groups[found.back()].insert(*l);
						else
						{
							loc->second.insert(loc->second.end(), l->second.begin(), l->second.end());
							sort(loc->second.begin(), loc->second.end());
						}
					}
					groups.erase(groups.begin() + found[k]);
				}

				map<parc, vector<int> >::iterator loc = groups[found.back()].find(j->first);
				if (loc == groups[found.back()].end())
					groups[found.back()].insert(*j);
				else
				{
					loc->second.insert(loc->second.end(), j->second.begin(), j->second.end());
					sort(loc->second.begin(), loc->second.end());
				}
			}
			else
			{
				groups.resize(groups.size()+1);
				groups.back().insert(*j);
			}
		}
	}

	for (int i = 0; i < (int)choices.size(); i++)
	{
		hse::iterator p(hse::place::type, result.places.size());
		result.places.push_back(place());
		cout << "Choice " << i << ": {";
		for (int j = 0; j < (int)choices[i].size(); j++)
		{
			map<pnode, int>::iterator t0 = nodes.find(choices[i][j].n0);
			if (t0 != nodes.end())
			{
				petri::arc a0(petri::iterator(transition::type, t0->second), p);
				if (find(result.arcs[transition::type].begin(), result.arcs[transition::type].end(), a0) == result.arcs[transition::type].end())
					result.arcs[transition::type].push_back(a0);
			}

			map<pnode, int>::iterator t1 = nodes.find(choices[i][j].n1);
			if (t1 != nodes.end())
			{
				petri::arc a1(p, petri::iterator(transition::type, t1->second));
				if (find(result.arcs[place::type].begin(), result.arcs[place::type].end(), a1) == result.arcs[place::type].end())
					result.arcs[place::type].push_back(a1);
			}

			if (j != 0)
				cout << ", ";
			cout << "(" << choices[i][j].to_string(variables) << ")";
		}
		cout << "}" << endl;
	}

	// Because of the way the cycles are merged we might need connections between the up and down going transitions of a variable
	// So we need to check if those dependencies are guaranteed somehow.
	vector<pair<vector<int>, int> > con;
	for (int i = 0; i < (int)result.transitions.size(); i++)
	{
		vector<pair<vector<int>, vector<int> > > tokens(1, pair<vector<int>, vector<int> >(result.prev(hse::transition::type, i), vector<int>()));
		vector<int> joint;

		bool loop = false;
		while (tokens.size() > 0)
		{
			vector<int> curr = tokens.back().first;
			vector<int> cov = tokens.back().second;
			cov.insert(cov.end(), curr.begin(), curr.end());
			sort(cov.begin(), cov.end());
			cov.resize(unique(cov.begin(), cov.end()) - cov.begin());
			tokens.pop_back();

			vector<vector<int> > ttoken(1, vector<int>());

			for (int j = 0; j < (int)curr.size(); j++)
			{
				int idx = (int)ttoken.size();
				vector<int> prev = result.prev(hse::place::type, curr[j]);
				for (int k = (int)prev.size()-1; k >= 0; k--)
				{
					if (k != 0)
					{
						ttoken.insert(ttoken.end(), ttoken.begin(), ttoken.begin() + idx);
						for (int l = (int)ttoken.size()-idx; l < (int)ttoken.size(); l++)
							ttoken[l].push_back(prev[k]);
					}
					else
						for (int l = 0; l < idx; l++)
							ttoken[l].push_back(prev[k]);
				}
			}

			for (int j = 0; j < (int)ttoken.size(); j++)
			{
				bool found = false;
				for (int k = 0; k < (int)ttoken[j].size(); k++)
				{
					if (are_mutex(result.transitions[ttoken[j][k]].local_action, result.transitions[i].local_action))
					{
						found = true;
						joint.push_back(ttoken[j][k]);
					}
					else if (are_mutex(~result.transitions[ttoken[j][k]].local_action, result.transitions[i].local_action))
						loop = true;
				}

				if (!found && !loop)
				{
					tokens.push_back(pair<vector<int>, vector<int> >(result.prev(hse::transition::type, ttoken[j]), cov));
					sort(tokens.back().first.begin(), tokens.back().first.end());
					tokens.back().first.resize(unique(tokens.back().first.begin(), tokens.back().first.end()) - tokens.back().first.begin());
					tokens.back().first = vector_difference(tokens.back().first, tokens.back().second);
					if (tokens.back().first.size() == 0)
						tokens.pop_back();
				}
			}
		}

		if (loop)
		{
			sort(joint.begin(), joint.end());
			joint.resize(unique(joint.begin(), joint.end()) - joint.begin());
			con.push_back(pair<vector<int>, int>(joint, i));
			cout << "Found Loop " << export_composition(result.transitions[i].local_action, variables).to_string() << endl;
		}
	}

	sort(con.begin(), con.end());
	con.resize(unique(con.begin(), con.end()) - con.begin());
	for (int i = 0; i < (int)con.size(); i++)
	{
		cout << "{";
		for (int j = 0; j < (int)con[i].first.size(); j++)
		{
			if (j != 0)
				cout << " ";
			cout << export_composition(result.transitions[con[i].first[j]].local_action, variables).to_string();
		}
		cout << "} => " << export_composition(result.transitions[con[i].second].local_action, variables).to_string() << endl;
		int p = (int)result.places.size();
		result.places.push_back(place());
		for (int j = 0; j < (int)con[i].first.size(); j++)
			result.connect(petri::iterator(petri::transition::type, con[i].first[j]), petri::iterator(petri::place::type, p));
		result.connect(hse::iterator(hse::place::type, p), hse::iterator(hse::transition::type, con[i].second));
	}

	// Now we need to place the initial marking
	vector<int> reset;
	for (int i = 0; i < (int)result.transitions.size(); i++)
		if (are_mutex(~result.transitions[i].local_action, g.reset[0].encodings))
			reset.push_back(i);

	sort(reset.begin(), reset.end());
	reset.resize(unique(reset.begin(), reset.end()) - reset.begin());

	vector<int> mark = result.next(hse::transition::type, reset);
	vector<int> pmark = result.prev(hse::transition::type, reset);
	sort(mark.begin(), mark.end());
	mark.resize(unique(mark.begin(), mark.end()) - mark.begin());
	sort(pmark.begin(), pmark.end());
	pmark.resize(unique(pmark.begin(), pmark.end()) - pmark.begin());
	mark = vector_difference(mark, pmark);

	vector<petri::token> rmark;
	for (int i = 0; i < (int)mark.size(); i++)
		rmark.push_back(petri::token(mark[i]));

	result.reset.push_back(state(rmark, g.reset[0].encodings));

	return result;
}

}
