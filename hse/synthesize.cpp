#include "synthesize.h"

#include <common/timer.h>
#include <common/mapping.h>

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

void gate::print(int uid, graph *base) {
	cout << base->nets[uid].to_string() << "-" << endl;
	cout << "\t" << ::to_string(tids[0]) << endl;
	cout << "\timplicant: " << export_expression(implicant[0], *base).to_string() << endl; 
	cout << "\texclusion: " << export_expression(exclusion[0], *base).to_string() << endl; 
	cout << "\tholding: " << export_expression(holding[0], *base).to_string() << endl; 
	cout << "\tassume: " << export_expression(assume[0], *base).to_string() << endl;

 	cout << base->nets[uid].to_string() << "+" << endl;
	cout << "\t" << ::to_string(tids[1]) << endl;
	cout << "\timplicant: " << export_expression(implicant[1], *base).to_string() << endl; 
	cout << "\texclusion: " << export_expression(exclusion[1], *base).to_string() << endl; 
	cout << "\tholding: " << export_expression(holding[1], *base).to_string() << endl; 
	cout << "\tassume: " << export_expression(assume[1], *base).to_string() << endl;
}

gate_set::gate_set() {
	base = nullptr;
}

gate_set::gate_set(graph *base) {
	this->base = base;
}

gate_set::~gate_set() {
}

void gate_set::load(bool senseless) {
	gates.clear();
	gates.resize(base->nets.size());

	for (int i = 0; i < (int)gates.size(); i++) {
		if (not base->reset.empty() and not base->reset[0].encodings.cubes.empty()) {
			gates[i].reset = base->reset[0].encodings.cubes[0].get(i);
		}
	}

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (auto t0i = base->transitions.begin(); t0i != base->transitions.end(); t0i++) {
		petri::iterator t0(petri::transition::type, t0i - base->transitions.begin());
		
		// The state encodings for the implicant set of states of the transition.
		// these are the state encodings for which there is no vacuous transition that would
		// take a token off of any of the places.
		boolean::cover predicate = base->predicate(vector<petri::iterator>(1, t0));
		boolean::cover implicant = predicate & t0i->guard;
		boolean::cover assume = base->arbitration(t0.index) & t0i->assume;

		// The transition actively affects the state of the system
		for (auto term = t0i->local_action.cubes.begin(); term != t0i->local_action.cubes.end(); term++) {
			vector<int> term_vars = term->vars();
			for (auto var = term_vars.begin(); var != term_vars.end(); var++) {
				if (find(base->ghost_nets.begin(), base->ghost_nets.end(), *var) != base->ghost_nets.end()) {
					continue;
				}
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
				gates[*var].assume[val] |= assume;
				gates[*var].tids[val].push_back(t0);
			}
		}
	}

	// TODO(edward.bingham) vacuous transitions and interfering transitions, see
	// filter_vacuous() in conflict identification.
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();
		if (find(base->ghost_nets.begin(), base->ghost_nets.end(), var) != base->ghost_nets.end()) {
			continue;
		}
	
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
					boolean::cover predicate = base->predicate(vector<petri::iterator>(1, *n));
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
	// TODO(edward.bingham) weaken the guard for one variable in such a way that
	// it makes it possible to weaken the guard for another variable more
	// optimally. Effectively you're shoring up one expression to block the
	// possibility of a failure in another expression.
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		gate->weaken_brute_force();
	}
}

