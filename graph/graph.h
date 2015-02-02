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

namespace graph
{
struct node
{
};

struct graph
{
	struct iterator
	{
		iterator();
		iterator(int p, int i);
		~iterator();

		int part;
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

	graph();
	~graph();

	vector<vector<node*> > nodes;
	vector<arc> arcs;

	int parts();
	iterator begin(int part = 0);
	iterator end(int part = 0);

	template <class node_type>
	iterator create_node(int part = 0)
	{
		if (part >= (int)nodes.size())
			nodes.resize(part+1);

		nodes[part].push_back(new node_type());
		return iterator(part, (int)nodes[part].size()-1);
	}

	template <class node_type>
	vector<iterator> create_nodes(int num, int part = 0)
	{
		if (part >= (int)nodes.size())
			nodes.resize(part+1);

		vector<iterator> result;
		for (int i = 0; i < num; i++)
			result.push_back(create_node<node_type>(part));
		return result;
	}

	iterator connect(iterator from, iterator to);
	vector<iterator> connect(iterator from, vector<iterator> to);
	iterator connect(vector<iterator> from, iterator to);
	vector<iterator> connect(vector<iterator> from, vector<iterator> to);

	template <class node_type>
	iterator push_back(iterator from, int part = 0)
	{
		iterator to = create_node<node_type>(part);
		return connect(from, to);
	}

	template <class node_type>
	iterator push_back(vector<iterator> from, int part = 0)
	{
		iterator to = create_node<node_type>(part);
		return connect(from, to);
	}

	template <class node_type>
	vector<iterator> push_back(iterator from, int num = 1, int part = 0)
	{
		vector<iterator> to = create_nodes<node_type>(num, part);
		return connect(from, to);
	}

	template <class node_type>
	vector<iterator> push_back(vector<iterator> from, int num = 1, int part = 0)
	{
		vector<iterator> to = create_nodes<node_type>(num, part);
		return connect(from, to);
	}

	template <class node_type>
	iterator push_front(iterator to, int part = 0)
	{
		iterator from = create_node<node_type>(part);
		connect(from, to);
		return from;
	}

	template <class node_type>
	iterator push_front(vector<iterator> to, int part = 0)
	{
		iterator from = create_node<node_type>(part);
		connect(from, to);
		return from;
	}

	template <class node_type>
	vector<iterator> push_front(iterator to, int num = 1, int part = 0)
	{
		vector<iterator> from = create_nodes<node_type>(num, part);
		connect(from, to);
		return from;
	}

	template <class node_type>
	vector<iterator> push_front(vector<iterator> to, int num = 1, int part = 0)
	{
		vector<iterator> from = create_nodes<node_type>(num, part);
		connect(from, to);
		return from;
	}

	template <class node_type>
	iterator insert(int a, int part = 0)
	{
		iterator n = create_node<node_type>(part);
		iterator to = arcs[a].to;
		arcs[a].to = n;
		connect(n, to);
		return n;
	}

	template <class node_type>
	iterator insert_alongside(iterator from, iterator to, int part = 0)
	{
		iterator n = create_node<node_type>(part);
		connect(from, n);
		connect(to, n);
		return n;
	}

	template <class node_type>
	iterator insert_before(iterator to, int part = 0)
	{
		iterator n = create_node<node_type>(part);
		for (int i = 0; i < arcs.size(); i++)
			if (arcs[i].to == to)
				arcs[i].to = n;
		connect(n, to);
		return n;
	}

	template <class node_type>
	iterator insert_after(iterator from, int part = 0)
	{
		iterator n = create_node<node_type>(part);
		for (int i = 0; i < arcs.size(); i++)
			if (arcs[i].from == from)
				arcs[i].from = n;
		connect(from, n);
		return n;
	}

	template <class node_type>
	iterator duplicate(iterator n)
	{
		nodes[n.part].push_back(new node_type(at<node_type>(n)));
		iterator d(n.part, (int)nodes[n.part].size()-1);
		int asize = (int)arcs.size();
		for (int i = 0; i < asize; i++)
		{
			if (arcs[i].from == n)
				connect(d, arcs[i].to);
			if (arcs[i].to == n)
				connect(arcs[i].from, d);
		}
		return d;
	}

	template <class node_type>
	vector<iterator> duplicate(vector<iterator> n)
	{
		vector<iterator> result;
		result.reserve(n.size());
		for (int i = 0; i < (int)n.size(); i++)
			result.push_back(duplicate<node_type>(n[i]));
		return result;
	}

	void cut(iterator n);
	void cut(vector<iterator> n, bool rsorted = false);
	void pluck(iterator n);
	void pluck(vector<iterator> n, bool rsorted = false);

	iterator merge(iterator n0, iterator n1);
	iterator merge(vector<iterator> n, bool rsorted = false);

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

	template <class node_type>
	node_type &operator[](iterator i)
	{
		return *((node_type*)nodes[i.part][i.index]);
	}

	template <class node_type>
	node_type &at(iterator i)
	{
		return *((node_type*)nodes[i.part][i.index]);
	}
};

}

#endif
