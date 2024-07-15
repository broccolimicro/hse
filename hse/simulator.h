#pragma once

#include <common/standard.h>
#include <ucs/variable.h>
#include <petri/state.h>
#include <petri/simulator.h>
#include "graph.h"
#include "state.h"

namespace hse
{

// An instability occurs when a transition is enabled, then disabled before
// firing. This creates a glitch on the output node that could kick the state
// machine out of a valid state.
// the enabled_transition represents the enabled transition that was then disabled.
struct instability : enabled_transition
{
	instability();
	instability(const enabled_transition &cause);
	~instability();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

// An instability occurs when the pull up and the pull down network of a gate
// are both enabled at the same time. This creates a short across the gate that
// could lead to circuit damage.
// the pair<term_index, term_index> represents the two interfering transitions.
struct interference : pair<term_index, term_index>
{
	interference();
	interference(const term_index &first, const term_index &second);
	~interference();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

// A mutex error occurs when multiple output transitions of a non-arbiter place
// become enabled at the same time. In this case, the user did not intend for
// the circuit to make a non-deterministic choice, but we find ourselves stuck
// with one because of an error in the rest of the design.
// the pair<enabled_transition, enabled_transition> represents the two enabled
// transitions that violate this mutex constraint.
struct mutex : pair<enabled_transition, enabled_transition>
{
	mutex();
	mutex(const enabled_transition &first, const enabled_transition &second);
	~mutex();

	string to_string(const hse::graph &g, const ucs::variable_set &v);
};

// A deadlock occurs when we reach a state with no enabled transitions. This
// means that the simulator has no way of making forward progress. Given a
// latency of picoseconds on most of these transitions, a circuit specification
// that terminates wouldn't be particularly useful.
// the state is the state we found with no enabled transitions.
struct deadlock : state
{
	deadlock();
	deadlock(const state &s);
	deadlock(vector<token> tokens, boolean::cube encodings);
	~deadlock();

	string to_string(const ucs::variable_set &v);
};

// This keeps track of a single simulation of a set of HSE and makes it easy to
// control that simulation either through an interactive interface or
// programmatically.
struct simulator
{
	// TODO(edward.bingham) this may no longer be necessary
	typedef petri::simulator<hse::place, hse::transition, petri::token, hse::state> super;

	simulator();
	simulator(graph *base, const ucs::variable_set *variables, state initial);
	~simulator();

	// This simulator is also used for elaboration, so a lot of the errors we
	// encounter may be encountered multiple times as the elaborator visits
	// different versions of the same state. This record errors of different
	// types to be deduplicated and displayed at the end of elaboration.
	vector<instability> instability_errors;
	vector<interference> interference_errors;
	vector<mutex> mutex_errors;

	// This records the set of transitions in the order they were fired.
	// Currently it is used to help with debugging (an instability happened and
	// here is the list of transitions leading up to it).

	// However, there is also a question about the definition of instability.
	// Technically, instability is defined with respect to the production rules,
	// which we do not have yet. We only have partial production rules that make
	// up the transitions in the HSE graph. Previous version have used the firing
	// history to try to guess at further terms that might be found in the
	// production rule to identify unstable variables sooner in the flow. This
	// functionality has currently been commented out as a result of a refactor
	// and rework of the underlying engine. It is unclear whether this
	// functionality needs to be explored further or whether doing so would be
	// incorrect.

	// See todo in simulator.cpp
	list<pair<boolean::cube, term_index> > history;

	// Remember these pointers so that we do not have to include them as inputs
	// to every simulation step. graph is the HSE we are simulating, and
	// variables keeps a mapping from variable name to index in a minterm.
	graph *base;
	const ucs::variable_set *variables;

	// The encoding is a minterm that records our knowledge about the current
	// value of each variable in the circuit.

	// Each variable can be one of four possible values:
	// '0' - GND
	// '1' - VDD
	// '-' - unknown (either 0 or 1)
	// 'X' - unstable or interfering (neither 0 nor 1) 

	// Unfortunately, a single variable might exist in multiple isochronic
	// domains. This means that the wire represented by the variable is long
	// enough that a transition on one end of the wire takes a significant amount
	// of time to propagate to the other end of the wire. Each end of that wire
	// is considered to be an isochronic domain, a section in which the
	// transition is assumed to propogate across atomically relative to
	// other events.

	// So when a transition happens on one end of the wire, we don't actually
	// know when it will arrive at the other end. This means that we lose
	// knowledge about the value at the other end of the wire, it becomes '-' in the encoding.

	// Therefore, the global encoding records not just our knowledge of the
	// current value of each wire, but records what all isochronic domains of
	// each variable will be given enough time. This means that the global
	// encoding does not lose knowledge about the value of any variable. Instead
	// the transition is assumed to happen atomically across all isochronic
	// domains.

	// This is used to determine if there is a transition in one isochronic
	// domain that interferes with a transition in another.
	
	// See haystack/lib/boolean/boolean/{cube.h, cube.cpp} for more
	// details about the minterm representation.
	boolean::cube encoding;
	boolean::cube global;

	// These are the tokens that mark the current state. Effectively, these are
	// the program counters. While Petri Nets technically allow more than one
	// token on a single place, our constrained adaptation of Petri Nets as a
	// representation of HSE does not.
	// See haystack/lib/hse/hse/state.h for the definition of token.
	vector<token> tokens;

	// These transitions could be selected to fire next. An enabled transition is
	// one in which all of the gates on the pull up or pull down network are
	// satisfied, and there is a connection from the source to the output through
	// the network of transistors. Because it has not yet fired, the output
	// hasn't passed the threshold voltage yet, signalling its complete
	// transition to the new value.
	vector<enabled_transition> loaded;

	// The ready array makes it easy to tell the elaborator and the user exactly
	// how many enabled transitions there are and makes it easy to select them
	// for firing.
	// ready[i].first indexes into simulator::loaded[]
	// ready[i].second indexes into base->transitions[loaded[ready[i].first].index].local_action.cubes[] which is the cube in the action of that transition
	vector<pair<int, int> > ready;

	int enabled(bool sorted = false);
	enabled_transition fire(int index);

	void merge_errors(const simulator &sim);
	state get_state();
	state get_key();
	vector<pair<int, int> > get_choices();
};

}

