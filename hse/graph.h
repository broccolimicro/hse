/*
 * graph.h
 *
 *  Created on: Jan 30, 2015
 *      Author: nbingham
 */

#include <boolean/cover.h>
#include <vector>

using namespace std;

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{
struct index
{
	index();
	index(int value, bool is_place);
	~index();

	int value;

	bool is_place() const;
	bool is_transition() const;
	string to_string() const;
	int idx() const;

	index &operator=(index i);
	index &operator--();
	index &operator++();
	index &operator--(int);
	index &operator++(int);
};

struct place
{
	boolean::cover predicate;
	boolean::cover effective;
};

struct transition
{
	boolean::cover action;
	bool active;
};

struct token
{
	int at;
	boolean::cube encoding;
};

struct arc
{
	int from;
	int to;
};

struct graph;

template <class token_type>
struct marking
{
	vector<token_type> tokens;
	graph *hse;
};

struct graph
{
	string name;
	vector<place> P;
	vector<transition> T;
	vector<arc> Apt;
	vector<arc> Atp;
	marking<token> M0;
};
}

#endif
