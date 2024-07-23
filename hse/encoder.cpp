/*
 * encoder.cpp
 *
 *  Created on: May 7, 2015
 *      Author: nbingham
 */

#include "encoder.h"
#include "graph.h"

#include <petri/path.h>
#include <interpret_boolean/export.h>

namespace hse
{

conflict::conflict()
{
	sense = -1;
}

conflict::conflict(term_index index, int sense, vector<petri::iterator> region, boolean::cover encoding)
{
	this->sense = sense;
	this->index = index;
	this->region = region;
	this->encoding = encoding;
}

conflict::~conflict()
{

}

suspect::suspect()
{
	sense = -1;
}

suspect::suspect(int sense, vector<petri::iterator> first, vector<petri::iterator> second)
{
	this->sense = sense;
	this->first = first;
	this->second = second;
}

suspect::~suspect()
{

}

encoder::encoder()
{
	base = NULL;
}

encoder::encoder(graph *base)
{
	this->base = base;
}

encoder::~encoder()
{

}

void encoder::add_conflict(int tid, int term, int sense, petri::iterator node, boolean::cover encoding)
{
	// cluster the conflicting places into regions. We want to be able to
	// eliminate entire regions of these conflicts with a single state variable.

	// get the input and output transitions of the place we are comparing against
	vector<petri::iterator> neighbors = base->neighbors(node, true);

	vector<int> merge;
	for (int i = 0; i < (int)conflicts.size(); i++) {
		if (conflicts[i].index.index == tid &&
				conflicts[i].index.term == term &&
				conflicts[i].sense == sense && 
				vector_intersection_size(neighbors, conflict_regions[i]) > 0) {
			merge.push_back(i);
		}
	}

	if (merge.size() > 0) {
		for (int i = 1; i < (int)merge.size(); i++) {
			conflicts[merge[0]].region.insert(
					conflicts[merge[0]].region.end(),
					conflicts[merge[i]].region.begin(),
					conflicts[merge[i]].region.end());
			conflict_regions[merge[0]].insert(
					conflict_regions[merge[0]].end(),
					conflict_regions[merge[i]].begin(),
					conflict_regions[merge[i]].end());
		}

		for (int i = (int)merge.size()-1; i > 0; i--) {
			conflicts.erase(conflicts.begin() + merge[i]);
			conflict_regions.erase(conflict_regions.begin() + merge[i]);
		}

		conflicts[merge[0]].region.push_back(node);
		conflicts[merge[0]].encoding |= encoding;
		conflict_regions[merge[0]].insert(conflict_regions[merge[0]].end(), neighbors.begin(), neighbors.end());
		sort(conflict_regions[merge[0]].begin(), conflict_regions[merge[0]].end());
	} else {
		conflicts.push_back(conflict(term_index(tid, term), sense, vector<petri::iterator>(1, node), encoding));
		conflict_regions.push_back(neighbors);
	}
}

void encoder::add_suspect(vector<petri::iterator> i, petri::iterator j, int sense)
{
	// cluster the suspected places into regions. We want to be able to
	// eliminate entire regions of these conflicts with a single state variable.

	// get the input and output transitions of the place we are comparing against
	vector<petri::iterator> neighbors = base->neighbors(j, true);

	vector<int> merge;
	for (int l = 0; l < (int)suspects.size(); l++) {
		if (suspects[l].first == i && suspects[l].sense == sense && vector_intersection_size(neighbors, suspect_regions[l]) > 0) {
			merge.push_back(l);
		}
	}

	if (merge.size() > 0) {
		for (int l = 1; l < (int)merge.size(); l++) {
			suspects[merge[0]].second.insert(suspects[merge[0]].second.end(), suspects[merge[l]].second.begin(), suspects[merge[l]].second.end());
			suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), suspect_regions[merge[l]].begin(), suspect_regions[merge[l]].end());
		}

		for (int l = (int)merge.size()-1; l > 0; l--) {
			suspects.erase(suspects.begin() + merge[l]);
			suspect_regions.erase(suspect_regions.begin() + merge[l]);
		}

		suspects[merge[0]].second.push_back(j);
		suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), neighbors.begin(), neighbors.end());
		sort(suspect_regions[merge[0]].begin(), suspect_regions[merge[0]].end());
	} else {
		suspects.push_back(suspect(sense, i, vector<petri::iterator>(1, j)));
		suspect_regions.push_back(neighbors);
	}
}

