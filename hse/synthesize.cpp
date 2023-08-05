#include "synthesize.h"

namespace hse
{

struct gate
{
	int reset;
	vector<petri::iterator> tids[2];
	boolean::cover implicant[2];
	boolean::cover exclusion[2];
};

void guard_weakening(graph &proc, prs::production_rule_set *out, ucs::variable_set &v, bool senseless, bool report_progress)
{
	vector<vector<vector<int> > > pdeps(proc.places.size(), vector<vector<int> >());
	vector<vector<vector<int> > > tdeps(proc.transitions.size(), vector<vector<int> >());

	for (int i = 0; i < (int)proc.places.size(); i++)
		pdeps[i] = proc.get_dependency_tree(hse::iterator(hse::place::type, i));

	for (int i = 0; i < (int)proc.transitions.size(); i++)
		tdeps[i] = proc.get_dependency_tree(hse::iterator(hse::transition::type, i));

	vector<gate> gates;
	gates.resize(v.nodes.size());

	for (int i = 0; i < (int)gates.size(); i++) {
		if (proc.reset.size() > 0) {
			gates[i].reset = proc.reset[0].encodings.get(i);
		}
	}

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (int i = 0; i < (int)proc.transitions.size(); i++)
	{
		//if (report_progress)
		//	progress("", "T" + ::to_string(i) + "/" + ::to_string(proc.transitions.size()), __FILE__, __LINE__);
		boolean::cover predicate = 1;

		vector<int> p = proc.prev(petri::transition::type, i);
		if (!proc.transitions[i].local_action.is_tautology() || p.size() > 1)
		{
			// The state encodings for the implicant set of states of the transition.
			// these are the state encodings for which there is no vacuous transition that would
			// take a token off of any of the places.
			for (int j = 0; j < (int)p.size(); j++)
				predicate &= proc.places[p[j]].predicate;
		}

		// The transition actively affects the state of the system
		for (int j = 0; j < (int)proc.transitions[i].local_action.cubes.size(); j++)
		{
			vector<int> vars = proc.transitions[i].local_action.cubes[j].vars();
			for (int k = 0; k < (int)vars.size(); k++) {
				int val = proc.transitions[i].local_action.cubes[j].get(vars[k]);

				// we cannot use the variables affected by the transitions in their rules because that would
				// make the rules self-invalidating, so we have to mask them out.
				boolean::cover implicant = (predicate & proc.transitions[i].guard).mask(boolean::cube(vars[k], val).mask());
				if (!senseless) {
					implicant = implicant.mask(1-val);
				}

				gates[vars[k]].implicant[val] |= implicant;
				gates[vars[k]].tids[val].push_back(petri::iterator(hse::transition::type, i));
			}
		}
	}

	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();

		for (int val = 0; val < 2; val++) {
			// We're going to check this transition against all of the places in the system
			for (int k = 0; k < (int)proc.places.size(); k++)
			{
				petri::iterator pid(petri::place::type, k);
				//vector<int> assigns = proc.associated_assigns(vector<int>(1, k));

				// The place is in the same process as the implicant set of states, its not in parallel with the transition we're checking,
				// and it isn't part of any implicant state
				// Check to make sure they aren't forced to be
				// mutually exclusive by an arbiter
				bool relevant = false;
				for (auto tid = gate->tids[val].begin(); tid != gate->tids[val].end() && !relevant; tid++) {
					vector<int> p = proc.prev(petri::transition::type, tid->index);
					relevant = ((proc.is_reachable(*tid, pid) || proc.is_reachable(pid, *tid)) &&
					             find(p.begin(), p.end(), k) == p.end() &&
											 !proc.is_parallel(pid, *tid) &&
											 !proc.common_arbiter(tdeps[tid->index], pdeps[k]));
				}

				if (relevant) {
					// Get only the state encodings for the place for which the transition is invacuous and
					// there is no other vacuous transition that would take a token off the place.
					gate->exclusion[val] |= proc.places[k].effective & boolean::cube(var, 1-val);

					// The implicant states for any actions that would have the same effect as the transition we're checking
					// can be removed from our check because "it could have fired there anyways"
					vector<int> check_transitions = proc.next(petri::place::type, k);
					for (int l = 0; l < (int)check_transitions.size(); l++) {
						for (int m = 0; m < (int)proc.transitions[check_transitions[l]].local_action.cubes.size(); m++) {
							// Check if this transition has the same effect
							if (!are_mutex(proc.transitions[check_transitions[l]].local_action.cubes[m], boolean::cube(var, 1-val))) {
								vector<int> check_places = proc.prev(petri::transition::type, check_transitions[l]);
								boolean::cover check = 1;
								// If it does, get its implicant states and remove them from the check
								for (int n = 0; n < (int)check_places.size(); n++)
									check &= proc.places[check_places[n]].predicate;
								check &= proc.transitions[check_transitions[l]].guard;

								gate->exclusion[val] |= check;
							}
						}
					}
				}
			}
		}
	}

	ucs::variable resetDecl, _resetDecl;
	resetDecl.name.push_back(ucs::instance("Reset", vector<int>()));
	_resetDecl.name.push_back(ucs::instance("_Reset", vector<int>()));
	int reset = v.define(resetDecl);
	int _reset = v.define(_resetDecl);

	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();

		for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				boolean::espresso(gate->implicant[val], ~gate->exclusion[val] & ~gate->implicant[val], gate->exclusion[val]);
			}
		}

		if (gate->reset != 2 && (gate->implicant[0] | gate->implicant[1]).is_tautology()) {
			gate->reset = 2;
		}

		if (gate->reset == 1) {
			gate->implicant[0] &= boolean::cube(_reset, 1);
			gate->implicant[1] |= boolean::cube(_reset, 0);
		} else if (gate->reset == 0) {
			gate->implicant[0] |= boolean::cube(reset, 1);
			gate->implicant[1] &= boolean::cube(reset, 0);
		}

		for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				out->rules.push_back(prs::production_rule());
				out->rules.back().guard = gate->implicant[val];
				out->rules.back().local_action = boolean::cube(var, val);
			}
		}
	}
}

void guard_strengthening(graph proc, prs::production_rule_set *out)
{
}

}
