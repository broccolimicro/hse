/*
 * graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cover.h>

#include "node.h"
#include "token.h"

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{
struct graph
{
	graph();
	~graph();

	enum
	{
		place = 0,
		transition = 1
	};

	vector<hse::place> places;
	vector<hse::transition> transitions;
	vector<arc> arcs[2];
	vector<iterator> source, sink;
	boolean::cover reset;

	iterator begin(int type);
	iterator end(int type);
	iterator begin_arc(int type);
	iterator end_arc(int type);

	iterator create(hse::place p);
	iterator create(hse::transition t);
	vector<iterator> create(vector<hse::place> p);
	vector<iterator> create(vector<hse::transition> t);
	vector<iterator> create(hse::place p, int num);
	vector<iterator> create(hse::transition t, int num);

	iterator connect(iterator from, iterator to);
	vector<iterator> connect(iterator from, vector<iterator> to);
	iterator connect(vector<iterator> from, iterator to);
	vector<iterator> connect(vector<iterator> from, vector<iterator> to);

	template <class node>
	iterator push_back(iterator from, node n)
	{
		return connect(from, create(n));
	}

	template <class node>
	iterator push_back(vector<iterator> from, node n)
	{
		return connect(from, create(n));
	}

	template <class node>
	vector<iterator> push_back(iterator from, node n, int num)
	{
		return connect(from, create(n, num));
	}

	template <class node>
	vector<iterator> push_back(vector<iterator> from, node n, int num)
	{
		return connect(from, create(n, num));
	}

	template <class node>
	iterator push_front(iterator to, node n)
	{
		return connect(create(n), to);
	}

	template <class node>
	iterator push_front(vector<iterator> to, node n)
	{
		return connect(create(n), to);
	}

	template <class node>
	vector<iterator> push_front(iterator to, node n, int num)
	{
		return connect(create(n, num), to);
	}

	template <class node>
	vector<iterator> push_front(vector<iterator> to, node n, int num)
	{
		return connect(create(n, num), to);
	}

	iterator insert(iterator a, hse::place n);
	iterator insert(iterator a, hse::transition n);
	iterator insert_alongside(iterator from, iterator to, hse::place n);
	iterator insert_alongside(iterator from, iterator to, hse::transition n);
	iterator insert_before(iterator to, hse::place n);
	iterator insert_before(iterator to, hse::transition n);
	iterator insert_after(iterator from, hse::place n);
	iterator insert_after(iterator from, hse::transition n);

	pair<vector<iterator>, vector<iterator> > cut(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);
	void cut(vector<iterator> n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL, bool rsorted = false);

	iterator duplicate(iterator n);
	vector<iterator> duplicate(vector<iterator> n);

	iterator duplicate_merge(iterator n0, iterator n1);
	iterator merge(iterator n0, iterator n1, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);
	void merge(vector<iterator> n0, vector<iterator> n1, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);

	void pinch(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);
	void pinch_forward(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);
	void pinch_backward(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);

	vector<iterator> next(iterator n);
	vector<iterator> next(vector<iterator> n);
	vector<iterator> prev(iterator n);
	vector<iterator> prev(vector<iterator> n);
	vector<int> next(int type, int n);
	vector<int> next(int type, vector<int> n);
	vector<int> prev(int type, int n);
	vector<int> prev(int type, vector<int> n);

	vector<iterator> out(iterator n);
	vector<iterator> out(vector<iterator> n);
	vector<iterator> in(iterator n);
	vector<iterator> in(vector<iterator> n);
	vector<int> out(int type, int n);
	vector<int> out(int type, vector<int> n);
	vector<int> in(int type, int n);
	vector<int> in(int type, vector<int> n);

	vector<iterator> next_arcs(iterator a);
	vector<iterator> next_arcs(vector<iterator> a);
	vector<iterator> prev_arcs(iterator a);
	vector<iterator> prev_arcs(vector<iterator> a);
	vector<int> next_arcs(int type, int a);
	vector<int> next_arcs(int type, vector<int> a);
	vector<int> prev_arcs(int type, int a);
	vector<int> prev_arcs(int type, vector<int> a);

	bool is_floating(iterator n);

	template <class node>
	node &operator[](iterator i)
	{
		if (i.type == place)
			return places[i.index];
		else
			return transitions[i.index];
	}

	template <class node>
	node &at(iterator i)
	{
		if (i.type == place)
			return places[i.index];
		else
			return transitions[i.index];
	}

	map<iterator, iterator> merge(const graph &g);
	map<iterator, iterator> sequence(const graph &g);

	void compact(bool proper_nesting = false);
};
}

#endif
