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

	vector<place> places;
	vector<transition> transitions;
	vector<arc> arcs[2];
	vector<vector<local_token> > source, sink;

	vector<half_synchronization> synchronizations;
	vector<int> reach;
	vector<synchronization_region> regions;

	vector<pair<iterator, iterator> > parallel_nodes;

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

	vector<iterator> next(iterator n) const;
	vector<iterator> next(vector<iterator> n) const;
	vector<iterator> prev(iterator n) const;
	vector<iterator> prev(vector<iterator> n) const;
	vector<int> next(int type, int n) const;
	vector<int> next(int type, vector<int> n) const;
	vector<int> prev(int type, int n) const;
	vector<int> prev(int type, vector<int> n) const;

	vector<iterator> out(iterator n) const;
	vector<iterator> out(vector<iterator> n) const;
	vector<iterator> in(iterator n) const;
	vector<iterator> in(vector<iterator> n) const;
	vector<int> out(int type, int n) const;
	vector<int> out(int type, vector<int> n) const;
	vector<int> in(int type, int n) const;
	vector<int> in(int type, vector<int> n) const;

	vector<iterator> next_arcs(iterator a) const;
	vector<iterator> next_arcs(vector<iterator> a) const;
	vector<iterator> prev_arcs(iterator a) const;
	vector<iterator> prev_arcs(vector<iterator> a) const;
	vector<int> next_arcs(int type, int a) const;
	vector<int> next_arcs(int type, vector<int> a) const;
	vector<int> prev_arcs(int type, int a) const;
	vector<int> prev_arcs(int type, vector<int> a) const;

	bool is_floating(iterator n) const;

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

	map<iterator, iterator> merge(int relation, const graph &g);
	void wrap();

	vector<vector<iterator> > cycles() const;

	void compact(bool proper_nesting = false);
	void reachability();
	bool is_reachable(iterator from, iterator to);
	bool is_parallel(iterator a, iterator b);

	// Generated through syntax
	void synchronize();
	void petrify();

	// Generated through simulation
	void elaborate(vector<instability> &unstable, vector<interference> &interfering, vector<deadlock> &deadlocks);
	graph to_state_graph(vector<instability> &unstable, vector<interference> &interfering, vector<deadlock> &deadlocks);
	graph to_petri_net();
};

}

#endif
