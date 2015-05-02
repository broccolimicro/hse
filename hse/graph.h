/*
 * graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <boolean/cover.h>

#include "node.h"

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{
struct graph
{
	graph();
	~graph();

	vector<place> places;
	vector<transition> transitions;
	vector<arc> arcs[2];
	vector<iterator> source, sink;
	boolean::cube reset;

	vector<half_synchronization> synchronizations;
	vector<bool> reach;
	vector<synchronization_region> regions;

	iterator begin(int type);
	iterator end(int type);
	iterator begin_arc(int type);
	iterator end_arc(int type);

	iterator create(place p);
	iterator create(transition t);
	iterator create(int n);
	vector<iterator> create(vector<place> p);
	vector<iterator> create(vector<transition> t);
	vector<iterator> create(place p, int num);
	vector<iterator> create(transition t, int num);
	vector<iterator> create(int n, int num);
	iterator copy(iterator i);
	vector<iterator> copy(iterator i, int num);
	vector<iterator> copy(vector<iterator> i, int num = 1);
	iterator copy_combine(int relation, iterator i0, iterator i1);
	iterator combine(int relation, iterator i0, iterator i1);

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

	iterator insert(iterator a, place n);
	iterator insert(iterator a, transition n);
	iterator insert(iterator a, int n);
	iterator insert_alongside(iterator from, iterator to, place n);
	iterator insert_alongside(iterator from, iterator to, transition n);
	iterator insert_alongside(iterator from, iterator to, int n);
	iterator insert_before(iterator to, place n);
	iterator insert_before(iterator to, transition n);
	iterator insert_before(iterator to, int n);
	iterator insert_after(iterator from, place n);
	iterator insert_after(iterator from, transition n);
	iterator insert_after(iterator from, int n);

	pair<vector<iterator>, vector<iterator> > cut(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);
	void cut(vector<iterator> n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL, bool rsorted = false);

	iterator duplicate(int relation, iterator i, bool add = true);
	vector<iterator> duplicate(int relation, iterator i, int num, bool add = true);
	vector<iterator> duplicate(int relation, vector<iterator> i, int num = 1, bool interleaved = false, bool add = true);

	void pinch(iterator n, vector<iterator> *i0 = NULL, vector<iterator> *i1 = NULL);

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
		if (i.type == place::type)
			return places[i.index];
		else
			return transitions[i.index];
	}

	template <class node>
	node &at(iterator i)
	{
		if (i.type == place::type)
			return places[i.index];
		else
			return transitions[i.index];
	}

	map<iterator, iterator> merge(const graph &g);
	map<iterator, iterator> sequence(const graph &g);

	void compact(bool proper_nesting = false);
	void reachability();
	bool is_reachable(iterator from, iterator to);

	// Generated through syntax
	void synchronize();
	void petrify();

	// Generated through simulation
	void elaborate();
	graph to_state_graph();
	graph to_petri_net();
};

}

#endif
