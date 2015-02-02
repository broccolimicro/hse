/*
 * graph.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: nbingham
 */

#include <common/text.h>
#include "graph.h"

namespace graph
{
graph::iterator::iterator()
{
	part = 0;
	index = 0;
}

graph::iterator::iterator(int p, int i)
{
	part = p;
	index = i;
}

graph::iterator::~iterator()
{

}

graph::iterator &graph::iterator::operator=(iterator i)
{
	part = i.part;
	index = i.index;
	return *this;
}

graph::iterator &graph::iterator::operator--()
{
	index--;
	return *this;
}

graph::iterator &graph::iterator::operator++()
{
	index++;
	return *this;
}

graph::iterator &graph::iterator::operator--(int)
{
	index--;
	return *this;
}

graph::iterator &graph::iterator::operator++(int)
{
	index++;
	return *this;
}

graph::iterator &graph::iterator::operator+=(int i)
{
	index += i;
	return *this;
}

graph::iterator &graph::iterator::operator-=(int i)
{
	index -= i;
	return *this;
}

graph::iterator graph::iterator::operator+(int i)
{
	graph::iterator result(*this);
	result.index += i;
	return result;
}

graph::iterator graph::iterator::operator-(int i)
{
	graph::iterator result(*this);
	result.index -= i;
	return result;
}

bool graph::iterator::operator==(iterator i) const
{
	return (part == i.part && index == i.index);
}

bool graph::iterator::operator!=(iterator i) const
{
	return (part != i.part || index != i.index);
}

bool graph::iterator::operator<(iterator i) const
{
	return (part < i.part ||
		   (part == i.part && index < i.index));
}

bool graph::iterator::operator>(iterator i) const
{
	return (part > i.part ||
		   (part == i.part && index > i.index));
}

bool graph::iterator::operator<=(iterator i) const
{
	return (part < i.part ||
		   (part == i.part && index <= i.index));
}

bool graph::iterator::operator>=(iterator i) const
{
	return (part > i.part ||
		   (part == i.part && index >= i.index));
}

graph::arc::arc()
{

}

graph::arc::arc(iterator from, iterator to)
{
	this->from = from;
	this->to = to;
}

graph::arc::~arc()
{

}

graph::graph()
{

}

graph::~graph()
{
	for (int i = 0; i < (int)nodes.size(); i++)
	{
		for (int j = 0; j < (int)nodes[i].size(); j++)
			if (nodes[i][j] != NULL)
			{
				delete nodes[i][j];
				nodes[i][j] = NULL;
			}
		nodes[i].clear();
	}
	nodes.clear();
	arcs.clear();
}

int graph::parts()
{
	return (int)nodes.size();
}

graph::iterator graph::begin(int part)
{
	return graph::iterator(part, 0);
}

graph::iterator graph::end(int part)
{
	return graph::iterator(part, (int)nodes[part].size());
}

graph::iterator graph::connect(graph::iterator from, graph::iterator to)
{
	arcs.push_back(arc(from, to));
	return to;
}

vector<graph::iterator> graph::connect(graph::iterator from, vector<graph::iterator> to)
{
	for (int i = 0; i < (int)to.size(); i++)
		arcs.push_back(arc(from, to[i]));
	return to;
}

graph::iterator graph::connect(vector<graph::iterator> from, graph::iterator to)
{
	for (int i = 0; i < (int)from.size(); i++)
		arcs.push_back(arc(from[i], to));
	return to;
}

vector<graph::iterator> graph::connect(vector<graph::iterator> from, vector<graph::iterator> to)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			arcs.push_back(arc(from[i], to[i]));
	return to;
}

void graph::cut(graph::iterator n)
{
	for (int i = 0; i < (int)arcs.size(); i++)
	{
		if (arcs[i].from.part == n.part && arcs[i].from.index > n.index)
			arcs[i].from.index--;
		if (arcs[i].to.part == n.part && arcs[i].to.index > n.index)
			arcs[i].to.index--;
	}

	delete nodes[n.part][n.index];
	nodes[n.part][n.index] = NULL;
	nodes[n.part].erase(nodes[n.part].begin() + n.index);
}

void graph::cut(vector<graph::iterator> n, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		cut(n[i]);
}

