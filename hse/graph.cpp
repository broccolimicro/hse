/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"
#include <common/message.h>

namespace hse
{
graph::graph()
{
	reset = 1;
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
	if (from.type == place && to.type == place)
	{
		iterator mid = create(hse::transition());
		arcs[from.type].push_back(arc(from, mid));
		arcs[mid.type].push_back(arc(mid, to));
	}
	else if (from.type == transition && to.type == transition)
	{
		iterator mid = create(hse::place());
		arcs[from.type].push_back(arc(from, mid));
		arcs[mid.type].push_back(arc(mid, to));
	}
	else
		arcs[from.type].push_back(arc(from, to));
	return to;
}

vector<iterator> graph::connect(iterator from, vector<iterator> to)
{
	for (int i = 0; i < (int)to.size(); i++)
		connect(from, to[i]);
	return to;
}

iterator graph::connect(vector<iterator> from, iterator to)
{
	for (int i = 0; i < (int)from.size(); i++)
		connect(from[i], to);
	return to;
}

vector<iterator> graph::connect(vector<iterator> from, vector<iterator> to)
{
	for (int i = 0; i < (int)from.size(); i++)
		for (int j = 0; j < (int)to.size(); j++)
			connect(from[i], to[i]);
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

pair<vector<iterator>, vector<iterator> > graph::cut(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > result;
	for (int i = (int)arcs[n.type].size()-1; i >= 0; i--)
	{
		if (arcs[n.type][i].from.index == n.index)
		{
			result.second.push_back(arcs[n.type][i].to);
			arcs[n.type].erase(arcs[n.type].begin() + i);
		}
		else if (arcs[n.type][i].from.index > n.index)
			arcs[n.type][i].from.index--;
	}
	for (int i = (int)arcs[1-n.type].size()-1; i >= 0; i--)
	{
		if (arcs[1-n.type][i].to.index == n.index)
		{
			result.first.push_back(arcs[1-n.type][i].from);
			arcs[1-n.type].erase(arcs[1-n.type].begin() + i);
		}
		else if (arcs[1-n.type][i].to.index > n.index)
			arcs[1-n.type][i].to.index--;
	}

	for (int i = (int)source.size()-1; i >= 0; i--)
	{
		if (source[i] == n)
			source.erase(source.begin() + i);
		else if (source[i].type == n.type && source[i].index > n.index)
			source[i].index--;
	}
	for (int i = (int)sink.size()-1; i >= 0; i--)
	{
		if (sink[i] == n)
			sink.erase(sink.begin() + i);
		else if (sink[i].type == n.type && sink[i].index > n.index)
			sink[i].index--;
	}

	if (i0 != NULL)
	{
		for (int i = (int)i0->size()-1; i >= 0; i--)
		{
			if (i0->at(i) == n)
				i0->erase(i0->begin() + i);
			else if (i0->at(i).type == n.type && i0->at(i).index > n.index)
				i0->at(i).index--;
		}
	}
	if (i1 != NULL)
	{
		for (int i = (int)i1->size()-1; i >= 0; i--)
		{
			if (i1->at(i) == n)
				i1->erase(i1->begin() + i);
			else if (i1->at(i).type == n.type && i1->at(i).index > n.index)
				i1->at(i).index--;
		}
	}

	if (n.type == place)
		places.erase(places.begin() + n.index);
	else if (n.type == transition)
		transitions.erase(transitions.begin() + n.index);

	return result;
}

void graph::cut(vector<iterator> n, vector<iterator> *i0, vector<iterator> *i1, bool rsorted)
{
	if (!rsorted)
		sort(n.rbegin(), n.rend());
	for (int i = 0; i < (int)n.size(); i++)
		cut(n[i], i0, i1);
}

iterator graph::duplicate(iterator n)
{
	iterator d;
	if (n.type == place)
	{
		d = create(places[n.index]);
		for (int i = (int)arcs[place].size()-1; i >= 0; i--)
			if (arcs[place][i].from == n)
				connect(d, arcs[place][i].to);
		for (int i = (int)arcs[transition].size()-1; i >= 0; i--)
			if (arcs[transition][i].to == n)
				connect(arcs[transition][i].from, d);
	}
	else if (n.type == transition)
	{
		d = create(transitions[n.index]);
		for (int i = (int)arcs[place].size()-1; i >= 0; i--)
			if (arcs[place][i].to == n)
				connect(arcs[place][i].from, d);
		for (int i = (int)arcs[transition].size()-1; i >= 0; i--)
			if (arcs[transition][i].from == n)
				connect(d, arcs[transition][i].to);
	}
	if (find(source.begin(), source.end(), n) != source.end())
		source.push_back(d);
	if (find(sink.begin(), sink.end(), n) != sink.end())
		sink.push_back(d);

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
	else
	{
		internal("hse::graph::duplicate_merge", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}

	for (int i = (int)arcs[result.type].size()-1; i >= 0; i--)
		if (arcs[result.type][i].from == n0 || arcs[result.type][i].from == n1)
			arcs[result.type].push_back(arc(result, arcs[result.type][i].to));
	for (int i = (int)arcs[1-result.type].size()-1; i >= 0; i--)
		if (arcs[1-result.type][i].to == n0 || arcs[1-result.type][i].to == n1)
			arcs[1-result.type].push_back(arc(arcs[1-result.type][i].from, result));

	bool n0_is_source = find(source.begin(), source.end(), n0) != source.end();
	bool n0_is_sink = find(sink.begin(), sink.end(), n0) != sink.end();
	bool n1_is_source = find(source.begin(), source.end(), n1) != source.end();
	bool n1_is_sink = find(sink.begin(), sink.end(), n1) != sink.end();
	if (n0_is_source || n1_is_source)
		source.push_back(result);
	if (n0_is_sink || n1_is_sink)
		sink.push_back(result);
	return result;
}

iterator graph::merge(iterator n0, iterator n1, vector<iterator> *i0, vector<iterator> *i1)
{
	if (n0.type == place && n1.type == place)
		places[n0.index] = hse::place::conditional_merge(places[n0.index], places[n1.index]);
	else if (n0.type == transition && n1.type == transition)
		transitions[n0.index] = hse::transition::parallel_merge(transitions[n0.index], transitions[n1.index]);
	else
	{
		internal("hse::graph::merge", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}

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

	bool n0_is_source = find(source.begin(), source.end(), n0) != source.end();
	bool n0_is_sink = find(sink.begin(), sink.end(), n0) != sink.end();
	bool n1_is_source = find(source.begin(), source.end(), n1) != source.end();
	bool n1_is_sink = find(sink.begin(), sink.end(), n1) != sink.end();
	for (int i = (int)source.size()-1; i >= 0; i--)
	{
		if (source[i] == n1)
			source.erase(source.begin() + i);
		else if (source[i].type == n1.type && source[i].index > n1.index)
			source[i].index--;
	}
	for (int i = (int)sink.size()-1; i >= 0; i--)
	{
		if (sink[i] == n1)
			sink.erase(sink.begin() + i);
		else if (sink[i].type == n1.type && sink[i].index > n1.index)
			sink[i].index--;
	}

	if (n1_is_source && !n0_is_source)
		source.push_back(n0);
	if (n1_is_sink && !n0_is_sink)
		sink.push_back(n0);

	if (i0 != NULL)
	{
		bool has_n0 = false;
		for (int i = (int)i0->size()-1; i >= 0; i--)
		{
			if ((i0->at(i) == n1 || i0->at(i) == n0) && !has_n0)
			{
				i0->at(i) = n0;
				has_n0 = true;
			}
			else if (i0->at(i) == n1 || i0->at(i) == n0)
				i0->erase(i0->begin() + i);
			else if (i0->at(i).type == n1.type && i0->at(i).index > n1.index)
				i0->at(i).index--;
		}
	}
	if (i1 != NULL)
	{
		bool has_n0 = false;
		for (int i = (int)i1->size()-1; i >= 0; i--)
		{
			if ((i1->at(i) == n1 || i1->at(i) == n0) && !has_n0)
			{
				i1->at(i) = n0;
				has_n0 = true;
			}
			else if (i1->at(i) == n1 || i1->at(i) == n0)
				i1->erase(i1->begin() + i);
			else if (i1->at(i).type == n1.type && i1->at(i).index > n1.index)
				i1->at(i).index--;
		}
	}

	if (n1.type == place)
		places.erase(places.begin() + n1.index);
	else if (n1.type == transition)
		transitions.erase(transitions.begin() + n1.index);
	return n0;
}

void graph::merge(vector<iterator> n0, vector<iterator> n1, vector<iterator> *i0, vector<iterator> *i1)
{
	for (int i = 0; i < (int)n0.size(); i++)
		for (int j = 0; j < (int)n1.size(); j++)
			duplicate_merge(n0[i], n1[j]);

	n0.insert(n0.end(), n1.begin(), n1.end());
	cut(n0, i0, i1);
}

void graph::pinch(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > neighbors;
	neighbors = cut(n, i0, i1);
	merge(neighbors.first, neighbors.second, i0, i1);
}

void graph::pinch_forward(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > neighbors;
	neighbors = cut(n, i0, i1);
	sort(neighbors.second.rbegin(), neighbors.second.rend());
	for (int i = 0; i < (int)neighbors.second.size(); i++)
	{
		pair<vector<iterator>, vector<iterator> > next_neighbors;
		next_neighbors = cut(neighbors.second[i], i0, i1);
		connect(next_neighbors.first, neighbors.first);
		connect(neighbors.first, next_neighbors.second);
	}
}

void graph::pinch_backward(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > neighbors;
	neighbors = cut(n, i0, i1);
	sort(neighbors.first.rbegin(), neighbors.first.rend());
	for (int i = 0; i < (int)neighbors.first.size(); i++)
	{
		pair<vector<iterator>, vector<iterator> > prev_neighbors;
		prev_neighbors = cut(neighbors.first[i], i0, i1);
		connect(prev_neighbors.first, neighbors.second);
		connect(neighbors.second, prev_neighbors.second);
	}
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

map<iterator, iterator> graph::merge(const graph &g)
{
	map<iterator, iterator> result;

	places.reserve(places.size() + g.places.size());
	for (int i = 0; i < (int)g.places.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(graph::place, i), iterator(graph::place, (int)places.size())));
		places.push_back(g.places[i]);
	}

	transitions.reserve(transitions.size() + g.transitions.size());
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(graph::transition, i), iterator(graph::transition, (int)transitions.size())));
		transitions.push_back(g.transitions[i]);
	}

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)g.arcs[i].size(); j++)
			arcs[i].push_back(arc(result[g.arcs[i][j].from], result[g.arcs[i][j].to]));

	for (int i = 0; i < (int)g.source.size(); i++)
		source.push_back(result[g.source[i]]);

	for (int i = 0; i < (int)g.sink.size(); i++)
		sink.push_back(result[g.sink[i]]);

	reset &= g.reset;

	return result;
}

