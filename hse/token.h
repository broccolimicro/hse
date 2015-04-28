/*
 * token.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>

#include "node.h"
#include "graph.h"

#ifndef hse_token_h
#define hse_token_h

namespace hse
{

/* A global token is a token that has access to global state information.
 */
struct global_token
{
	global_token();
	~global_token();

	int index;
};

bool operator<(global_token i, global_token j);
bool operator>(global_token i, global_token j);
bool operator<=(global_token i, global_token j);
bool operator>=(global_token i, global_token j);
bool operator==(global_token i, global_token j);
bool operator!=(global_token i, global_token j);

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct local_token
{
	local_token();
	local_token(int index, boolean::cube state);
	~local_token();

	int index;
	boolean::cube state;
};

bool operator<(local_token i, local_token j);
bool operator>(local_token i, local_token j);
bool operator<=(local_token i, local_token j);
bool operator>=(local_token i, local_token j);
bool operator==(local_token i, local_token j);
bool operator!=(local_token i, local_token j);

/* This points to the cube 'term' in the action of transition 'index' in a graph.
 * i.e. g.transitions[index].action.cubes[term]
 */
struct term_index
{
	term_index();
	term_index(int index, int term);
	~term_index();

	int index;
	int term;
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
struct enabled_transition : term_index
{
	enabled_transition();
	enabled_transition(int index);
	~enabled_transition();

	vector<int> local_tokens;
	vector<int> global_tokens;
	boolean::cube state;
};

}

#endif
