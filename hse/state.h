/*
 * state.h
 *
 *  Created on: Jun 23, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>
#include <boolean/cover.h>
#include <petri/state.h>
#include <petri/graph.h>
#include <ucs/variable.h>

#ifndef hse_state_h
#define hse_state_h

namespace hse
{

struct graph;

/* This points to the cube 'term' in the action of transition 'index' in a graph.
 * i.e. g.transitions[index].action.cubes[term]
 */
struct term_index : petri::term_index
{
	term_index();
	term_index(int index, int term);
	~term_index();

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

/* This stores all the information necessary to fire an enabled transition: the local
 * and remote tokens that enable it, and the total state of those tokens.
 */
struct enabled_transition : petri::enabled_transition<term_index>
{
	enabled_transition();
	enabled_transition(int index);
	~enabled_transition();

	vector<term_index> history;
	boolean::cube local_action;
	boolean::cube remote_action;
	boolean::cube guard_action;
	boolean::cover guard;
	bool vacuous;
	bool stable;
};

bool operator<(enabled_transition i, enabled_transition j);
bool operator>(enabled_transition i, enabled_transition j);
bool operator<=(enabled_transition i, enabled_transition j);
bool operator>=(enabled_transition i, enabled_transition j);
bool operator==(enabled_transition i, enabled_transition j);
bool operator!=(enabled_transition i, enabled_transition j);

struct enabled_environment : petri::enabled_transition<term_index>
{
	enabled_environment();
	enabled_environment(int index);
	~enabled_environment();

	boolean::cover guard;
};

struct reset_token;

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct local_token : petri::token
{
	local_token();
	local_token(int index, bool remotable);
	local_token(int index, boolean::cover guard, vector<enabled_transition> prev, bool remotable);
	local_token(const reset_token &t);
	~local_token();

	boolean::cover guard;
	vector<enabled_transition> prev;
	bool remotable;

	local_token &operator=(const reset_token &t);
	string to_string();
};

/* A remote token is a token that does not maintain its own state but acts only as an environment for a local token.
 */
struct remote_token : petri::token
{
	remote_token();
	remote_token(int index, boolean::cover guard);
	remote_token(const reset_token &t);
	~remote_token();

	boolean::cover guard;

	remote_token &operator=(const reset_token &t);
	string to_string();
};

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct reset_token : petri::token
{
	reset_token();
	reset_token(int index, bool remotable);
	reset_token(const remote_token &t);
	reset_token(const local_token &t);
	~reset_token();

	bool remotable;

	reset_token &operator=(const remote_token &t);
	reset_token &operator=(const local_token &t);
	string to_string(const ucs::variable_set &variables);
};

ostream &operator<<(ostream &os, vector<reset_token> t);

struct state : petri::state<reset_token>
{
	state();
	state(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings);
	state(vector<local_token> tokens, deque<enabled_environment> environment, boolean::cover encodings);
	~state();

	vector<term_index> environment;
	boolean::cover encodings;

	void hash(hasher &hash) const;

	static state merge(int composition, const state &s0, const state &s1);
	static state collapse(int composition, int index, const state &s);
	state convert(map<petri::iterator, petri::iterator> translate) const;
	bool is_subset_of(const state &s);
	string to_string(const ucs::variable_set &variables);
};

bool operator<(state s1, state s2);
bool operator>(state s1, state s2);
bool operator<=(state s1, state s2);
bool operator>=(state s1, state s2);
bool operator==(state s1, state s2);
bool operator!=(state s1, state s2);

}

#endif
