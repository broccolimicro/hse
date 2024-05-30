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

	vector<term_index> history;
	boolean::cube guard_action;
	boolean::cover guard;
	boolean::cover sequence;
	bool vacuous;
	bool stable;

	string to_string(const graph &g, const ucs::variable_set &v);
};

bool operator<(enabled_transition i, enabled_transition j);
bool operator>(enabled_transition i, enabled_transition j);
bool operator<=(enabled_transition i, enabled_transition j);
bool operator>=(enabled_transition i, enabled_transition j);
bool operator==(enabled_transition i, enabled_transition j);
bool operator!=(enabled_transition i, enabled_transition j);

// A local token maintains its own local state information and cannot access global state
// except through transformations applied to its local state.
struct token : petri::token
{
	token();
	//token(const hse::token &t);
	//token(petri::token t, boolean::cover guard, boolean::cover sequence);
	token(petri::token t);
	token(int index);
	token(int index, boolean::cover sequence);
	~token();

	boolean::cover sequence;
	
	string to_string();
};

struct state : petri::state<petri::token>
{
	state();
	state(vector<petri::token> tokens, boolean::cube encodings);
	state(vector<hse::token> tokens, boolean::cube encodings);
	~state();

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

