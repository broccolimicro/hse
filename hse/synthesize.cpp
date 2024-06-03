#include "synthesize.h"

namespace hse
{

gate::gate() {
	reset = 2;
}

gate::~gate() {
}

bool gate::is_combinational()
{
	// TODO we really only need to cover all of the holding states
	return (implicant[0] | implicant[1]).is_tautology();
}

void gate::weaken_espresso()
{
	for (int val = 0; val < 2; val++) {
		if (tids[val].size() > 0) {
			boolean::espresso(implicant[val], ~exclusion[val] & ~implicant[val], exclusion[val]);
		}
	}

	if (tids[0].size() == 0 || tids[1].size() == 0)
		return;

	int width[2];
 	width[0] = implicant[0].area();
 	width[1] = implicant[1].area();
	boolean::cover not_implicant[2];
 	not_implicant[0] = ~implicant[0];
 	not_implicant[1] = ~implicant[1];
	bool comb[2];
	comb[0] = are_mutex(not_implicant[0], exclusion[1]);
	comb[1] = are_mutex(not_implicant[1], exclusion[0]);
	
	if (comb[0] && comb[1]) {
		if (width[0] < width[1]) {
			implicant[1] = not_implicant[0];
		} else {
			implicant[0] = not_implicant[1];
		}
	} else if (comb[0]) {
		implicant[1] = not_implicant[0];
	} else if (comb[1]) {
		implicant[0] = not_implicant[1];
	}
}

void gate::weaken_brute_force()
{
	vector<boolean::cover> incl[2] = {vector<boolean::cover>(), vector<boolean::cover>()};
	vector<int> idx[2] = {vector<int>(), vector<int>()};
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < (int)implicant[i].cubes.size(); j++) {
			incl[i].push_back(boolean::weaken(implicant[i].cubes[j], exclusion[i]));
			idx[i].push_back(0);
		}
	}

	bool change = true;
	while (change) {
		change = false;
		for (int i = 0; i < (int)idx[0].size(); i++) {
			for (int j = 0; j < (int)idx[1].size(); j++) {
				if (!are_mutex(incl[0][i].cubes[idx[0][i]], incl[1][j].cubes[idx[1][j]])) {
					if (i < (int)idx[0].size()-1 && (j >= (int)idx[1].size()-1 || incl[0][i].cubes[idx[0][i]+1] < incl[1][j].cubes[idx[1][j]+1])) {
						idx[0][i]++;
						change = true;
					} else if (j < (int)idx[1].size()-1) {
						idx[1][j]++;
						change = true;
					}
				}
			}
		}
	}

	for (int i = 0; i < 2; i++) {
		boolean::cover candidates;
		for (int j = 0; j < (int)idx[i].size(); j++) {
			candidates.cubes.push_back(incl[i][j].cubes[idx[i][j]]);
		}
		candidates.minimize();
		sort(candidates.cubes.begin(), candidates.cubes.end());

		bool change = true;
		while (change) {
			change = false;
			
			for (int j = (int)candidates.cubes.size()-1; j >= 0; j--) {
				boolean::cover test = candidates;
				test.cubes.erase(test.begin() + j);
				if (implicant[i].is_subset_of(test)) {
					candidates = test;
					change = true;
					break;
				}
			}
		}

		implicant[i] = candidates;
	}

	if (are_mutex(~implicant[0], exclusion[1])) {
		implicant[1] = ~implicant[0];
	} else if (are_mutex(~implicant[1], exclusion[0])) {
		implicant[0] = ~implicant[1];
	}
}

gate_set::gate_set() {
	base = nullptr;
	vars = nullptr;
}

gate_set::gate_set(graph *base, ucs::variable_set *vars) {
	this->base = base;
	this->vars = vars;
}

gate_set::~gate_set() {
}

