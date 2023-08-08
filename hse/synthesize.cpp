#include "synthesize.h"

namespace hse
{

void guard_weakening(graph &proc, prs::production_rule_set *out, ucs::variable_set &v, bool senseless, bool report_progress)
{
	vector<gate> gates;
	gates.resize(v.nodes.size());

	for (int i = 0; i < (int)gates.size(); i++) {
		if (proc.reset.size() > 0) {
			gates[i].reset = proc.reset[0].encodings.get(i);
		}
	}

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (auto t0i = proc.transitions.begin(); t0i != proc.transitions.end(); t0i++) {
		petri::iterator t0(petri::transition::type, t0i - proc.transitions.begin());
		//if (report_progress)
		//	progress("", "T" + ::to_string(i) + "/" + ::to_string(proc.transitions.size()), __FILE__, __LINE__);

		// The state encodings for the implicant set of states of the transition.
		// these are the state encodings for which there is no vacuous transition that would
		// take a token off of any of the places.
		boolean::cover predicate = proc.predicate(t0);
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
			vector<petri::iterator> relevant = proc.relevant_nodes(gate->tids[val]);
			
			for (auto n = relevant.begin(); n != relevant.end(); n++) {
				if (n->type == petri::place::type) {
					gate->exclusion[val] |= proc.effective(*n) & boolean::cube(var, 1-val);
				} else {
					boolean::cover predicate = proc.predicate(*n);
					gate->exclusion[val] |= predicate & ~proc.transitions[n->index].guard & boolean::cube(var, 1-val);
					if (find(gate->tids[1-val].begin(), gate->tids[1-val].end(), *n) != gate->tids[1-val].end()) {
						gate->exclusion[val] |= predicate & proc.transitions[n->index].guard;
					} else {
						gate->exclusion[val] |= predicate & proc.transitions[n->index].guard & boolean::cube(var, 1-val);
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

	//cout << endl;
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();
		/*for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				cout << "exclusion " << export_expression(gate->exclusion[val], v).to_string() << endl;
				cout << "rule " << export_expression(gate->implicant[val], v).to_string() << " -> " << export_composition(boolean::cube(var, val), v).to_string() << endl;
			}
		}*/

		gate->weaken_brute_force();
		
		/*for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				cout << "rule " << export_expression(gate->implicant[val], v).to_string() << " -> " << export_composition(boolean::cube(var, val), v).to_string() << endl;
			}
		}
		cout << endl;*/

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

		for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				out->rules.push_back(prs::production_rule());
				out->rules.back().guard = gate->implicant[val];
				out->rules.back().local_action = boolean::cube(var, val);
			}
		}
	}
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

struct guard_option
{
	guard_option() {}
	guard_option(boolean::cover guard) {
		this->count = guard.area();
		this->guard = guard;
		sort(this->guard.cubes.begin(), this->guard.cubes.end());
	}
	~guard_option() {}

	int count;
	boolean::cover guard;
};

bool operator<(const guard_option &o0, const guard_option &o1)
{
	if (o0.count != o1.count) {
		return o0.count < o1.count;
	}

	if (o0.guard.cubes.size() != o1.guard.cubes.size()) {
		return o0.guard.cubes.size() < o1.guard.cubes.size();
	}

	for (int i = 0; i < (int)o0.guard.cubes.size() && i < (int)o1.guard.cubes.size(); i++) {
		if (o0.guard.cubes[i] != o1.guard.cubes[i]) {
			return o0.guard.cubes[i] < o1.guard.cubes[i];
		}
	}

	return false;
}

bool operator!=(const guard_option &o0, const guard_option &o1)
{
	return o0.count != o1.count || o0.guard != o1.guard;
}

void gate::weaken_brute_force()
{
	// number of terms, guard
	vector<guard_option> options[2];
	options[0].push_back(guard_option(implicant[0]));
	options[1].push_back(guard_option(implicant[1]));

	// val, guard
	vector<pair<int, boolean::cover> > stack;
	cout << implicant[0] << endl;
	cout << implicant[1] << endl;
	stack.push_back(pair<int, boolean::cover>(0, implicant[0]));
	stack.push_back(pair<int, boolean::cover>(1, implicant[1]));

	long n = 0;
	while (stack.size() > 0) {
		if (n%100 == 0) {
			cout << stack.size() << endl;
			n = 0;
		}
		n++;
		pair<int, boolean::cover> curr = stack.back();
		stack.pop_back();

		for (int i = 0; i < (int)curr.second.cubes.size(); i++) {
			vector<int> vars = curr.second.cubes[i].vars();
			for (int j = 0; j < (int)vars.size(); j++) {
				pair<int, boolean::cover> next = curr;
				next.second.cubes[i].hide(vars[j]);
				next.second.minimize();
				if (are_mutex(next.second, exclusion[curr.first])) {
					guard_option toadd(next.second);

					auto loc = lower_bound(options[curr.first].begin(), options[curr.first].end(), toadd);
					if (loc == options[curr.first].end() || loc->guard != toadd.guard) {
						options[curr.first].insert(loc, toadd);
						stack.push_back(next);
					}
				}
			}
		}
	}

	implicant[0] = options[0].back().guard;
	implicant[1] = options[1].back().guard;

	vector<guard_option>::iterator i[2] = {options[0].begin(), options[1].begin()};
	for (; i[0] != options[0].end() && i[1] != options[1].end(); i[0]++, i[1]++) {
		for (int j = 0; j < 2; j++) {
			boolean::cover inv = ~i[j]->guard;
			if (are_mutex(inv, exclusion[1-j])) {
				implicant[j] = i[j]->guard;
				implicant[1-j] = inv;
				return;
			}
		}

		for (int j = 0; j < 2; j++) {
			for (auto k = options[1-j].begin(); k != i[1-j]+1; k++) {
				if (are_mutex(k->guard, i[j]->guard)) {
					implicant[j] = i[j]->guard;
					implicant[1-j] = k->guard;
					return;
				}
			}
		}
	}
	for (int j = 0; j < 2; j++) {
		for (; i[j] != options[j].end(); i[j]++) {
			boolean::cover inv = ~i[j]->guard;
			if (are_mutex(inv, exclusion[1-j])) {
				implicant[j] = i[j]->guard;
				implicant[1-j] = inv;
				return;
			}

			for (auto k = options[1-j].begin(); k != options[1-j].end(); k++) {
				if (are_mutex(k->guard, i[j]->guard)) {
					implicant[j] = i[j]->guard;
					implicant[1-j] = k->guard;
					return;
				}
			}
		}
	}
}

}
