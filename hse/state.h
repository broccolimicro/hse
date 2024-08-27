#pragma once

#include <common/standard.h>
#include <boolean/cube.h>
#include <boolean/cover.h>
#include <petri/state.h>
#include <petri/graph.h>
#include <ucs/variable.h>

#include <bit>

namespace hse
{

struct graph;

// This points to the cube 'term' in the action of transition 'index' in a graph.
struct term_index
{
	term_index();
	term_index(int index);
	term_index(int index, int term);
	~term_index();

	// graph::transitions[index].local_action.cubes[term]
	int index;
	int term;

	void hash(hasher &hash) const;

	string to_string(const graph &g, const ucs::variable_set &v);
	petri::iterator iter();
};

bool operator<(term_index i, term_index j);
bool operator>(term_index i, term_index j);
bool operator<=(term_index i, term_index j);
bool operator>=(term_index i, term_index j);
bool operator==(term_index i, term_index j);
bool operator!=(term_index i, term_index j);

// This stores all the information necessary to fire an enabled transition: the local
// and remote tokens that enable it, and the total state of those tokens.
struct enabled_transition : petri::enabled_transition
{
	enabled_transition();
	enabled_transition(int index);
	enabled_transition(int index, int term);
	~enabled_transition();

	// inherited from petri::enabled_transition
	// int index;
	// vector<int> tokens;

	// An enabled transition creates tokens on its output before we ever fire it.
	// If the transition is vacuous, these tokens can then be used to enable the
	// next non-vacuous transition. This vector keeps track of the index of each
	// of those output tokens.
	vector<int> output_marking;

	// An enabled transition's history keeps track of every transition that fired
	// between when this transition was enabled and when it fires. This allows us
	// to determine whether this transition was stable and non-interfering when
	// we finally decide to let the event fire.
	vector<term_index> history;
	
	// The intersection of all of the terms of the guard of this transition which
	// the current state passed. This is recorded by boolean::passes_guard(),
	// then ANDed into the current state to fill in missing information.
	boolean::cube guard_action;

	// The effective guard of this enabled transition. The definition of
	// "effective guard" is a bit lengthy, see simulator.cpp for a thorough
	// discussion.
	boolean::cover guard;

	// The collection of all the guards through vacuous transitions leading to
	// this transition.
	boolean::cover depend;

	// The collection of all assumptions through vacuous transitions leading to
	// this transition.
	boolean::cover assume;

	// The set of assignments up to and including the last non-vacuous assignment
	// preceding this enabled transition.
	boolean::cover sequence;

	// An enabled transition is vacuous if the assignment would leave the current
	// state encoding unaffected.
	bool vacuous;

	// An enabled transition becomes unstable when the current state no longer
	// passes the guard.
	bool stable;

	string to_string(const graph &g, const ucs::variable_set &v);
};

bool operator<(enabled_transition i, enabled_transition j);
bool operator>(enabled_transition i, enabled_transition j);
bool operator<=(enabled_transition i, enabled_transition j);
bool operator>=(enabled_transition i, enabled_transition j);
bool operator==(enabled_transition i, enabled_transition j);
bool operator!=(enabled_transition i, enabled_transition j);

// Tokens are like program counters, marking the position of the current state
// of execution. A token may exist at any place in the HSE as long as it is not
// possible for more than one token to exist in that place.
struct token : petri::token
{
	token();
	//token(const hse::token &t);
	//token(petri::token t, boolean::cover guard, boolean::cover sequence);
	token(petri::token t);
	token(int index, boolean::cover assume, boolean::cover guard, boolean::cover sequence, int cause=-1);
	~token();

	// The current place this token resides at.
	// inherited from petri::token
	// int index

	// Contains the previous guards not acknowledged by a non-vacuous transition.
	boolean::cover guard;

	// Contains the previous assumptions not implemented by a non-vacuous transition.
	boolean::cover assume;

	// Contains the previous assignments experienced by the input tokens.
	boolean::cover sequence;

	// If this token is an extension of the base
	// state through a vacuous transition, which
	// preloaded transition caused that extension?
	// index into simulator::enabled::preload
	int cause;
	
	string to_string();
};

// A state is a collection of tokens marking where we are executing in the HSE,
// and a set of variable assignments encoded as a minterm. The simulator
// provides the infrastracture to walk through the state space one transition
// at a time using this structure.
struct state : petri::state<petri::token>
{
	state();
	state(vector<petri::token> tokens, boolean::cube encodings);
	state(vector<hse::token> tokens, boolean::cube encodings);
	~state();

	// The tokens marking our location in the HSE
	// inherited from petri::state
	// vector<token> tokens;

	// The current value assiged to each variable.
	boolean::cube encodings;

	void hash(hasher &hash) const;

	static state merge(const state &s0, const state &s1);
	static state collapse(int index, const state &s);
	state convert(map<petri::iterator, vector<petri::iterator> > translate) const;
	bool is_subset_of(const state &s);
	string to_string(const ucs::variable_set &variables);
};

ostream &operator<<(ostream &os, state s);

bool operator<(state s1, state s2);
bool operator>(state s1, state s2);
bool operator<=(state s1, state s2);
bool operator>=(state s1, state s2);
bool operator==(state s1, state s2);
bool operator!=(state s1, state s2);

}

namespace std {

template<> struct hash<hse::state> {
	std::size_t operator()(const hse::state& s) const noexcept {
		static std::hash<int> h;
		size_t result = 0;
		for (int i = 0; i < (int)s.tokens.size(); i++) {
			size_t value = h(s.tokens[i].index);
			int r = i%32;
			result = result ^ ((value << r) | (value >> (32-r)));
		}
		return result;
	}
};

}
