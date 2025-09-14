#pragma once

#include <common/standard.h>
#include <common/net.h>
#include <boolean/cover.h>
#include <petri/graph.h>

#include "state.h"

namespace hse
{

const string ghost_prefix = "__b";

using petri::iterator;
using petri::parallel;
using petri::choice;
using petri::sequence;

// Handshaking Expansions (HSE) are represented by a collection of
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
	// vector<split_group> groups;

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
	// with an arbiter or synchronizer depending on whether the guards are stable.
	bool arbiter;
	bool synchronizer;

	// See graph::ghost_nets for documentation
	vector<int> ghost_nets;

	static place merge(int composition, const place &p0, const place &p1);
};

ostream &operator<<(ostream &os, const place &p);

// A transition represents a partial production rule (partial guard -> assignment),
// driving a wire to VDD or GND if the guard is satisfied by the current state
// of the circuit.
struct transition : petri::transition
{
	transition(boolean::cover assume=1, boolean::cover guard = 1, boolean::cover local_action = 1, boolean::cover remote_action = 1);
	~transition();

	// inherited from petri::transition
	// vector<split_group> groups;

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

	// TODO(edward.bingham) How do I represent timing assumptions in HSE?
	// What kinds of timing assumptions can be made?
	// 
	// 1. Assume this transition arrives before this other transition arrived or
	//    would have arrived (adversarial path assumption), stable and unstable variants?
	// 2. Assume this transition arrives after this other transition arrived or
	//    would have arrived, stable and unstable variants?
	// 
	// Wait a bounded amount of time after entering a certain state for all
	// potential transitions on a wire to propagate and for that wire to be
	// stable. A given state MIGHT trigger a transition, and we don't have enough
	// information in the state to acknowledge that transition if it were to
	// happen. Therefore, wait for a bounded amount of time after that state for
	// the resulting glitch to stabilize.


	// This expression must evaluate to true against the current state before
	// this transition is allowed to be enabled or fired. If 'assume' is
	// tautilogically false, then all transitions in this process must be
	// disabled before this transition is allowed to be enabled.

	// assumptions must propagate through vacuous transitions like guards.
	boolean::cover assume; 

	// See graph::ghost_nets for documentation
	boolean::cover ghost;

	transition subdivide(int term) const;

	static transition merge(int composition, const transition &t0, const transition &t1);
	static bool mergeable(int composition, const transition &t0, const transition &t1);

	bool is_infeasible() const;
	bool is_vacuous() const;
};

ostream &operator<<(ostream &os, const transition &t);

struct net {
	net();
	net(string name, int region, bool is_ghost=false);
	~net();

	string name;
	int region;

	vector<int> remote;
	bool is_ghost;

	string to_string() const;
};

// A graph represents a Handshaking Expansion as a Petri Net.
struct graph : petri::graph<hse::place, hse::transition, petri::token, hse::state>
{
	// See haystack/lib/petri/petri/petri.h for the structure definition.
	typedef petri::graph<hse::place, hse::transition, petri::token, hse::state> super;

	graph();
	~graph();

	string name;

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

	// In order to efficiently update the state space after inserting a state
	// variable transition, we need to know how state propagates out of selection
	// statements. To do this, a variable will be defined for each branch of each
	// conditional split and an assignment will be added to the first transition
	// on that branch. However, these variables should not be used to determine
	// state conflicts, so we need to be able to hide them before we synthesize.
	// So, we record the set of variables added.
	vector<int> ghost_nets;
	vector<net> nets;

	int netIndex(string name, bool define=false);
	int netIndex(string name) const;
	string netAt(int uid) const;
	int netCount() const;

	using super::create;
	int create(net n = net());

	void connect_remote(int from, int to);
	vector<vector<int> > remote_groups();
	string getNetName(int uid) const;

	hse::transition &at(term_index idx);
	boolean::cube &term(term_index idx);

	using super::merge;
	virtual Mapping<petri::iterator> merge(graph g);

	boolean::cover predicate(petri::region pos) const;
	boolean::cover effective(petri::iterator i, vector<petri::iterator> *prev = nullptr) const;
	boolean::cover implicant(petri::region pos) const;
	boolean::cover effective_implicant(petri::region pos) const;
	boolean::cover filter_vacuous(petri::iterator i, boolean::cover encoding, boolean::cube action) const;
	boolean::cover exclusion(int index) const;
	boolean::cover arbitration(int index) const;
	bool common_arbiter(petri::iterator a, petri::iterator b) const;

	void update_masks();

	void post_process(bool proper_nesting = false, bool aggressive = false, bool annotate=true, bool debug=false);
	void check_variables();
	vector<petri::iterator> relevant_nodes(vector<petri::iterator> i);

	void annotate_conditional_branches();
};

}
