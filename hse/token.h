/*
 * token.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>

#include "node.h"

#ifndef hse_token_h
#define hse_token_h

namespace hse
{

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct token
{
	token();
	token(int index, boolean::cube state);
	token(int index, boolean::cube state, vector<term_index> guard);
	~token();

	int index;
	boolean::cube state;
	vector<term_index> guard;
};

bool operator<(token i, token j);
bool operator>(token i, token j);
bool operator<=(token i, token j);
bool operator>=(token i, token j);
bool operator==(token i, token j);
bool operator!=(token i, token j);

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
	vector<term_index> guard;
};

struct instability
{
	instability();
	instability(term_index effect, vector<term_index> cause);
	~instability();

	term_index effect;
	vector<term_index> cause;
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
	deadlock(vector<token> tokens);
	~deadlock();

	vector<token> tokens;
};

bool operator<(deadlock i, deadlock j);
bool operator>(deadlock i, deadlock j);
bool operator<=(deadlock i, deadlock j);
bool operator>=(deadlock i, deadlock j);
bool operator==(deadlock i, deadlock j);
bool operator!=(deadlock i, deadlock j);

}

#endif