map<iterator, iterator> graph::sequence(const graph &g)
{
	map<iterator, iterator> result;

	places.reserve(places.size() + g.places.size());
	for (int i = 0; i < (int)g.places.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(graph::place, i), iterator(graph::place, (int)places.size())));
		places.push_back(g.places[i]);
	}

	transitions.reserve(transitions.size() + g.transitions.size());
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(graph::transition, i), iterator(graph::transition, (int)transitions.size())));
		transitions.push_back(g.transitions[i]);
	}

	for (int i = 0; i < 2; i++)
		for (int j = 0; j < (int)g.arcs[i].size(); j++)
			arcs[i].push_back(arc(result[g.arcs[i][j].from], result[g.arcs[i][j].to]));

	if (sink.size() == 0)
		for (int i = 0; i < (int)g.source.size(); i++)
			source.push_back(result[g.source[i]]);
	else
		for (int i = 0; i < (int)g.source.size(); i++)
			connect(sink, result[g.source[i]]);

	sink.clear();
	for (int i = 0; i < (int)g.sink.size(); i++)
		sink.push_back(result[g.sink[i]]);

	reset &= g.reset;

	return result;
}

void graph::compact(bool proper_nesting)
{
	bool change = true;
	while (change)
	{
		change = false;

		for (iterator i(hse::graph::transition, 0); i < (int)transitions.size(); )
		{
			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			bool affect = false;
			/* A transition will never be enabled if its action is not physically possible or it doesn't
			 * have any input arcs. These transitions may be removed while preserving proper nesting, token flow
			 * stability, non interference, and deadlock freedom.
			 */
			if (transitions[i.index].action == 0 || p.size() == 0)
			{
				if (find(source.begin(), source.end(), i) != source.end())
				{
					source.insert(source.end(), n.begin(), n.end());
					if (transitions[i.index].type == transition::active)
						reset = boolean::transition(reset, transitions[i.index].action);
					else if (transitions[i.index].type == transition::passive)
						reset &= transitions[i.index].action;
				}

				cut(i);
				affect = true;
			}
			/* We can know for sure if a transition is vacuous before we've elaborated the state space if
			 * the transition is the universal cube. These transitions may be pinched while preserving token flow
			 * stability, non interference, and deadlock freedom. However, proper nesting is not necessarily
			 * preserved. We have to take special precautions if we want to preserver proper nesting.
			 */
			if (!affect && transitions[i.index].action == 1)
			{
				if (!proper_nesting)
				{
					pinch(i);
					affect = true;
				}
				else
				{
					vector<iterator> np = next(p);
					vector<iterator> pn = prev(n);

					if (p.size() == 1 && n.size() == 1 && np.size() == 1 && pn.size() == 1)
					{
						pinch(i);
						affect = true;
					}
					else
					{
						vector<iterator> nn = next(n);
						vector<iterator> nnp = next(np);
						vector<iterator> pp = prev(p);
						vector<iterator> ppn = prev(pn);

						if (n.size() == 1 && nn.size() == 1 && nnp.size() == 1 && np.size() == 1)
						{
							pinch_forward(i);
							affect = true;
						}
						else if (p.size() == 1 && pp.size() == 1 && ppn.size() == 1 && pn.size() == 1)
						{
							pinch_backward(i);
							affect = true;
						}
					}
				}
			}

			if (!affect)
				i++;
			else
				change = true;
		}

		for (iterator i(hse::graph::place, 0); i < (int)places.size(); )
		{
			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			bool affect = false;

			/* We know a place will never be marked if it is not in the initial marking and it has no input arcs
			 * Furthermore, if the place is in the initial marking and it has only one output transition, we can
			 * just fire its output transition on reset.
			 */
			if (p.size() == 0)
			{
				vector<iterator>::iterator s = find(source.begin(), source.end(), i);
				if (s != source.end() && n.size() <= 1)
				{
					if (n.size() == 1)
						*s = n.back();

					cut(i);
					affect = true;
				}
				else if (s == source.end())
				{
					cut(i);
					affect = true;
				}
			}
			/* Identify implicit internal choice and make it explicit internal choice.
			 * First find a place with multiple output transitions (a conditional split)
			 */
			if (!affect && n.size() > 1)
			{
				// Then check to see if the types (active or passive) of all the output transitions match
				bool internal_choice = true;
				for (int j = 1; j < (int)n.size() && internal_choice; j++)
					if (transitions[n[j].index].type != transitions[n[0].index].type)
						internal_choice = false;

				if (internal_choice)
				{
					// Next we need to make sure all of their input and output places match
					vector<iterator> nn = next(n[0]);
					vector<iterator> pn = prev(n[0]);

					sort(nn.begin(), nn.end());
					sort(pn.begin(), pn.end());

					for (int j = 1; j < (int)n.size() && internal_choice; j++)
					{
						vector<iterator> jnn = next(n[j]);
						sort(jnn.begin(), jnn.end());

						if (jnn != nn)
							internal_choice = false;

						if (internal_choice)
						{
							vector<iterator> jpn = prev(n[j]);
							sort(jpn.begin(), jpn.end());

							if (jpn != pn)
								internal_choice = false;
						}
					}

					// Now we can do the merge
					if (internal_choice)
					{
						for (int j = 1; j < (int)n.size(); j++)
							transitions[n[0].index] = hse::transition::conditional_merge(transitions[n[0].index], transitions[n[j].index]);

						n.erase(n.begin());
						cut(n);
						affect = true;
					}
				}
			}

			if (!affect)
				i++;
			else
				change = true;
		}
	}
}

}
