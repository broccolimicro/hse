/*
 * token.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>
#include <boolean/variable.h>

#include "node.h"

#ifndef hse_token_h
#define hse_token_h

namespace hse
{

struct graph;
struct reset_token;

/* A remote token is a token that does not maintain its own state but acts only as an environment for a local token.
 */
struct remote_token
{
	remote_token();
	remote_token(int index);
	remote_token(const reset_token &t);
	~remote_token();

	int index;

	remote_token &operator=(const reset_token &t);

	string to_string();
};

bool operator<(remote_token i, remote_token j);
bool operator>(remote_token i, remote_token j);
bool operator<=(remote_token i, remote_token j);
bool operator>=(remote_token i, remote_token j);
bool operator==(remote_token i, remote_token j);
bool operator!=(remote_token i, remote_token j);

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct local_token
{
	local_token();
	local_token(int index, boolean::cube state);
	local_token(int index, boolean::cube state, vector<int> guard, bool remotable);
	local_token(const reset_token &t);
	~local_token();

	int index;
	boolean::cube state;
	vector<int> guard;
	bool remotable;

	local_token &operator=(const reset_token &t);
	string to_string(const boolean::variable_set &variables);
};

bool operator<(local_token i, local_token j);
bool operator>(local_token i, local_token j);
bool operator<=(local_token i, local_token j);
bool operator>=(local_token i, local_token j);
bool operator==(local_token i, local_token j);
bool operator!=(local_token i, local_token j);

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct reset_token
{
	reset_token();
	reset_token(int index, boolean::cube state);
	reset_token(int index, boolean::cube state, bool remotable);
	reset_token(const remote_token &t);
	reset_token(const local_token &t);
	~reset_token();

	int index;
	boolean::cube state;
	bool remotable;

	reset_token &operator=(const remote_token &t);
	reset_token &operator=(const local_token &t);
	string to_string(const boolean::variable_set &variables);
};

bool operator<(reset_token i, reset_token j);
bool operator>(reset_token i, reset_token j);
bool operator<=(reset_token i, reset_token j);
bool operator>=(reset_token i, reset_token j);
bool operator==(reset_token i, reset_token j);
bool operator!=(reset_token i, reset_token j);

/* This stores all the information necessary to fire an enabled transition: the local
 * and remote tokens that enable it, and the total state of those tokens.
 */
struct enabled_transition : term_index
{
	enabled_transition();
	enabled_transition(int index);
	~enabled_transition();

	vector<int> tokens;
	boolean::cube state;
	vector<term_index> history;
	vector<int> guard;
	bool vacuous;
};

struct enabled_environment : term_index
{
	enabled_environment();
	enabled_environment(int index);
	~enabled_environment();

	vector<int> tokens;
};

struct instability
{
	instability();
	instability(term_index effect, vector<term_index> cause);
	~instability();

	term_index effect;
	vector<term_index> cause;

	string to_string(const hse::graph &g, const boolean::variable_set &v);
};

bool operator<(instability i, instability j);
bool operator>(instability i, instability j);
bool operator<=(instability i, instability j);
bool operator>=(instability i, instability j);
bool operator==(instability i, instability j);
bool operator!=(instability i, instability j);

struct interference
{
	interference();
	interference(term_index first, term_index second);
	~interference();

	term_index first;
	term_index second;

	string to_string(const hse::graph &g, const boolean::variable_set &v);
};

bool operator<(interference i, interference j);
bool operator>(interference i, interference j);
bool operator<=(interference i, interference j);
bool operator>=(interference i, interference j);
bool operator==(interference i, interference j);
bool operator!=(interference i, interference j);

struct deadlock
{
	deadlock();
	deadlock(vector<local_token> tokens);
	~deadlock();

	vector<local_token> tokens;

	string to_string(const boolean::variable_set &v);
};

bool operator<(deadlock i, deadlock j);
bool operator>(deadlock i, deadlock j);
bool operator<=(deadlock i, deadlock j);
bool operator>=(deadlock i, deadlock j);
bool operator==(deadlock i, deadlock j);
bool operator!=(deadlock i, deadlock j);

}

#endif
