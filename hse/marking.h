/*
 * marking.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>

#include "node.h"
#include "token.h"
#include "graph.h"

#ifndef hse_marking_h
#define hse_marking_h

namespace hse
{

struct enabled_transition
{
	enabled_transition();
	enabled_transition(int index);
	~enabled_transition();

	vector<int> tokens;
	boolean::cube state;
	int index;
	int term;
};

struct marking
{
	marking();
	~marking();

	vector<local_token> local;
	vector<remote_token> remote;
	graph *base;

	vector<enabled_transition> enabled(bool sorted = false);
	void fire(enabled_transition t);

	bool is_marked(iterator loc) const;
	void mark(iterator loc, boolean::cube state);
	void mark(vector<iterator> loc, boolean::cube state);

	vector<iterator> to_raw();
};
}

#endif