void encoder::check(ucs::variable_set &variables, bool senseless, bool report_progress)
{
	cout << "Computing Parallel Groups" << endl;
	base->compute_split_groups(parallel);
	cout << "Parallel Groups" << endl;
	for (int i = 0; i < (int)base->places.size(); i++) {
		cout << "p" << i << " {" << endl;
		for (int j = 0; j < (int)base->places[i].groups[parallel].size(); j++) {
			cout << "\t" << base->places[i].groups[parallel][j].to_string() << endl;
		}
		cout << "}" << endl;
	}

	for (int i = 0; i < (int)base->transitions.size(); i++) {
		cout << "t" << i << " {" << endl;
		for (int j = 0; j < (int)base->transitions[i].groups[parallel].size(); j++) {
			cout << "\t" << base->transitions[i].groups[parallel][j].to_string() << endl;
		}
		cout << "}" << endl;
	}


	if (base == NULL)
		return;

	conflict_regions.clear();
	suspect_regions.clear();
	conflicts.clear();
	suspects.clear();

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (auto t0i = base->transitions.begin(); t0i != base->transitions.end(); t0i++) {
		petri::iterator t0(petri::transition::type, t0i - base->transitions.begin());

		if (report_progress)
			progress("", "T" + ::to_string(t0.index) + "/" + ::to_string(base->transitions.size()), __FILE__, __LINE__);

		// The transition actively affects the state of the system
		if (t0i->local_action.is_tautology())
			continue;

		vector<petri::iterator> t0_prev;
		boolean::cover predicate = base->predicate(t0, &t0_prev);
		boolean::cover implicant = predicate & t0i->guard;
		vector<petri::iterator> relevant = base->relevant_nodes(vector<petri::iterator>(1, t0));

		for (int term = 0; term < (int)t0i->local_action.cubes.size(); term++) {
			boolean::cube action = t0i->local_action.cubes[term];
			boolean::cover not_action = ~action;

			for (int sense = senseless ? -1 : 0; senseless ? sense == -1 : sense < 2; sense++) {
				// If we aren't specifically checking senses or this transition affects the senses we are checking
				if (!senseless && action.mask(1-sense).is_tautology()) {
					continue;
				}

				// we cannot use the variables affected by the transitions in their rules because that would
				// make the rules self-invalidating, so we have to mask them out.
				boolean::cover term_implicant = (implicant & not_action).mask(action.mask()).mask(sense);

				for (auto i = relevant.begin(); i != relevant.end(); i++) {
					// Get the list of places that are suspect against parallel merges
					// A place is suspect if making it an implicant for an active transition
					// could make it a conflict.
					if (t0_prev.size() > 1 && !are_mutex(predicate.mask(sense), base->effective(*i))) {
						add_suspect(t0_prev, *i, sense);
					}

					// Get only the state encodings for the place for which the transition is invacuous and
					// there is no other vacuous transition that would take a token off the place.
					boolean::cover encoding = base->effective_implicant(*i);

					// The implicant states for any actions that would have the same effect as the transition we're checking
					// can be removed from our check because "it could have fired there anyways"
					// If all terms cover all of the assignments in t0, then it can't conflict
					if (i->type == petri::transition::type &&
							base->transitions[i->index].local_action.is_subset_of(action)) {
						continue;
					}

					// The state encodings for the implicant set of states of the transition.
					// these are the state encodings for which there is no vacuous transition that would
					// take a token off of any of the places.
					// Get only the state encodings for the place for which the transition is invacuous and
					// there is no other vacuous transition that would take a token off the place.

					// Check if they share a common state encoding given the above checks
					encoding &= not_action;

					//cout << "here " << t0 << " -> " << *i << endl;
					//cout << export_expression(term_implicant, variables).to_string() << endl;
					//cout << export_expression(encoding, variables).to_string() << endl;
					if (!are_mutex(term_implicant, encoding)) {
						add_conflict(t0.index, term, sense, *i, term_implicant & encoding);
					}
				}
			}
		}
	}

	// get the list of places that are suspect against other places
	for (auto p0 = base->begin(petri::place::type); p0 != base->end(petri::place::type); p0++) {
		boolean::cover p0_predicate = base->predicate(p0);

		vector<petri::iterator> relevant = base->relevant_nodes(vector<petri::iterator>(1, p0));
		for (auto p1 = relevant.begin(); p1 != relevant.end(); p1++) {
			if (report_progress) {
				progress("", "P" + ::to_string(p0.index) + "->P" + ::to_string(p1->index), __FILE__, __LINE__);
			}

			boolean::cover p1_effective = base->effective(*p1);
		
			for (int sense = senseless ? -1 : 0; senseless ? sense == -1 : sense < 2; sense++) {
				// Check the implicant predicate against the other place's effective.
				// The predicate is used because inserting a state variable
				// transition will negate any "you might as well be here" effects.
				if (!are_mutex(p0_predicate.mask(sense), p1_effective)) {
					add_suspect(vector<petri::iterator>(1, p0), *p1, sense);
				}
			}
		}
	}

	if (report_progress)
		done_progress();
}