void graph::pluck(graph::iterator n)
{
	vector<graph::iterator> from;
	vector<graph::iterator> to;
	for (int i = 0; i < (int)arcs.size(); )
	{
		if (arcs[i].from == n)
			to.push_back(arcs[i].to);
		if (arcs[i].to == n)
			from.push_back(arcs[i].from);

		if (arcs[i].from == n || arcs[i].to == n)
			arcs.erase(arcs.begin() + i);
		else
			i++;
	}

	connect(from, to);
	cut(n);
}

void graph::pluck(vector<graph::iterator> n, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		pluck(n[i]);
}

graph::iterator graph::merge(graph::iterator n0, graph::iterator n1)
{
	for (int i = 0; i < (int)arcs.size(); i++)
	{
		if (arcs[i].from == n1)
			arcs[i].from = n0;
		else if (arcs[i].from.part == n1.part && arcs[i].from.index > n1.index)
			arcs[i].from.index--;

		if (arcs[i].to == n1)
			arcs[i].to = n0;
		else if (arcs[i].to.part == n1.part && arcs[i].to.index > n1.index)
			arcs[i].to.index--;
	}

	delete nodes[n1.part][n1.index];
	nodes[n1.part][n1.index] = NULL;
	nodes[n1.part].erase(nodes[n1.part].begin() + n1.index);
	return n0;
}

graph::iterator graph::merge(vector<graph::iterator> n, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());

	iterator result = n.back();
	n.pop_back();

	for (int i = 0; i < (int)arcs.size(); i++)
	{
		if (find(n.begin(), n.end(), arcs[i].from) != n.end())
			arcs[i].from = result;
		else
			for (int j = 0; j < (int)n.size(); j++)
				if (arcs[i].from.part == n[j].part && arcs[i].from.index > n[j].index)
					arcs[i].from.index--;

		if (find(n.begin(), n.end(), arcs[i].to) != n.end())
			arcs[i].to = result;
		else
			for (int j = 0; j < (int)n.size(); j++)
				if (arcs[i].to.part == n[j].part && arcs[i].to.index > n[j].index)
					arcs[i].to.index--;
	}

	for (int i = 0; i < (int)n.size(); i++)
	{
		delete nodes[n[i].part][n[i].index];
		nodes[n[i].part][n[i].index] = NULL;
		nodes[n[i].part].erase(nodes[n[i].part].begin() + n[i].index);
	}
	return result;
}

vector<graph::iterator> graph::next(graph::iterator n)
{
	vector<graph::iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n)
			result.push_back(arcs[i].to);
	return result;
}

vector<graph::iterator> graph::next(vector<graph::iterator> n)
{
	vector<graph::iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].from) != n.end())
			result.push_back(arcs[i].to);
	return result;
}

vector<graph::iterator> graph::prev(graph::iterator n)
{
	vector<graph::iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].to == n)
			result.push_back(arcs[i].from);
	return result;
}

vector<graph::iterator> graph::prev(vector<graph::iterator> n)
{
	vector<graph::iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].to) != n.end())
			result.push_back(arcs[i].from);
	return result;
}

vector<int> graph::outgoing(graph::iterator n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n)
			result.push_back(i);
	return result;
}

vector<int> graph::outgoing(vector<graph::iterator> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].from) != n.end())
			result.push_back(i);
	return result;
}

vector<int> graph::incoming(graph::iterator n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].to == n)
			result.push_back(i);
	return result;
}

vector<int> graph::incoming(vector<graph::iterator> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].to) != n.end())
			result.push_back(i);
	return result;
}

vector<int> graph::next(int a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == arcs[a].to)
			result.push_back(i);
	return result;
}

vector<int> graph::next(vector<int> a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		for (int j = 0; j < (int)a.size(); j++)
			if (arcs[i].from == arcs[a[j]].to)
			{
				result.push_back(i);
				j = (int)a.size();
			}
	return result;
}

vector<int> graph::prev(int a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].to == arcs[a].from)
			result.push_back(i);
	return result;
}

vector<int> graph::prev(vector<int> a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		for (int j = 0; j < (int)a.size(); j++)
			if (arcs[i].to == arcs[a[j]].from)
			{
				result.push_back(i);
				j = (int)a.size();
			}
	return result;
}

bool graph::is_floating(graph::iterator n)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n || arcs[i].to == n)
			return false;
	return true;
}

}
