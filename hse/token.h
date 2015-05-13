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
	~token();

	int index;
	boolean::cube state;
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
};



}

#endif