void gate_set::build_reset() {
	int reset = base->netIndex("Reset", true);
	int _reset = base->netIndex("_Reset", true);

	// TODO(edward.bingham) There a more complete algorithm for minimal resets on production rules.
	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();
		if (find(base->ghost_nets.begin(), base->ghost_nets.end(), var) != base->ghost_nets.end()) {
			continue;
		}

		if (gate->reset != 2 and gate->is_combinational() and gate->assume[0].is_tautology() and gate->assume[1].is_tautology()) {
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

// TODO State Graph Optimization
//
// The goal of this project is to augment the existing circuit synthesis
// functionality to generate production ready circuits. This includes creating
// the state holding elements, adding precharge networks to clean up analog
// effects that might create glitches, optionally harden the gates against
// transients for robustness, and combine gates into shared gate networks. 
//
// The following steps are guidelines and not hard rules. If you think you
// found a better way to approach the problem, then feel free to chase that
// down. If you need supporting infrastructure anywhere else in the project,
// feel free to add that in. If you need to modify this function definition,
// go for it.
//
// 1. Generate Staticizers
//   a. Generate weak feedback staticizers
//   b. Generate combinational feedback staticizers
//   c. Use state graph information to generate partially combinational staticizers.
//
// 2. Use state graph to add cross coupling to enforce illegal state avoidance
//
// 3. Generate precharge networks
//   a. Determine what data structure changes need to be made to support precharge networks 
//   b. Use state graph information to generate precharge networks
//
// 4. Generate shared gate networks
//   a. Determine what data structure changes need to be made to support shared gate networks
//   b. Use state graph to generate optimal shared gate networks
//
// === Successful completion of project ===
// 
// Your time is yours, what do you want to tackle next?
// Some ideas:
// 1. Expand upon shared gate network functionality to optimize production rule
//    expressions throughout the circuit.
// 2. Build out the user interface in hseenc for working through all of these functions.
//
// Final cleanup:
// 1. Clean up any bugs
// 2. Prepare demo
// 3. Document as needed

void gate_set::build_staticizers() {
}

void gate_set::build_precharge() {
}

void gate_set::harden_state() {
}

void gate_set::build_shared_gates() {
}

void gate_set::save(prs::production_rule_set *out) {
	out->name = base->name;

	mapping m((int)base->nets.size());
	for (int i = 0; i < (int)base->nets.size(); i++) {
		if (not base->nets[i].is_ghost) {
			m.set(i, out->create(prs::net(base->nets[i].name, base->nets[i].region)));
		}
	}

	int gnd = out->netIndex("GND", true);
	int vdd = out->netIndex("Vdd", true);
	out->set_power(vdd, gnd);

	out->require_driven = true;
	out->require_stable = true;
	out->require_noninterfering = true;

	for (auto gate = gates.begin(); gate != gates.end(); gate++) {
		int var = gate - gates.begin();
		int ovar = m.map(var);
		if (find(base->ghost_nets.begin(), base->ghost_nets.end(), var) != base->ghost_nets.end()) {
			continue;
		}

		for (int val = 0; val < 2; val++) {
			if (gate->tids[val].size() > 0) {
				boolean::cover assume = gate->assume[val];
				assume.apply(m.nets);
				boolean::cover implicant = gate->implicant[val];
				implicant.apply(m.nets);
				prs::attributes attr;
				attr.assume = assume;
				out->add(val == 1 ? vdd : gnd, implicant, ovar, val, attr);
			}
		}

		out->nets[ovar].keep = not gate->is_combinational();
	}
}

void synthesize_rules(prs::production_rule_set *out, graph *base, bool senseless, bool report_progress, bool debug) {
	if (report_progress) {
		printf("  %s...", base->name.c_str());
		fflush(stdout);
	}

	Timer tmr;
	gate_set gates(base);
	gates.load(senseless);

	if (debug) {
		gates.print();
	}

	gates.weaken();
	gates.build_reset();
	gates.save(out);

	if (report_progress) {
		float synthDelay = tmr.since();
		int combCount = 0;
		int gateCount = 0;

		for (auto g = gates.gates.begin(); g != gates.gates.end(); g++) {
			if (not g->tids[0].empty() or not g->tids[1].empty()) {
				gateCount++;
				if (g->is_combinational()) {
					combCount++;
				}
			}
		}

		printf("[%s%d COMBINATIONAL AND %d STATE HOLDING GATES%s]\t%gs\n", KGRN, combCount, gateCount-combCount, KNRM, synthDelay);
	}
}

void gate_set::print() {
	for (int uid = 0; uid < (int)gates.size(); uid++) {
		gates[uid].print(uid, base);
		cout << endl;
	}
}

}

