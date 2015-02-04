/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"

namespace hse
{
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

iterator graph::begin_arc(int type)
{
	return iterator(type, 0);
}

iterator graph::end_arc(int type)
{
	return iterator(type, (int)arcs[type].size());
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
	arcs[from.type].push_back(arc(from, to));
	return to;
}

vector<iterator> graph::connect(iterator from, vector<iterator> to)
{
	for (int i = 0; i < (int)to.size(); i++)
		arcs[from.type].push_back(arc(from, to[i]));
	return to;
}

iterator graph::connect(vector<iterator> from, iterator to)
{
	for (int i = 0; i < (int)from.size(); i++)
		arcs[from[i].type].push_back(arc(from[i], to));
	return to;
}

vector<iterator> graph::connect(vector<iterator> from, vector<iterator> to)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			arcs[from[i].type].push_back(arc(from[i], to[i]));
	return to;
}

iterator graph::insert(iterator a, hse::place n)
{
	iterator i[2];
	i[place] = create(n);
	i[transition] = create(hse::transition());
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[place];
}

iterator graph::insert(iterator a, hse::transition n)
{
	iterator i[2];
	i[place] = create(hse::place());
	i[transition] = create(n);
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[transition];
}

iterator graph::insert_alongside(iterator from, iterator to, hse::place n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(hse::transition());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(hse::transition());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_alongside(iterator from, iterator to, hse::transition n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(hse::place());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(hse::place());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_before(iterator to, hse::place n)
{
	iterator i[2];
	i[transition] = create(hse::transition());
	i[place] = create(n);
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[place];
}

iterator graph::insert_before(iterator to, hse::transition n)
{
	iterator i[2];
	i[transition] = create(n);
	i[place] = create(hse::place());
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[transition];
}

iterator graph::insert_after(iterator from, hse::place n)
{
	iterator i[2];
	i[transition] = create(hse::transition());
	i[place] = create(n);
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[place];
}


iterator graph::insert_after(iterator from, hse::transition n)
{
	iterator i[2];
	i[transition] = create(n);
	i[place] = create(hse::place());
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[transition];
}

iterator graph::duplicate(iterator n)
{
	iterator d;
	if (n.type == place)
	{
		d = create(places[n.index]);
		for (int i = (int)arcs[place].size(); i >= 0; i--)
			if (arcs[place][i].from == n)
				connect(d, arcs[place][i].to);
		for (int i = (int)arcs[transition].size(); i >= 0; i--)
			if (arcs[transition][i].to == n)
				connect(arcs[transition][i].from, d);
	}
	else if (n.type == transition)
	{
		d = create(transitions[n.index]);
		for (int i = (int)arcs[place].size(); i >= 0; i--)
			if (arcs[place][i].to == n)
				connect(arcs[place][i].from, d);
		for (int i = (int)arcs[transition].size(); i >= 0; i--)
			if (arcs[transition][i].from == n)
				connect(d, arcs[transition][i].to);
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
	if (n0.type == place && n1.type == place)
		result = create(hse::place::conditional_merge(places[n0.index], places[n1.index]));
	else if (n0.type == transition && n1.type == transition)
		result = create(hse::transition::parallel_merge(transitions[n0.index], transitions[n1.index]));
	else // TODO make this an internal failure signal
		return iterator();

	for (int i = (int)arcs[result.type].size(); i >= 0; i--)
		if (arcs[result.type][i].from == n0 || arcs[result.type][i].from == n1)
			arcs[result.type].push_back(arc(result, arcs[result.type][i].to));
	for (int i = (int)arcs[1-result.type].size(); i >= 0; i--)
		if (arcs[1-result.type][i].to == n0 || arcs[1-result.type][i].to == n1)
			arcs[1-result.type].push_back(arc(arcs[1-result.type][i].from, result));
	return result;
}

iterator graph::merge(iterator n0, iterator n1)
{
	if (n0.type == place && n1.type == place)
		places[n0.index] = hse::place::conditional_merge(places[n0.index], places[n1.index]);
	else if (n0.type == transition && n1.type == transition)
		transitions[n0.index] = hse::transition::parallel_merge(transitions[n0.index], transitions[n1.index]);
	else // TODO make this an internal failure signal
		return iterator();

	for (int i = 0; i < (int)arcs[n1.type].size(); i++)
	{
		if (arcs[n1.type][i].from == n1)
			arcs[n1.type][i].from = n0;
		else if (arcs[n1.type][i].from.index > n1.index)
			arcs[n1.type][i].from.index--;
	}
	for (int i = 0; i < (int)arcs[1-n1.type].size(); i++)
	{
		if (arcs[1-n1.type][i].to == n1)
			arcs[1-n1.type][i].to = n0;
		else if (arcs[1-n1.type][i].to.index > n1.index)
			arcs[1-n1.type][i].to.index--;
	}

	if (n1.type == place)
		places.erase(places.begin() + n1.index);
	else if (n1.type == transition)
		transitions.erase(transitions.begin() + n1.index);
	return n0;
}

void graph::cut(iterator n)
{
	for (int i = (int)arcs[n.type].size(); i >= 0; i--)
	{
		if (arcs[n.type][i].from == n)
			arcs[n.type].erase(arcs[n.type].begin() + i);
		else if (arcs[n.type][i].from.index > n.index)
			arcs[n.type][i].from.index--;
	}
	for (int i = (int)arcs[1-n.type].size(); i >= 0; i--)
	{
		if (arcs[1-n.type][i].to == n)
			arcs[1-n.type].erase(arcs[1-n.type].begin() + i);
		else if (arcs[1-n.type][i].to.index > n.index)
			arcs[1-n.type][i].to.index--;
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

	for (int i = (int)arcs[n.type].size(); i >= 0; i--)
	{
		if (arcs[n.type][i].from.index == n.index)
		{
			to.push_back(arcs[n.type][i].to);
			arcs[n.type].erase(arcs[n.type].begin() + i);
		}
		else if (arcs[n.type][i].from.index > n.index)
			arcs[n.type][i].from.index--;
	}
	for (int i = (int)arcs[1-n.type].size(); i >= 0; i--)
	{
		if (arcs[1-n.type][i].to.index == n.index)
		{
			from.push_back(arcs[1-n.type][i].from);
			arcs[1-n.type].erase(arcs[1-n.type].begin() + i);
		}
		else if (arcs[1-n.type][i].to.index > n.index)
			arcs[1-n.type][i].to.index--;
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
	for (int i = 0; i < (int)arcs[n.type].size(); i++)
		if (arcs[n.type][i].from.index == n.index)
			result.push_back(arcs[n.type][i].to);
	return result;
}

vector<iterator> graph::next(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = next(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::prev(iterator n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-n.type].size(); i++)
		if (arcs[1-n.type][i].to.index == n.index)
			result.push_back(arcs[1-n.type][i].from);
	return result;
}

vector<iterator> graph::prev(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = prev(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::next(int type, int n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (arcs[type][i].from.index == n)
			result.push_back(arcs[type][i].to.index);
	return result;
}

vector<int> graph::next(int type, vector<int> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (find(n.begin(), n.end(), arcs[type][i].from.index) != n.end())
			result.push_back(arcs[type][i].to.index);
	return result;
}

vector<int> graph::prev(int type, int n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to.index == n)
			result.push_back(arcs[1-type][i].from.index);
	return result;
}

vector<int> graph::prev(int type, vector<int> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (find(n.begin(), n.end(), arcs[1-type][i].to.index) != n.end())
			result.push_back(arcs[1-type][i].from.index);
	return result;
}

vector<iterator> graph::out(iterator n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[n.type].size(); i++)
		if (arcs[n.type][i].from.index == n.index)
			result.push_back(iterator(n.type, i));
	return result;
}

vector<iterator> graph::out(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = out(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::in(iterator n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-n.type].size(); i++)
		if (arcs[1-n.type][i].to.index == n.index)
			result.push_back(iterator(1-n.type, i));
	return result;
}

vector<iterator> graph::in(vector<iterator> n)
{
	vector<iterator> result;
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = in(n[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::out(int type, int n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (arcs[type][i].from.index == n)
			result.push_back(i);
	return result;
}

vector<int> graph::out(int type, vector<int> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[type].size(); i++)
		if (find(n.begin(), n.end(), arcs[type][i].from.index) != n.end())
			result.push_back(i);
	return result;
}

vector<int> graph::in(int type, int n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to.index == n)
			result.push_back(i);
	return result;
}

vector<int> graph::in(int type, vector<int> n)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (find(n.begin(), n.end(), arcs[1-type][i].to.index) != n.end())
			result.push_back(i);
	return result;
}

vector<iterator> graph::next_arcs(iterator a)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-a.type].size(); i++)
		if (arcs[1-a.type][i].from == arcs[a.type][a.index].to)
			result.push_back(iterator(1-a.type, i));
	return result;
}

vector<iterator> graph::next_arcs(vector<iterator> a)
{
	vector<iterator> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<iterator> temp = next_arcs(a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<iterator> graph::prev_arcs(iterator a)
{
	vector<iterator> result;
	for (int i = 0; i < (int)arcs[1-a.type].size(); i++)
		if (arcs[1-a.type][i].to == arcs[a.type][a.index].from)
			result.push_back(iterator(1-a.type, i));
	return result;
}

vector<iterator> graph::prev_arcs(vector<iterator> a)
{
	vector<iterator> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<iterator> temp = prev_arcs(a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::next_arcs(int type, int a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].from == arcs[type][a].to)
			result.push_back(i);
	return result;
}

vector<int> graph::next_arcs(int type, vector<int> a)
{
	vector<int> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<int> temp = next_arcs(type, a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

vector<int> graph::prev_arcs(int type, int a)
{
	vector<int> result;
	for (int i = 0; i < (int)arcs[1-type].size(); i++)
		if (arcs[1-type][i].to == arcs[type][a].from)
			result.push_back(i);
	return result;
}

vector<int> graph::prev_arcs(int type, vector<int> a)
{
	vector<int> result;
	for (int i = 0; i < (int)a.size(); i++)
	{
		vector<int> temp = prev_arcs(type, a[i]);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

bool graph::is_floating(iterator n)
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)arcs[i].size(); j++)
			if (arcs[i][j].from == n || arcs[i][j].to == n)
				return false;
	return true;
}
}
