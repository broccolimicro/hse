/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"

namespace hse
{

place::place()
{

}

place::~place()
{

}

static place place::parallel_merge(place p0, place p1)
{
	place result;
	result.effective = p0.effective & p1.effective;
	result.predicate = p0.predicate & p1.predicate;
	return result;
}

static place place::conditional_merge(place p0, place p1)
{
	place result;
	result.effective = p0.effective | p1.effective;
	result.predicate = p0.predicate | p1.predicate;
	return result;
}

transition::transition()
{
	type = active;
	action = 1;
}

transition::transition(int type, boolean::cover action)
{
	this->type = type;
	this->action = action;
}

transition::~transition()
{

}

static transition transition::parallel_merge(transition t0, transition t1)
{
	transition result;
	result.action = t0.action & t1.action;
	result.type = t0.type;
	return result;
}

static transition transition::conditional_merge(transition t0, transition t1)
{
	transition result;
	result.action = t0.action | t1.action;
	result.type = t0.type;
	return result;
}

iterator::iterator()
{
	type = 0;
	index = 0;
}

iterator::iterator(int type, int index)
{
	this->type = type;
	this->index = index;
}

iterator::~iterator()
{

}

iterator &iterator::operator=(iterator i)
{
	type = i.type;
	index = i.index;
	return *this;
}

iterator &iterator::operator--()
{
	index--;
	return *this;
}

iterator &iterator::operator++()
{
	index++;
	return *this;
}

iterator &iterator::operator--(int)
{
	index--;
	return *this;
}

iterator &iterator::operator++(int)
{
	index++;
	return *this;
}

iterator &iterator::operator+=(int i)
{
	index += i;
	return *this;
}

iterator &iterator::operator-=(int i)
{
	index -= i;
	return *this;
}

iterator iterator::operator+(int i)
{
	iterator result(*this);
	result.index += i;
	return result;
}

iterator iterator::operator-(int i)
{
	iterator result(*this);
	result.index -= i;
	return result;
}

bool iterator::operator==(iterator i) const
{
	return (type == i.type && index == i.index);
}

bool iterator::operator!=(iterator i) const
{
	return (type != i.type || index != i.index);
}

bool iterator::operator<(iterator i) const
{
	return (type < i.type ||
		   (type == i.type && index < i.index));
}

bool iterator::operator>(iterator i) const
{
	return (type > i.type ||
		   (type == i.type && index > i.index));
}

bool iterator::operator<=(iterator i) const
{
	return (type < i.type ||
		   (type == i.type && index <= i.index));
}

bool iterator::operator>=(iterator i) const
{
	return (type > i.type ||
		   (type == i.type && index >= i.index));
}

arc::arc()
{

}

arc::arc(iterator from, iterator to)
{
	this->from = from;
	this->to = to;
}

arc::~arc()
{

}

graph::graph()
{

}

graph::~graph()
{

}

iterator graph::begin(int type)
{
	return iterator(type, 0);
}

iterator graph::end(int type)
{
	return iterator(type, type == place ? (int)places.size() : (int)transitions.size());
}

iterator graph::create(hse::place p)
{
	places.push_back(p);
	return iterator(place, (int)places.size()-1);
}

iterator graph::create(hse::transition t)
{
	transitions.push_back(t);
	return iterator(transition, (int)transitions.size()-1);
}

vector<iterator> graph::create(vector<hse::place> p)
{
	vector<iterator> result;
	for (int i = 0; i < (int)p.size(); i++)
	{
		places.push_back(p[i]);
		result.push_back(iterator(place, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(vector<hse::transition> t)
{
	vector<iterator> result;
	for (int i = 0; i < (int)t.size(); i++)
	{
		transitions.push_back(t[i]);
		result.push_back(iterator(place, (int)transitions.size()-1));
	}
	return result;
}

vector<iterator> graph::create(hse::place p, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		places.push_back(p);
		result.push_back(iterator(place, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(hse::transition t, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		transitions.push_back(t);
		result.push_back(iterator(place, (int)transitions.size()-1));
	}
	return result;
}

iterator graph::connect(iterator from, iterator to)
{
	arcs.push_back(arc(from, to));
	return to;
}

vector<iterator> graph::connect(iterator from, vector<iterator> to)
{
	for (int i = 0; i < (int)to.size(); i++)
		arcs.push_back(arc(from, to[i]));
	return to;
}

iterator graph::connect(vector<iterator> from, iterator to)
{
	for (int i = 0; i < (int)from.size(); i++)
		arcs.push_back(arc(from[i], to));
	return to;
}

vector<iterator> graph::connect(vector<iterator> from, vector<iterator> to)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			arcs.push_back(arc(from[i], to[i]));
	return to;
}

iterator graph::duplicate(iterator n)
{
	iterator d = n;
	if (n.type == place)
	{
		d.index = (int)places.size();
		places.push_back(places[n.index]);
	}
	else if (n.type == transition)
	{
		d.index = (int)transitions.size();
		transitions.push_back(transitions[n.index]);
	}
	int asize = (int)arcs.size();
	for (int i = 0; i < asize; i++)
	{
		if (arcs[i].from == n)
			arcs.push_back(arc(d, arcs[i].to));
		if (arcs[i].to == n)
			arcs.push_back(arc(arcs[i].from, d));
	}
	return d;
}

vector<iterator> graph::duplicate(vector<iterator> n)
{
	vector<iterator> result;
	result.reserve(n.size());
	for (int i = 0; i < (int)n.size(); i++)
		result.push_back(duplicate(n[i]));
	return result;
}

iterator graph::duplicate_merge(iterator n0, iterator n1)
{
	iterator result;
	if (n0.type == place)
		result = create(hse::place::conditional_merge(places[n0.index], places[n1.index]));
	else if (n0.type == transition)
		result = create(hse::transition::parallel_merge(transitions[n0.index], transitions[n1.index]));

	for (int i = 0; i < (int)arcs.size(); i++)
	{
		if (arcs[i].from == n0 || arcs[i].from == n1)
			arcs.push_back(arc(result, arcs[i].to));
		if (arcs[i].to == n0 || arcs[i].to == n1)
			arcs.push_back(arc(arcs[i].from, result));
	}
	return result;
}

iterator graph::merge(iterator n0, iterator n1)
{
	if (n0.type == place)
		places[n0.index] = hse::place::conditional_merge(places[n0.index], places[n1.index]);
	else if (n0.type == transition)
		transitions[n0.index] = hse::transition::parallel_merge(transitions[n0.index], transitions[n1.index]);

	for (int i = 0; i < (int)arcs.size(); i++)
	{
		if (arcs[i].from == n1)
			arcs[i].from = n0;
		else if (arcs[i].from.index > n1.index)
			arcs[i].from.index--;

		if (arcs[i].to == n1)
			arcs[i].to = n0;
		else if (arcs[i].to.index > n1.index)
			arcs[i].to.index--;
	}

	if (n1.type == place)
		places.erase(places.begin() + n1.index);
	else if (n1.type == transition)
		transitions.erase(transitions.begin() + n1.index);
	return n0;
}

void graph::cut(iterator n)
{
	for (int i = 0; i < (int)arcs.size();)
	{
		if (arcs[i].from == n || arcs[i].to == n)
			arcs.erase(arcs.begin() + i);
		else
		{
			if (arcs[i].from.type == n.type && arcs[i].from.index > n.index)
				arcs[i].from.index--;
			if (arcs[i].to.type == n.type && arcs[i].to.index > n.index)
				arcs[i].to.index--;
			i++;
		}
	}

	if (n.type == place)
		places.erase(places.begin() + n.index);
	else if (n.type == transition)
		transitions.erase(transitions.begin() + n.index);
}

void graph::cut(vector<iterator> n, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		cut(n[i]);
}

void graph::pinch(iterator n)
{
	vector<iterator> to;
	vector<iterator> from;

	for (int i = 0; i < (int)arcs.size();)
	{
		arc a = arcs[i];
		if (arcs[i].from.type == n.type && arcs[i].from.index > n.index)
			arcs[i].from.index--;
		if (arcs[i].to.type == n.type && arcs[i].to.index > n.index)
			arcs[i].to.index--;

		if (a.from == n && a.to != n)
		{
			to.push_back(arcs[i].to);
			arcs.erase(arcs.begin() + i);
		}
		else if (a.to == n && a.from != n)
		{
			from.push_back(arcs[i].from);
			arcs.erase(arcs.begin() + i);
		}
		else
			i++;
	}

	if (n.type == place)
		places.erase(places.begin() + n.index);
	else if (n.type == transition)
		transitions.erase(transitions.begin() + n.index);

	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			duplicate_merge(from[i], to[j]);

	cut(from);
	cut(to);
}

void graph::pinch(vector<iterator> n, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		pinch(n[i]);
}

vector<iterator> graph::next(iterator n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n)
			result.push_back(arcs[i].to);
	return result;
}

vector<iterator> graph::next(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].from) != n.end())
			result.push_back(arcs[i].to);
	return result;
}

vector<iterator> graph::prev(iterator n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].to == n)
			result.push_back(arcs[i].from);
	return result;
}

vector<iterator> graph::prev(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].to) != n.end())
			result.push_back(arcs[i].from);
	return result;
}

vector<int> graph::outgoing(iterator n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n)
			result.push_back(i);
	return result;
}

vector<int> graph::outgoing(vector<iterator> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (find(n.begin(), n.end(), arcs[i].from) != n.end())
			result.push_back(i);
	return result;
}

vector<int> graph::incoming(iterator n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].to == n)
			result.push_back(i);
	return result;
}

vector<int> graph::incoming(vector<iterator> n)
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

bool graph::is_floating(iterator n)
{
	for (int i = 0; i < (int)arcs.size(); i++)
		if (arcs[i].from == n || arcs[i].to == n)
			return false;
	return true;
}
}
