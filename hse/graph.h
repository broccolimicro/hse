/*
 * graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include <common/standard.h>

#ifndef hse_graph_h
#define hse_graph_h

namespace hse
{

struct place
{
	place();
	~place();

	boolean::cover predicate;
	boolean::cover effective;

	static place parallel_merge(place p0, place p1);
	static place conditional_merge(place p0, place p1);
};

struct transition
{
	transition();
	transition(int type, boolean::cover action = 1);
	~transition();

	enum
	{
		passive = 0,
		active = 1
	};

	boolean::cover action;
	int type;

	static transition parallel_merge(transition t0, transition t1);
	static transition conditional_merge(transition t0, transition t1);
};

struct iterator
{
	iterator();
	iterator(int type, int index);
	~iterator();

	int type;
	int index;

	iterator &operator=(iterator i);
	iterator &operator--();
	iterator &operator++();
	iterator &operator--(int);
	iterator &operator++(int);

	iterator &operator+=(int i);
	iterator &operator-=(int i);

	iterator operator+(int i);
	iterator operator-(int i);

	bool operator==(iterator i) const;
	bool operator!=(iterator i) const;
	bool operator<(iterator i) const;
	bool operator>(iterator i) const;
	bool operator<=(iterator i) const;
	bool operator>=(iterator i) const;
};

struct arc
{
	arc();
	arc(iterator from, iterator to);
	~arc();

	iterator from;
	iterator to;
};

struct token
{
	token();
	~token();

	iterator index;
};

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
	vector<arc> arcs;
	vector<token> init;

	iterator begin(int type);
	iterator end(int type);

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
	vector<iterator> push_back(vector<iterator> from, node n, int num = 1)
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

	template <class node>
	iterator insert(int a, node n)
	{
		iterator i = create(n);
		arcs.push_back(arc(i, arcs[a].to));
		arcs[a].to = i;
		return i;
	}

	template <class node>
	iterator insert_alongside(iterator from, iterator to, node n)
	{
		iterator i = create(n);
		connect(from, i);
		connect(i, to);
		return i;
	}

	template <class node>
	iterator insert_before(iterator to, node n)
	{
		iterator i = create(n);
		for (int j = 0; j < arcs.size(); j++)
			if (arcs[j].to == to)
				arcs[j].to = i;
		connect(i, to);
		return i;
	}

	template <class node>
	iterator insert_after(iterator from, node n)
	{
		iterator i = create(n);
		for (int j = 0; j < arcs.size(); j++)
			if (arcs[j].from == from)
				arcs[j].from = i;
		connect(from, i);
		return i;
	}

	iterator duplicate(iterator n);
	vector<iterator> duplicate(vector<iterator> n);

	iterator duplicate_merge(iterator n0, iterator n1);
	iterator merge(iterator n0, iterator n1);

	void cut(iterator n);
	void cut(vector<iterator> n, bool rsorted = false);

	void pinch(iterator n);
	void pinch(vector<iterator> n, bool rsorted);

	vector<iterator> next(iterator n);
	vector<iterator> next(vector<iterator> n);
	vector<iterator> prev(iterator n);
	vector<iterator> prev(vector<iterator> n);
	vector<int> outgoing(iterator n);
	vector<int> outgoing(vector<iterator> n);
	vector<int> incoming(iterator n);
	vector<int> incoming(vector<iterator> n);
	vector<int> next(int a);
	vector<int> next(vector<int> a);
	vector<int> prev(int a);
	vector<int> prev(vector<int> a);

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
};
}

#endif