void gate_set::load(bool senseless) {
	gates.clear();
	gates.resize(vars->nodes.size());

	for (int i = 0; i < (int)gates.size(); i++) {
		if (base->reset.size() > 0) {
			gates[i].reset = base->reset[0].encodings.get(i);
		}
	}

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (auto t0i = base->transitions.begin(); t0i != base->transitions.end(); t0i++) {
		petri::iterator t0(petri::transition::type, t0i - base->transitions.begin());
		
		// The state encodings for the implicant set of states of the transition.
		// these are the state encodings for which there is no vacuous transition that would
		// take a token off of any of the places.
		boolean::cover predicate = base->predicate(t0);
		boolean::cover implicant = predicate & t0i->guard;

		// The transition actively affects the state of the system
		for (auto term = t0i->local_action.cubes.begin(); term != t0i->local_action.cubes.end(); term++) {
			vector<int> vars = term->vars();
			for (auto var = vars.begin(); var != vars.end(); var++) {
				int val = term->get(*var);

				// we cannot use the variables affected by the transitions in their rules because that would
				// make the rules self-invalidating, so we have to mask them out.
				boolean::cover term_implicant = implicant & boolean::cube(*var, 1-val);
				if (!senseless) {
					term_implicant = term_implicant.mask(1-val);
				}
				term_implicant.hide(*var);
				term_implicant.minimize();

				gates[*var].implicant[val] |= term_implicant;
				gates[*var].tids[val].push_back(t0);
			}
		}
	}

	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();
		
		for (int val = 0; val < 2; val++) {
			// We're going to check this transition against all of the places in the system
			vector<petri::iterator> relevant = base->relevant_nodes(gate->tids[val]);
		
			//cout << "Production rule for " << var << " " << val << " " << ::to_string(gate->tids[val]) << endl;
			for (auto n = relevant.begin(); n != relevant.end(); n++) {
				//cout << "excluding " << *n << endl;
				if (n->type == petri::place::type) {
					//cout << export_expression(base->places[n->index].effective, v).to_string() << " " << export_expression(boolean::cube(var, 1-val), v).to_string() << endl;
					//cout << export_expression(base->effective(*n), v).to_string() << " " << export_expression(boolean::cube(var, 1-val), v).to_string() << endl;
					gate->exclusion[val] |= base->effective(*n) & boolean::cube(var, 1-val);
				} else {
					boolean::cover predicate = base->predicate(*n);
					//gate->exclusion[val] |= predicate & ~base->transitions[n->index].guard & boolean::cube(var, 1-val);
					if (find(gate->tids[1-val].begin(), gate->tids[1-val].end(), *n) != gate->tids[1-val].end()) {
						gate->exclusion[val] |= predicate & base->transitions[n->index].guard;
					} else {
						gate->exclusion[val] |= predicate & base->transitions[n->index].guard & boolean::cube(var, 1-val);
					}
				}
			}
		}
	}
}

void gate_set::weaken() {
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		gate->weaken_brute_force();
	}
}

void gate_set::build_reset() {
	ucs::variable resetDecl, _resetDecl;
	resetDecl.name.push_back(ucs::instance("Reset", vector<int>()));
	_resetDecl.name.push_back(ucs::instance("_Reset", vector<int>()));
	int reset = vars->define(resetDecl);
	int _reset = vars->define(_resetDecl);

	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		if (gate->reset != 2 && gate->is_combinational()) {
			// TODO not sure if this is actually correct
			gate->reset = 2;
		}

		if (gate->reset == 1) {
			gate->implicant[0] &= boolean::cube(_reset, 1);
			gate->implicant[1] |= boolean::cube(_reset, 0);
		} else if (gate->reset == 0) {
			gate->implicant[0] |= boolean::cube(reset, 1);
			gate->implicant[1] &= boolean::cube(reset, 0);
		}
	}
}

void gate_set::build_staticizers() {
}

void gate_set::build_precharge() {
}

void gate_set::harden_state() {
}

void gate_set::build_shared_gates() {
}

void gate_set::save(prs::production_rule_set *out) {
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();

		for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				out->rules.push_back(prs::production_rule());
				out->rules.back().guard = gate->implicant[val];
				out->rules.back().local_action = boolean::cube(var, val);
			}
		}
	}
}

}

