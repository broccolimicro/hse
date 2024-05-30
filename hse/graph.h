#pragma once

#include <common/standard.h>
#include <boolean/cover.h>
#include <ucs/variable.h>
#include <petri/graph.h>

#include "state.h"

namespace hse
{

using petri::iterator;
using petri::parallel;
using petri::choice;
using petri::sequence;

// In Haystack, Handshaking Expansions (HSE) are represented by a collection of
// petri nets in which the transitions are augmented with guards. Before a
// transition may fire, the tokens at the input places must collectively have a
// state encoding that statisfies the condition encoded in the guard.
// 
// For definitions of Production Rule, Guard, Handshaking Expansions, etc, see:
// https://en.wikipedia.org/wiki/Quasi-delay-insensitive_circuit

// A place represents a semicolon in an HSE, which is a partial state
// within the circuit.
struct place : petri::place
{
	place();
	place(boolean::cover predicate);
	~place();

	// inherited from petri::place
	// vector<parallel_group> parallel_groups;

	// This records every state encoding seen by tokens at this place. This and
	// the effective predicate are used to synthesize production rules from this
	// state space.
	boolean::cover predicate;
	// Same as the predicate, but excludes state encodings that enable a guard
	// from any of the outgoing transitions.
	boolean::cover effective;

	// This is used to limit the predicate to only variables we can observe in
	// our process
	boolean::cube mask;

	// if true, more than one output transition from this place can be enabled
	// simultaneously. This means that the hardware needs to make a
	// non-deterministic decision about which one to fire. This is generally done
	// with an arbiter.
	bool arbiter;

	static place merge(int composition, const place &p0, const place &p1);
};

// A transition represents a partial production rule (partial guard -> assignment),
// driving a wire to VDD or GND if the guard is satisfied by the current state
// of the circuit.
struct transition : petri::transition
{
	transition(boolean::cover guard = 1, boolean::cover local_action = 1, boolean::cover remote_action = 1);
	~transition();

	// inherited from petri::transition
	// vector<parallel_group> parallel_groups;

	// The current state must meet the conditions specified in the guard before
	// the transition may fire
	boolean::cover guard;

	// The assignment is broken up in to two pieces.

	// The local_action is the effect this assignment has on the state encodings
	// of the tokens that go through this transition.
	boolean::cover local_action;
	// The remote_action is the effect this assignment has on the state encodings
	// of the tokens that aren't affected by this transition.
	boolean::cover remote_action;

	transition subdivide(int term) const;

	static transition merge(int composition, const transition &t0, const transition &t1);
	static bool mergeable(int composition, const transition &t0, const transition &t1);

	bool is_infeasible() const;
	bool is_vacuous() const;
};

// A graph represents a Handshaking Expansion as a Petri Net.
struct graph : petri::graph<hse::place, hse::transition, petri::token, hse::state>
{
	// See haystack/lib/petri/petri/petri.h for the structure definition.
	typedef petri::graph<hse::place, hse::transition, petri::token, hse::state> super;

	graph();
	~graph();

	// inherited from petri::graph
	// vector<int> node_distances;
	// bool node_distances_ready;
	// bool parallel_nodes_ready;
	//
	// vector<place> places;
	// vector<transition> transitions;
	// vector<arc> arcs[2];
	// vector<state> source, sink;
	// vector<state> reset;

	boolean::cover predicate(petri::iterator i, vector<petri::iterator> *prev = nullptr) const;
	boolean::cover effective(petri::iterator i, vector<petri::iterator> *prev = nullptr) const;
	boolean::cover implicant(petri::iterator i, vector<petri::iterator> *prev = nullptr) const;
	boolean::cover effective_implicant(petri::iterator i, vector<petri::iterator> *prev = nullptr) const;
	bool common_arbiter(petri::iterator a, petri::iterator b) const;

	void post_process(const ucs::variable_set &variables, bool proper_nesting = false, bool aggressive = false);
	void check_variables(const ucs::variable_set &variables);
	vector<petri::iterator> relevant_nodes(vector<petri::iterator> i);
};

}
