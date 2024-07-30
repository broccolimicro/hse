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
	vector<petri::iterator> neighbors = base->neighbors(node, false);
	neighbors.push_back(node);
	sort(neighbors.begin(), neighbors.end());

	vector<int> merge;
	for (int i = 0; i < (int)conflicts.size(); i++) {
		if (conflicts[i].index.index == tid &&
				conflicts[i].index.term == term &&
				conflicts[i].sense == sense && 
				vector_intersects(neighbors, conflict_regions[i])) {
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
	vector<petri::iterator> neighbors = base->neighbors(j, false);
	neighbors.push_back(j);
	sort(neighbors.begin(), neighbors.end());

	vector<int> merge;
	for (int l = 0; l < (int)suspects.size(); l++) {
		if (suspects[l].first == i && suspects[l].sense == sense && vector_intersection_size(neighbors, suspect_regions[l]) > 0) {
			merge.push_back(l);
		}
	}

	if (not merge.empty()) {
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
		
		sort(suspects[merge[0]].second.begin(), suspects[merge[0]].second.end());
		suspects[merge[0]].second.erase(unique(suspects[merge[0]].second.begin(), suspects[merge[0]].second.end()), suspects[merge[0]].second.end());
		
		sort(suspect_regions[merge[0]].begin(), suspect_regions[merge[0]].end());
		suspect_regions[merge[0]].erase(unique(suspect_regions[merge[0]].begin(), suspect_regions[merge[0]].end()), suspect_regions[merge[0]].end());
	} else {
		suspects.push_back(suspect(sense, i, vector<petri::iterator>(1, j)));
		suspect_regions.push_back(neighbors);
	}
}

vector<suspect> encoder::find_suspects(vector<petri::iterator> pos, int sense) {
	vector<suspect> result;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		vector<bool> found(result.size(), false);
		for (auto j = suspects.begin(); j != suspects.end(); j++) {
			if (j->sense == sense and find(j->first.begin(), j->first.end(), *i) != j->first.end()) {
				if (i == pos.begin()) {
					result.push_back(*j);
				} else {
					auto best = result.end();
					int best_score = 0;
					for (auto k = result.begin(); k != result.end(); k++) {
						int score = vector_intersection_size(k->second, j->second);
						if (score > best_score) {
							best = k;
							best_score = score;
						}
					}

					if (best != result.end()) {
						found[best-result.begin()] = true;
						best->second = vector_intersection(best->second, j->second);
					}
				}
			}
		}

		for (int j = (int)found.size()-1; j >= 0; j--) {
			if (not found[j]) {
				result.erase(result.begin() + j);
			}
		}
	}
	return result;
}