// TODO State Variable Insertion
//
// Our goal is to disambiguate state encodings to prevent transitions from
// firing in states they shouldn't (non-implicant states). We do this by
// inserting new assignments on new variables that break up the state space.
// 
// The following citations describe the problem further:
// 
// Martin, Alain J. "Compiling communicating processes into delay-insensitive
// VLSI circuits." Distributed computing 1 (1986): 226-234.
//
// Cortadella, Jordi, et al. "Complete state encoding based on the theory of
// regions." Proceedings Second International Symposium on Advanced Research in
// Asynchronous Circuits and Systems. IEEE, 1996.
//
// Lecture 16 of github.com/broccolimicro/course-self-timed-circuits/tree/summer-2023
void encoder::insert_state_variables(ucs::variable_set &variables) {
	// Trace all conflicts

	// index 0 indicates that this state variable transition needs to be downgoing
	// index 1 indicates that this state variable transition needs to be upgoing
	struct CodingProblem {
		boolean::cube term;
		array<petri::path_set, 2> traces;
	};
	vector<CodingProblem> problems;
	for (int i = 0; i < (int)conflicts.size(); i++) {
		vector<petri::iterator> from(conflicts[i].region);
		vector<petri::iterator> to(1, petri::iterator(transition::type, conflicts[i].index.index));
		
		if (conflicts[i].sense == conflict::UP) {
			problems.push_back(CodingProblem{
				base->transitions[conflicts[i].index.index].local_action[conflicts[i].index.term].mask(conflict::DOWN), {
				petri::trace(*base, to, from),
				petri::trace(*base, from, to)
			}});
		} else {
			problems.push_back(CodingProblem{
				base->transitions[conflicts[i].index.index].local_action[conflicts[i].index.term].mask(conflict::UP), {
				petri::trace(*base, from, to),
				petri::trace(*base, to, from)
			}});
		}

		// Remove vacuous traces
		for (auto j = problems.back().traces[1-conflicts[i].sense].paths.begin(); j != problems.back().traces[1-conflicts[i].sense].paths.end(); ) {
			bool found = false;
			for (auto t = base->begin(transition::type); t != base->end(transition::type); t++) {
				if ((*j)[t] > 0 and are_mutex(base->transitions[t.index].local_action, problems.back().term)) {
					found = true;
					break;
				}
			}
			if (not found) {
				j = problems.back().traces[1-conflicts[i].sense].paths.erase(j);
			} else {
				j++;
			}
		}
		problems.back().traces[conflicts[i].sense].repair();
	}

	cout << "Before Clustering" << endl;
	for (int i = 0; i < (int)problems.size(); i++) {
		cout << to_string(problems[i].term.vars()) << endl;
		cout << "v" << i << "-\t" << problems[i].traces[0] << endl;
		cout << "v" << i << "+\t" << problems[i].traces[1] << endl;
	}

	// Cluster those traces
	for (int i = (int)problems.size()-1; i >= 0; i--) {
		for (int j = i-1; j >= 0; j--) {
			if (problems[i].term.vars() == problems[j].term.vars()) {
				array<petri::path_set, 2> traces = problems[j].traces;
				if (not traces[0].merge(*base, problems[i].traces[0])) {
					continue;
				}

				if (not traces[1].merge(*base, problems[i].traces[1])) {
					continue;
				}

				problems[j].traces = traces;
				problems.erase(problems.begin() + i);
				break;
			}
		}
	}

	cout << "After Clustering" << endl;
	// Insert state variable transitions
	for (int i = 0; i < (int)problems.size(); i++) {
		cout << to_string(problems[i].term.vars()) << endl;
		cout << "v" << i << "-\t" << problems[i].traces[0] << endl;
		cout << "v" << i << "+\t" << problems[i].traces[1] << endl;

		array<vector<petri::iterator>, 2> assign;

		// Figure out where to put the state variable transitions
		for (int j = 0; j < 2; j++) {		
			while (not problems[i].traces[j].total.is_empty()) {
				vector<petri::iterator> pos = problems[i].traces[j].total.maxima();
				// TODO(edward.bingham) I should pick the one that triggers the fewest state suspects
				petri::iterator best;
				int count = -1;
				for (auto k = pos.begin(); k != pos.end(); k++) {
					int cost = 0;
					for (int l = 0; l < (int)suspects.size(); l++) {
						if (suspects[l].sense == j and find(suspects[l].first.begin(), suspects[l].first.end(), *k) != suspects[l].first.end()) {
							cost++;
						}
					}
					if (count < 0 or cost < count) {
						best = *k;
						count = cost;
					}
				}

				assign[j].push_back(best);
				problems[i].traces[j] = problems[i].traces[j].avoidance(best);
			}
		}
		
		cout << "inserting v" << i << "+ at " << to_string(assign[1]) << endl;
		cout << "inserting v" << i << "- at " << to_string(assign[0]) << endl;

		// Create the state variable
		ucs::variable v;
		v.name.push_back(ucs::instance("v" + ::to_string(i), vector<int>()));
		int vid = variables.define(v);

		// Figure out the reset states
		petri::path_set v0 = petri::trace(*base, assign[0], assign[1]);
		petri::path_set v1 = petri::trace(*base, assign[1], assign[0]);
		cout << "Looking for Reset for v" << i << endl;
		cout << "v" << i << "-; v" << i << "+ " << v0 << endl;
		cout << "v" << i << "+; v" << i << "- " << v1 << endl;
		for (auto s = base->reset.begin(); s != base->reset.end(); s++) {
			int reset = 2;
			for (auto t = s->tokens.begin(); t != s->tokens.end(); t++) {
				petri::iterator pos(place::type, t->index);
				if (v0.total[pos] > 0) {
					if (reset == 0 or reset == 2) {
						reset = 0;
					} else {
						reset = -1;
					}
				} else if (v1.total[pos] > 0) {
					if (reset == 1 or reset == 2) {
						reset = 1;
					} else {
						reset = -1;
					}
				}

				if (reset < 0) {
					break;
				}
			}
			if (reset < 0) {
				reset = 2;
			}
			s->encodings.set(vid, reset);
		}

		// Insert the transitions into the graph
		for (int j = 0; j < 2; j++) {
			for (auto k = assign[j].begin(); k != assign[j].end(); k++) {
				petri::iterator pos;
				if (k->type == place::type) {
					pos = base->insert_before(*k, transition());
				} else {
					base->insert_after(*k, transition(
						1, base->transitions[k->index].local_action,
						base->transitions[k->index].remote_action));
					pos = *k;
				}
				base->transitions[pos.index].local_action = boolean::cover(vid, j);
				base->transitions[pos.index].remote_action = base->transitions[pos.index].local_action.remote(variables.get_groups());
			}
		}
	}

	printf("done insert state variables\n");
	base->update_masks();
	base->source = base->reset;

	// The following steps are guidelines and not hard rules. If you think you
	// found a better way to approach the problem, then feel free to chase that
	// down. If you need supporting infrastructure anywhere else in the project,
	// feel free to add that in. If you need to modify this function definition,
	// go for it.

	// 1. Learn some of the underlying infrastructure.
	//   a. Create a new variable using ucs::variable_set::define() from
	//      haystack/lib/ucs/ucs/variable.h
	//   b. Set up the reset behavior for that variable looking at
	//      graph::reset[].encodings from haystack/lib/hse/hse/graph.h and
	//      haystack/lib/hse/hse/state.h
	//   c. Insert a pair of transitions for that variable into the HSE. Look at
	//      petri::graph::insert_after(), petri::graph::insert_before(), and
	//      petri::graph::insert_alongside() from haystack/lib/petri/petri/graph.h
  //
  // 2. Create a list of paths that need to be cut.
	// 	 a. Understand encoder::conflict and encoder::suspects, we
	// 	    need to cut all of the paths between all of the conflicts,
	// 	    making sure not to create new conflicts as a result of the
	// 	    suspects.
	//   b. For a given conflict, identify all other transitions on
	//      the variable experiencing this state conflict.
	//   c. walk the graph from conflict::region to conflict::index
	//      and trace all paths from one to the other that does not
	//      include another transition on the same variable.
	//
	// 3. Try to cut those paths (this will create new conflicts).
	//   a. Pick a pair of places which cut the majority of paths
	//   b. Insert your transitions at those two places.
	//
	// 4. Iterate	
	//   a. Create a depth first search algorithm where we insert
	//      transitions to disambiguate a conflict, then re-evaluate the
	//      state encodings and conflicts, and repeat for different
	//      orderings of conflicts to tackle. The algorithm should
	//      complete when the first solution is found.
	//
	// === Successful completion of project ===
	// 
	// Your time is yours, what do you want to tackle next?
	// Some ideas:
	// 1. Perhaps find a way to update the predicate space and
	//    conflicts without redoing the whole simulation
	// 2. Perhaps find better ways to direct and prune the depth first
	//    search algorithm
	// 3. Jump on another project

	// Final cleanup:
	// 1. Clean up any bugs
	// 2. Prepare demo
	// 3. Document as needed
}

}
