#pragma once

#include <common/standard.h>
#include <boolean/cube.h>
#include <boolean/cover.h>
#include <petri/state.h>
#include <petri/graph.h>
#include <ucs/variable.h>

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

	vector<term_index> history;
	
	// The intersection of all of the terms of the guard of this transition which
	// the current state passed. This is recorded by boolean::passes_guard(),
	// then ANDed into the current state to fill in missing information.
	boolean::cube guard_action;

	// The effective guard of this enabled transition. The definition of
	// "effective guard" is a bit lengthy, see simulator.cpp for a thorough
	// discussion.
	boolean::cover guard;
	
	// The set of assignments up to and including the last non-vacuous assignment
	// preceding this enabled transition. See the todo in simulator.cpp
	// boolean::cover sequence;

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
	token(int index);
	token(int index, boolean::cover sequence);
	~token();

	// The current place this token resides at.
	// inherited from petri::token
	// int index

	// Contains the previous assignments experienced by the input tokens.
	// See the todo in simulator.cpp
	// boolean::cover sequence;
	
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