void encoder::check(ucs::variable_set &variables, bool senseless, bool report_progress)
{
	//cout << "Computing Parallel Groups" << endl;
	base->compute_split_groups(parallel);
	/*cout << "Parallel Groups" << endl;
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
	}*/


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

		for (auto p1 = relevant.begin(); p1 != relevant.end(); p1++) {
			if (report_progress) {
				progress("", "T" + ::to_string(t0.index) + "->P" + ::to_string(p1->index), __FILE__, __LINE__);
			}

			boolean::cover p1_effective = base->effective_implicant(*p1);
		
			for (int sense = senseless ? -1 : 0; senseless ? sense == -1 : sense < 2; sense++) {
				// Check the implicant predicate against the other place's effective.
				// The predicate is used because inserting a state variable
				// transition will negate any "you might as well be here" effects.
				if (!are_mutex(implicant.mask(sense), p1_effective)) {
					add_suspect(vector<petri::iterator>(1, t0), *p1, 1-sense);
				}
			}
		}

		for (int term = 0; term < (int)t0i->local_action.cubes.size(); term++) {
			boolean::cube action = t0i->local_action.cubes[term];
			boolean::cover not_action = ~action;

			for (int sense = senseless ? -1 : 0; senseless ? sense == -1 : sense < 2; sense++) {
				// If we aren't specifically checking senses or this transition affects the senses we are checking
				if (!senseless && not action.has(sense)) {
					continue;
				}

				// we cannot use the variables affected by the transitions in their rules because that would
				// make the rules self-invalidating, so we have to mask them out.
				boolean::cover term_implicant = (implicant & not_action).mask(action.mask()).mask(sense);

				for (auto i = relevant.begin(); i != relevant.end(); i++) {
					// Get the list of places that are suspect against parallel merges
					// A place is suspect if making it an implicant for an active transition
					// could make it a conflict. A parallel merge can end up as a suspect
					// against individual places at the merge as a result of different
					// parallel states. This suspect will be replicated with places
					// outside the parallel merge, so we don't need to include it.
					//
					// TODO(edward.bingham) I don't actually think this is necessary. I
					// think if we take the intersection of the suspects at each place,
					// we'll get the same result.
					// if (t0_prev.size() > 1 && !are_mutex(predicate.mask(sense), base->effective_implicant(*i)) and find(t0_prev.begin(), t0_prev.end(), *i) == t0_prev.end()) {
					// 	add_suspect(t0_prev, *i, 1-sense);
					// }

					// Get only the state encodings for the place for which the transition is invacuous and
					// there is no other vacuous transition that would take a token off the place.
					boolean::cover encoding = base->effective_implicant(*i);

					// The implicant states for any actions that would have the same effect as the transition we're checking
					// can be removed from our check because "it could have fired there anyways"
					// If all terms cover all of the assignments in t0, then it can't conflict
					
					if (t0.index == 14 and i->type == place::type and i->index == 19) {
						cout << "base encoding" << export_expression(encoding, variables).to_string() << endl;
					}

					if (not base->firing_conflicts(*i, term_implicant, action)) {
						continue;
					}

					// The state encodings for the implicant set of states of the transition.
					// these are the state encodings for which there is no vacuous transition that would
					// take a token off of any of the places.
					// Get only the state encodings for the place for which the transition is invacuous and
					// there is no other vacuous transition that would take a token off the place.

					// Check if they share a common state encoding given the above checks
					encoding &= not_action;

					if (t0.index == 14 and i->type == place::type and i->index == 19) {
						cout << "here " << t0 << " -> " << *i << endl;
						cout << "term implicant" << export_expression(term_implicant, variables).to_string() << endl;
						cout << "final encoding" << export_expression(encoding, variables).to_string() << endl;
					}
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

			boolean::cover p1_effective = base->effective_implicant(*p1);
		
			for (int sense = senseless ? -1 : 0; senseless ? sense == -1 : sense < 2; sense++) {
				// Check the implicant predicate against the other place's effective.
				// The predicate is used because inserting a state variable
				// transition will negate any "you might as well be here" effects.
				if (!are_mutex(p0_predicate.mask(sense), p1_effective)) {
					add_suspect(vector<petri::iterator>(1, p0), *p1, 1-sense);
				}
			}
		}
	}

	if (report_progress)
		done_progress();
}

int encoder::score_insertion(int sense, vector<petri::iterator> pos, const petri::path_set &dontcare) {
	sort(pos.begin(), pos.end());
	vector<suspect> new_conflicts = find_suspects(pos, sense);
	
	int cost = 0;
	for (auto i = new_conflicts.begin(); i != new_conflicts.end(); i++) {
		// TODO(edward.bingham) I should check state suspects against
		// previous insertions. I should insert the assignments that are
		// more certain first.

		cost += not dontcare.covers(i->second);
	}

	if (pos.size() == 1 and pos[0].type == transition::type) {
		cost += base->transitions[pos[0].index].local_action.has(sense);
	} else {
		vector<petri::iterator> nn = base->next(pos);
		sort(nn.begin(), nn.end());
		nn.erase(unique(nn.begin(), nn.end()), nn.end());
		for (auto i = nn.begin(); i != nn.end(); i++) {
			cost += base->transitions[i->index].local_action.has(sense);
		}
	}

	return cost;
}

int encoder::score_insertion(int sense, petri::iterator pos, const petri::path_set &dontcare, vector<vector<petri::iterator> > *result) {
	vector<suspect> new_conflicts = find_suspects(vector<petri::iterator>(1, pos), sense);

	int bestcost = score_insertion(sense, vector<petri::iterator>(1, pos), dontcare);
	vector<petri::iterator> best(1, pos);
	//cout << "(" << pos << ", 0) -> " << bestcost << endl;

	vector<petri::iterator> p = base->prev(pos);
	if (pos.type == transition::type) {
		if (not base->transitions[pos.index].guard.is_tautology() and p.size() > 1 and dontcare.touches(p)) {
			int cost = score_insertion(sense, p, dontcare);
			//cout << "(" << pos << ", -1) -> " << cost << endl;
			if (cost < bestcost) {
				bestcost = cost;
				best = p;
			}
		}
	} else {
		for (auto m = p.begin(); m != p.end(); m++) {
			vector<petri::iterator> n = base->next(*m);
			if (n.size() > 1) {
				int cost = score_insertion(sense, n, dontcare);
				//cout << "(" << *m << ", 1) -> " << cost << endl;
				if (cost < bestcost) {
					bestcost = cost;
					best = n;
				}		
			}
		}
	}

	if (result != nullptr) {
		result->push_back(best);
	}
	return bestcost;
}

int encoder::score_insertions(int sense, vector<petri::iterator> pos, const petri::path_set &dontcare, vector<vector<petri::iterator> > *result) {
	int cost = 0;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		cost += score_insertion(sense, *i, dontcare, result);
	}
	return cost;
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
		// Group paths into rings, then merge after the fact.
		array<petri::path_set, 2> traces;
	};
	vector<CodingProblem> problems;
	for (int i = 0; i < (int)conflicts.size(); i++) {
		vector<petri::iterator> from(conflicts[i].region);
		vector<petri::iterator> to(1, conflicts[i].index.iter());
		
		if (conflicts[i].sense == conflict::UP) {
			problems.push_back(CodingProblem{
				base->term(conflicts[i].index).mask(conflict::DOWN), {
				petri::trace(*base, to, from),
				petri::trace(*base, from, to)
			}});
		} else {
			problems.push_back(CodingProblem{
				base->term(conflicts[i].index).mask(conflict::UP), {
				petri::trace(*base, from, to),
				petri::trace(*base, to, from)
			}});
		}

		// Remove vacuous traces
		// DESIGN(edward.bingham) There is guaranteed to be exactly one path set in
		// each of the trace lists for each coding problem by construction.
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

	/*cout << "Before Clustering" << endl;
	for (int i = 0; i < (int)problems.size(); i++) {
		cout << to_string(problems[i].term.vars()) << endl;
		cout << "v" << i << "-\t" << problems[i].traces[0] << endl;
		cout << "v" << i << "+\t" << problems[i].traces[1] << endl;
	}*/

	// TODO(edward.bingham) State variable insertion ended up being far more
	// complicated that initially thought. Currently, we're implementing a very
	// naive trace clustering algorithm, but we can do better.
	//
	// Clustering:
	//
	// 1. determine which conflicts and suspects are mergible, create a graph of
	//    mergible suspects and conflicts such that the edges of that graph are
	//    weighted by the mergibility type. There are two types of mergibility
	//    between two conflicts/suspects:
	//    Type 1: The merged conflicts can be solved with a pair of transitions v0+; v0-
	//    Type 2: The merged conflicts can be solved with four transitions v0+; v0-; v0+; v0-
	//
	// 2. Find all maximal cliques in the graph of type 1 and 2 mergible
	//    conflicts and type 1 mergible conflicts and suspects. This prevents us
	//    from adding transitions into the handshaking expansion unecessarily as
	//    a result of untriggered suspects.
	//    *** This actually doesn't work. up-down patterns create cycles, not cliques.   
	//
	// 3. Select the smallest subset of non-overlapping cliques that maximizes
	//    coverage of all conflicts.
	//
	// 4. Given that subset, for any two overlapping cliques, remove the
	//    overlapped vertex from one of the two cliques with preference toward
	//    elimination/reduction of type 2 mergibility. This gives us more options
	//    for where to place the state variable transition to solve the encoding
	//    problem.
	//
	// Search and Solve:
	//
	// 1. 
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

	// TODO(edward.bingham) I need to make it possible to insert a state
	// variable transition before/after a parallel split or merge. To me, it
	// seems the simplest way to do this is to have four insertion classes.
	// 1. At a place
	// 2. At a transition
	// 3. After a transition
	// 4. Before a transition
	// However, certain places will overlap with insertions before or after a
	// transition. So the insertions before or after transition may only be
	// valid if there are multiple inputs/outputs to that transition.
	// A pair of places will never share *both* an input transition and an
	// output transition. If so, then we would simply delete one of the two
	// plces in the post_process() function. This means that we could represent
	// the "before this transition" as a set of input places and the "after
	// this transition" as a set of output places. This would also allow us to
	// insert state variables partially before or partially after a parallel
	// split.

	// assign[location of state variable insertion][direction of assignment] = {variable uids}
	map<vector<petri::iterator>, array<vector<int>, 2> > groups;

	int uid = -1;
	//cout << "After Clustering" << endl;
	// Figure out where to insert state variable transitions, create state
	// variables, and determine their reset state.
	for (int i = 0; i < (int)problems.size(); i++) {
		cout << to_string(problems[i].term.vars()) << endl;
		cout << "v" << i << "-\t" << problems[i].traces[0] << endl;
		cout << "v" << i << "+\t" << problems[i].traces[1] << endl;

		// Create the state variable
		int vid = -1;
		while (vid < 0) {
			++uid;
			ucs::variable v;
			v.name.push_back(ucs::instance("v" + ::to_string(uid), vector<int>()));
			vid = variables.define(v);
		}

		array<vector<vector<petri::iterator> >, 2> options;
		options[0] = problems[i].traces[0].enumerate();
		options[1] = problems[i].traces[1].enumerate();

		array<vector<vector<petri::iterator> >, 2> best;
		array<petri::path_set, 2> colorings({
			petri::path_set(base->places.size(), base->transitions.size()),
			petri::path_set(base->places.size(), base->transitions.size())
		});
		int min_cost = -1;

		cout << "options[0] = " << to_string(options[0]) << endl;
		cout << "options[1] = " << to_string(options[1]) << endl;

		// Figure out where to put the state variable transitions
		for (auto j = options[0].begin(); j != options[0].end(); j++) {
			for (auto k = options[1].begin(); k != options[1].end(); k++) {
				// Check for interfering transitions
				bool interfering = false;
				for (auto ji = j->begin(); ji != j->end() and not interfering; ji++) {
					for (auto ki = k->begin(); ki != k->end() and not interfering; ki++) {
						interfering = base->is(parallel, *ji, *ki);
					}
				}

				if (not interfering) {
					petri::path_set v0 = petri::trace(*base, *j, *k, true);
					petri::path_set v1 = petri::trace(*base, *k, *j, true);

					// check each transition against suspects and generate a score
					// find the pair with the best score.

					// TODO(edward.bingham) search all partial states from this
					// insertion for the one that minimizes the number of new conflicts.
					// I actually know that answer ahead of time based on the overlap in
					// the set of suspects of the two parallel places and the sense of
					// the input and output transitions. Pruning the search using this
					// parameters would make for a very efficient greedy algorithm which
					// is quadratic time relative to the number of parallel nodes. The
					// only problem with this approach is that it requires that I use
					// petri::insert_at() which ends up removing places. Those removed
					// places would have been used in other overlapping insertions.
					// Step 1: Tackle the insert_at() approach
					//   * I could keep all of the redundant places until after all of
					//     the state variable insertions have been done, then do a
					//     search for redundant places and remove them. This would
					//     maintain the indices and structure needed for the other state
					//     variable insertions.
					//   * As I execute overalapping insertions, I need to add all newly
					//     redundant places into the next insertion. See page 228 of my
					//     "circuits" notebook.
					// Step 2: Partial state searching 
					array<vector<vector<petri::iterator> >, 2> curr;
					int cost = score_insertions(1, *k, v1, &curr[1])
						+ score_insertions(0, *j, v0, &curr[0]);
					if (min_cost < 0 or cost < min_cost) {
						min_cost = cost;
						best = curr;
						colorings[0] = v0;
						colorings[1] = v1;
					}
				}
			}
		}

		for (int j = 0; j < 2; j++) {
			for (auto k = best[j].begin(); k != best[j].end(); k++) {
				cout << "Adding (" << to_string(*k) << endl;
				auto iter = groups.insert(pair<vector<petri::iterator>, array<vector<int>, 2> >(*k, array<vector<int>, 2>()));
				iter.first->second[j].push_back(vid);
			}
		}
		
		cout << "inserting v" << uid << "+ at " << to_string(best[1]) << endl;
		cout << "inserting v" << uid << "- at " << to_string(best[0]) << endl;

		// Figure out the reset states
		cout << "Looking for Reset for v" << uid << endl;
		cout << "v" << i << "-; v" << uid << "+ " << colorings[0] << endl;
		cout << "v" << i << "+; v" << uid << "- " << colorings[1] << endl;
		for (auto s = base->reset.begin(); s != base->reset.end(); s++) {
			int reset = 2;
			for (auto t = s->tokens.begin(); t != s->tokens.end(); t++) {
				petri::iterator pos(place::type, t->index);
				if (colorings[0].total[pos] > 0) {
					if (reset == 0 or reset == 2) {
						reset = 0;
					} else {
						reset = -1;
					}
				} else if (colorings[1].total[pos] > 0) {
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
	}

	// This will group state variable transitions by their insertion location.
	// When multiple state variables are inserted at the same location, we have
	// to be careful about the composition of those transitions. Given a
	// downgoing output transition a&~b->x-, we need to insert state variables
	// like so: [a]; (v0-,v1-); [~b]; (v2+,v3+); x-. For upgoing a&~b->x+, its
	// inverted: [~b]; (v2+,v3+); [a]; (v0-,v1-); x+. If the guard has multiple
	// terms with different senses, then this becomes problematic. For example,
	// a&~b|~a&b->x-.

	// TODO(edward.bingham) handle conflicting multiterm guards.

	// Insert the transitions into the graph
	for (auto group = groups.begin(); group != groups.end(); group++) {
		if (group->second[0].empty() and group->second[1].empty()) {
			// This should never happen
			cout << "error: both sides of state transition group are empty" << endl;
			continue;
		}
		// This is the set of output transitions (n) and input places (p) for this state variable insertion.
		vector<petri::iterator> n;
		for (auto i = group->first.begin(); i != group->first.end(); i++) {
			if (i->type == transition::type) {
				n.push_back(*i);
			} else {
				vector<petri::iterator> nn = base->next(*i);
				n.insert(n.end(), nn.begin(), nn.end());
			}
		}
		sort(n.begin(), n.end());
		n.erase(unique(n.begin(), n.end()), n.end());

		// Figure out the output sense of all of the transitions.
		int sense = 2;
		for (auto i = n.begin(); i != n.end() and sense >= 0; i++) {
			if (base->transitions[i->index].local_action.has(1)) {
				if (sense == 2 or sense == 1) {
					sense = 1;
				} else {
					sense = -1;
				}
			}
			if (base->transitions[i->index].local_action.has(0)) {
				if (sense == 2 or sense == 0) {
					sense = 0;
				} else {
					sense = -1;
				}
			}
		}

		// extract the guard from the transition in the graph if we're inserting at
		// a transition.
		boolean::cover guard = 1;
		for (auto i = group->first.begin(); i != group->first.end(); i++) {
			if (i->type == transition::type) {
				guard &= base->transitions[i->index].guard;
				base->transitions[i->index].guard = 1;
			}
		}
		int guardsense = 2;
		if (guard.has(1)) {
			guardsense = 1;
		}
		if (guard.has(0)) {
			if (guardsense == 2 or guardsense == 0) {
				guardsense = 0;
			} else {
				guardsense = -1;
			}
		}

		// If the output transitions don't agree on a sense, then ordering the
		// state variable transitions won't help. So we should just put all of the
		// transitions in parallel with eachother.
		if (sense != 0 and sense != 1) {
			sense = 0;
		}

		// Otherwise, we want to insert the matched sense transitions first, then
		// insert the unmatched sense transitions. v0+; v1-; x+

		// Insert matched sense transitions
		for (int x = 0; x < 2; x++) {
			if (not group->second[1-sense].empty()) {
				// Handle the first one, then insert the others in parallel to the first one.
				vector<petri::iterator> pos = base->duplicate(
						petri::parallel, 
						base->insert_at(
							base->add_redundant(group->first),
							transition(guardsense != sense or group->second[sense].empty() ? guard : boolean::cover(1))
						),
						group->second[1-sense].size(),
						false);

				auto i = group->second[1-sense].begin();
				auto j = pos.begin();
				for (; i != group->second[1-sense].end() and j != pos.end(); i++, j++) {
					base->transitions[j->index].local_action = boolean::cover(*i, 1-sense);
					base->transitions[j->index].remote_action = base->transitions[j->index].local_action.remote(variables.get_groups());
				}
			}
			sense = 1-sense;
		}
	}

	for (auto i = base->places.begin(); i != base->places.end(); i++) {
		cout << "p" << (i-base->places.begin()) << ":" << endl;
		for (auto j = i->groups[parallel].begin(); j != i->groups[parallel].end(); j++) {
			cout << "\t" << j->to_string() << endl;
		}
	}

	for (auto i = base->transitions.begin(); i != base->transitions.end(); i++) {
		cout << "t" << (i-base->transitions.begin()) << ":" << endl;
		for (auto j = i->groups[parallel].begin(); j != i->groups[parallel].end(); j++) {
			cout << "\t" << j->to_string() << endl;
		}
	}


	//printf("done insert state variables\n");
	base->erase_redundant();
	base->update_masks();
	base->source = base->reset;

	// TODO(edward.bingham) Update the predicate space and conflicts without
	// resimulating the whole state space.
}

}
