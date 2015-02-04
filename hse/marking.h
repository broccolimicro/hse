/*
 * marking.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cube.h>

#include "node.h"

#ifndef hse_marking_h
#define hse_marking_h

namespace hse
{

struct token
{
	token();
	token(int index, boolean::cube state);
	~token();

	int index;
	boolean::cube state;

	bool operator<(token t) const;
};

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

struct graph;

struct marking
{
	marking();
	~marking();

	vector<token> tokens;
	graph *base;

	vector<enabled_transition> enabled(bool sorted = false);
	void fire(enabled_transition t);
};
}

#endif
