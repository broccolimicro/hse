/*
 * graph.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "graph.h"
#include "simulator.h"
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
	return iterator(type, type == place::type ? (int)places.size() : (int)transitions.size());
}

iterator graph::begin_arc(int type)
{
	return iterator(type, 0);
}

iterator graph::end_arc(int type)
{
	return iterator(type, (int)arcs[type].size());
}

iterator graph::create(place p)
{
	places.push_back(p);
	return iterator(place::type, (int)places.size()-1);
}

iterator graph::create(transition t)
{
	transitions.push_back(t);
	return iterator(transition::type, (int)transitions.size()-1);
}

iterator graph::create(int n)
{
	if (n == place::type)
		return create(place());
	else if (n == transition::type)
		return create(transition());
	else
		return iterator();
}

vector<iterator> graph::create(vector<place> p)
{
	vector<iterator> result;
	for (int i = 0; i < (int)p.size(); i++)
	{
		places.push_back(p[i]);
		result.push_back(iterator(place::type, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(vector<transition> t)
{
	vector<iterator> result;
	for (int i = 0; i < (int)t.size(); i++)
	{
		transitions.push_back(t[i]);
		result.push_back(iterator(place::type, (int)transitions.size()-1));
	}
	return result;
}

vector<iterator> graph::create(place p, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		places.push_back(p);
		result.push_back(iterator(place::type, (int)places.size()-1));
	}
	return result;
}

vector<iterator> graph::create(transition t, int num)
{
	vector<iterator> result;
	for (int i = 0; i < num; i++)
	{
		transitions.push_back(t);
		result.push_back(iterator(place::type, (int)transitions.size()-1));
	}
	return result;
}

vector<iterator> graph::create(int n, int num)
{
	if (n == place::type)
		return create(place(), num);
	else if (n == transition::type)
		return create(transition(), num);
	else
		return vector<iterator>();
}

iterator graph::copy(iterator i)
{
	if (i.type == place::type)
	{
		places.push_back(places[i.index]);
		return iterator(i.type, places.size()-1);
	}
	else
	{
		transitions.push_back(transitions[i.index]);
		return iterator(i.type, transitions.size()-1);
	}
}

vector<iterator> graph::copy(iterator i, int num)
{
	vector<iterator> result;
	if (i.type == place::type)
		for (int j = 0; j < num; j++)
		{
			places.push_back(places[i.index]);
			result.push_back(iterator(i.type, places.size()-1));
		}
	else
		for (int j = 0; j < num; j++)
		{
			transitions.push_back(transitions[i.index]);
			result.push_back(iterator(i.type, transitions.size()-1));
		}
	return result;
}

vector<iterator> graph::copy(vector<iterator> i, int num)
{
	vector<iterator> result;
	for (int j = 0; j < (int)i.size(); j++)
	{
		vector<iterator> temp = copy(i[j], num);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

iterator graph::copy_combine(int relation, iterator i0, iterator i1)
{
	if (i0.type == place::type && i1.type == place::type)
		return create(hse::merge(relation, places[i0.index], places[i1.index]));
	else if (i0.type == transition::type && i1.type == transition::type)
	{
		if (transitions[i0.index].behavior == transitions[i1.index].behavior)
			return create(hse::merge(relation, transitions[i0.index], transitions[i1.index]));
		else
		{
			internal("hse::graph::copy_combine", "transition behaviors do not match", __FILE__, __LINE__);
			return iterator();
		}
	}
	else
	{
		internal("hse::graph::copy_combine", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}
}

iterator graph::combine(int relation, iterator i0, iterator i1)
{
	if (i0.type == place::type && i1.type == place::type)
	{
		places[i0.index] = hse::merge(relation, places[i0.index], places[i1.index]);
		return i0;
	}
	else if (i0.type == transition::type && i1.type == transition::type)
	{
		if (transitions[i0.index].behavior == transitions[i1.index].behavior)
		{
			transitions[i0.index] = hse::merge(relation, transitions[i0.index], transitions[i1.index]);
			return i0;
		}
		else
		{
			internal("hse::graph::copy_combine", "transition behaviors do not match", __FILE__, __LINE__);
			return iterator();
		}
	}
	else
	{
		internal("hse::graph::copy_combine", "iterator types do not match", __FILE__, __LINE__);
		return iterator();
	}
}

iterator graph::connect(iterator from, iterator to)
{
	if (from.type == place::type && to.type == place::type)
	{
		iterator mid = create(transition());
		arcs[from.type].push_back(arc(from, mid));
		arcs[mid.type].push_back(arc(mid, to));
	}
	else if (from.type == transition::type && to.type == transition::type)
	{
		iterator mid = create(place());
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

iterator graph::insert(iterator a, place n)
{
	iterator i[2];
	i[place::type] = create(n);
	i[transition::type] = create(transition());
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[place::type];
}

iterator graph::insert(iterator a, transition n)
{
	iterator i[2];
	i[place::type] = create(place());
	i[transition::type] = create(n);
	arcs[a.type].push_back(arc(i[a.type], arcs[a.type][a.index].to));
	arcs[1-a.type].push_back(arc(i[1-a.type], i[a.type]));
	arcs[a.type][a.index].to = i[1-a.type];
	return i[transition::type];
}

iterator graph::insert(iterator a, int n)
{
	if (n == place::type)
		return insert(a, place());
	else if (n == transition::type)
		return insert(a, transition());
	else
		return iterator();
}

iterator graph::insert_alongside(iterator from, iterator to, place n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(transition());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(transition());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_alongside(iterator from, iterator to, transition n)
{
	iterator i = create(n);
	if (from.type == i.type)
	{
		iterator j = create(place());
		connect(from, j);
		connect(j, i);
	}
	else
		connect(from, i);

	if (to.type == i.type)
	{
		iterator j = create(place());
		connect(i, j);
		connect(j, to);
	}
	else
		connect(i, to);

	return i;
}

iterator graph::insert_alongside(iterator from, iterator to, int n)
{
	if (n == place::type)
		return insert_alongside(from, to, place());
	else if (n == transition::type)
		return insert_alongside(from, to, transition());
	else
		return iterator();
}

iterator graph::insert_before(iterator to, place n)
{
	iterator i[2];
	i[transition::type] = create(transition());
	i[place::type] = create(n);
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[place::type];
}

iterator graph::insert_before(iterator to, transition n)
{
	iterator i[2];
	i[transition::type] = create(n);
	i[place::type] = create(place());
	for (int j = 0; j < (int)arcs[1-to.type].size(); j++)
		if (arcs[1-to.type][j].to.index == to.index)
			arcs[1-to.type][j].to.index = i[to.type].index;
	connect(i[1-to.type], to);
	connect(i[to.type], i[1-to.type]);
	return i[transition::type];
}

iterator graph::insert_before(iterator to, int n)
{
	if (n == place::type)
		return insert_before(to, place());
	else if (n == transition::type)
		return insert_before(to, transition());
	else
		return iterator();
}

iterator graph::insert_after(iterator from, place n)
{
	iterator i[2];
	i[transition::type] = create(transition());
	i[place::type] = create(n);
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[place::type];
}


iterator graph::insert_after(iterator from, transition n)
{
	iterator i[2];
	i[transition::type] = create(n);
	i[place::type] = create(place());
	for (int j = 0; j < (int)arcs[from.type].size(); j++)
		if (arcs[from.type][j].from.index == from.index)
			arcs[from.type][j].from.index = i[from.type].index;
	connect(from, i[1-from.type]);
	connect(i[1-from.type], i[from.type]);
	return i[transition::type];
}

iterator graph::insert_after(iterator from, int n)
{
	if (n == place::type)
		return insert_after(from, place());
	else if (n == transition::type)
		return insert_after(from, transition());
	else
		return iterator();
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

	if (n.type == place::type)
		places.erase(places.begin() + n.index);
	else if (n.type == transition::type)
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

iterator graph::duplicate(int relation, iterator i)
{
	iterator d = copy(i);
	if (i.type == relation)
	{
		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				connect(d, arcs[i.type][j].to);
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				connect(arcs[1-i.type][j].from, d);
	}
	else
	{
		vector<iterator> x = create(1-i.type, 4);
		vector<iterator> y = create(i.type, 2);

		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				arcs[i.type][j].from = y[1];
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				arcs[1-i.type][j].to = y[0];

		connect(y[0], x[0]);
		connect(y[0], x[1]);
		connect(x[0], i);
		connect(x[1], d);
		connect(i, x[2]);
		connect(d, x[3]);
		connect(x[2], y[1]);
		connect(x[3], y[1]);
	}

	if (find(source.begin(), source.end(), i) != source.end())
		source.push_back(d);
	if (find(sink.begin(), sink.end(), i) != sink.end())
		sink.push_back(d);


	return d;
}

vector<iterator> graph::duplicate(int relation, iterator i, int num)
{
	vector<iterator> d = copy(i, num-1);
	if (i.type == relation)
	{
		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				for (int k = 0; k < (int)d.size(); k++)
					connect(d[k], arcs[i.type][j].to);
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				for (int k = 0; k < (int)d.size(); k++)
					connect(arcs[1-i.type][j].from, d[k]);
	}
	else
	{
		vector<iterator> x = create(1-i.type, 2*(num-1));
		vector<iterator> y = create(i.type, 2);
		vector<iterator> z = create(1-i.type, 2);

		for (int j = (int)arcs[i.type].size()-1; j >= 0; j--)
			if (arcs[i.type][j].from == i)
				arcs[i.type][j].from = y[1];
		for (int j = (int)arcs[1-i.type].size()-1; j >= 0; j--)
			if (arcs[1-i.type][j].to == i)
				arcs[1-i.type][j].to = y[0];

		connect(y[0], z[0]);
		connect(z[0], i);
		connect(i, z[1]);
		connect(z[1], y[1]);

		for (int k = 0; k < (int)d.size(); k++)
		{
			connect(y[0], x[k*2 + 0]);
			connect(x[k*2 + 0], d[k]);
			connect(d[k], x[k*2 + 1]);
			connect(x[k*2 + 1], y[1]);
		}
	}

	if (find(source.begin(), source.end(), i) != source.end())
		source.insert(source.end(), d.begin(), d.end());
	if (find(sink.begin(), sink.end(), i) != sink.end())
		sink.insert(sink.end(), d.begin(), d.end());

	d.push_back(i);

	return d;
}

vector<iterator> graph::duplicate(int relation, vector<iterator> n, int num, bool interleaved)
{
	vector<iterator> result;
	result.reserve(n.size()*num);
	for (int i = 0; i < (int)n.size(); i++)
	{
		vector<iterator> temp = duplicate(relation, n[i], num);
		if (interleaved && i > 0)
			for (int j = 0; j < (int)temp.size(); j++)
				result.insert(result.begin() + j*(i+1) + 1, temp[j]);
		else
			result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}

void graph::pinch(iterator n, vector<iterator> *i0, vector<iterator> *i1)
{
	pair<vector<iterator>, vector<iterator> > neighbors = cut(n, i0, i1);

	vector<iterator> left = duplicate(1-n.type, neighbors.first, neighbors.second.size(), false);
	vector<iterator> right = duplicate(1-n.type, neighbors.second, neighbors.first.size(), true);

	for (int i = 0; i < (int)right.size(); i++)
	{
		combine(right[i].type, left[i], right[i]);

		for (int j = 0; j < (int)arcs[right[i].type].size(); j++)
			if (arcs[right[i].type][j].from == right[i])
				arcs[right[i].type][j].from = left[i];

		for (int j = 0; j < (int)arcs[1-right[i].type].size(); j++)
			if (arcs[1-right[i].type][j].to == right[i])
				arcs[1-right[i].type][j].to = left[i];

		if (find(source.begin(), source.end(), right[i]) != source.end())
			source.push_back(left[i]);
		if (find(sink.begin(), sink.end(), right[i]) != source.end())
			sink.push_back(left[i]);
	}

	cut(right, i0, i1);
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
		result.insert(pair<iterator, iterator>(iterator(place::type, i), iterator(place::type, (int)places.size())));
		places.push_back(g.places[i]);
	}

	transitions.reserve(transitions.size() + g.transitions.size());
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(transition::type, i), iterator(transition::type, (int)transitions.size())));
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
		result.insert(pair<iterator, iterator>(iterator(place::type, i), iterator(place::type, (int)places.size())));
		places.push_back(g.places[i]);
	}

	transitions.reserve(transitions.size() + g.transitions.size());
	for (int i = 0; i < (int)g.transitions.size(); i++)
	{
		result.insert(pair<iterator, iterator>(iterator(transition::type, i), iterator(transition::type, (int)transitions.size())));
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

		for (iterator i(transition::type, 0); i < (int)transitions.size() && !change; )
		{
			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			sort(n.begin(), n.end());
			sort(p.begin(), p.end());

			bool affect = false;
			/* If a transition is in the source list, we need to remove it from the source list
			 * and add its input places. If it doesn't have any input places, then we need to add one.
			 */
			vector<iterator>::iterator s = find(source.begin(), source.end(), i);
			if (!affect && s != source.end())
			{
				if (p.size() == 0)
				{
					p.push_back(create(hse::place::type));
					connect(p, i);
				}
				source.erase(s);
				source.insert(source.end(), p.begin(), p.end());
				affect = true;
			}

			/* If a transition is in the sink list, we need to remove it from the sink list
			 * and add its output places. If it doesn't have any output places, then we need to add one.
			 */
			s = find(sink.begin(), sink.end(), i);
			if (!affect && s != sink.end())
			{
				if (n.size() == 0)
				{
					n.push_back(create(hse::place::type));
					connect(i, n);
				}
				sink.erase(s);
				sink.insert(sink.end(), n.begin(), n.end());
				affect = true;
			}

			/* A transition will never be enabled if its action is not physically possible.
			 * These transitions may be removed while preserving proper nesting, token flow
			 * stability, non interference, and deadlock freedom. At this point, it is not
			 * possible for this transition to be in the source list.
			 */
			if (!affect && transitions[i.index].action == 0)
			{
				cut(i);
				affect = true;
			}

			/* We can know for sure if a transition is vacuous before we've elaborated the state space if
			 * the transition is the universal cube. These transitions may be pinched while preserving token flow,
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

						if ((n.size() == 1 && nn.size() == 1 && nnp.size() == 1 && np.size() == 1) ||
							(p.size() == 1 && pp.size() == 1 && ppn.size() == 1 && pn.size() == 1))
						{
							pinch(i);
							affect = true;
						}
					}
				}
			}

			/* If an active transition is enabled at reset and is guaranteed to fire (there is no choice),
			 * then we can just fire the transition, accumulate its effect in the reset state and move the
			 * source tokens forward.
			 */
			/*sort(source.begin(), source.end());
			if (!affect && transitions[i.index].behavior == transition::active && vector_intersection_size(p, source) == (int)p.size())
			{
				vector<iterator> pn = next(p);
				sort(pn.begin(), pn.end());
				if (unique(pn.begin(), pn.end()) - pn.begin() == 1)
				{
					for (vector<iterator>::iterator j = source.begin(); j != source.end();)
					{
						vector<iterator>::iterator pi = find(p.begin(), p.end(), *j);
						if (pi != p.end())
						{
							j = source.erase(j);
							p.erase(pi);
						}
						else
							j++;
					}
					source.insert(source.end(), n.begin(), n.end());
					reset = boolean::local_transition(reset, transitions[i.index].action);
					affect = true;
				}
			}*/

			if (!affect)
				i++;
			else
				change = true;
		}

		for (iterator i(place::type, 0); i < (int)places.size() && !change; )
		{
			bool i_is_reset = (find(source.begin(), source.end(), i) != source.end());

			vector<iterator> n = next(i);
			vector<iterator> p = prev(i);

			sort(n.begin(), n.end());
			sort(p.begin(), p.end());

			bool affect = false;

			/* We know a place will never be marked if it is not in the initial marking and it has no input arcs.
			 * This means that its output transitions will never fire.
			 */
			if (p.size() == 0 && (!i_is_reset || n.size() == 0))
			{
				cut(n);
				cut(i);
				affect = true;
			}

			/* Check to see if there are any excess places whose existence doesn't affect the behavior of the circuit
			 */
			for (iterator j = i+1; j < (int)places.size(); )
			{
				bool j_is_reset = (find(source.begin(), source.end(), i) != source.end());

				vector<iterator> n2 = next(j);
				vector<iterator> p2 = prev(j);

				sort(n2.begin(), n2.end());
				sort(p2.begin(), p2.end());

				if (n == n2 && p == p2 && i_is_reset == j_is_reset)
				{
					cut(j);
					affect = true;
				}
				else
					j++;
			}

			if (!affect)
				i++;
			else
				change = true;
		}


		vector<iterator> left;
		vector<iterator> right;

		vector<vector<iterator> > n, p;
		vector<vector<pair<vector<iterator>, vector<iterator> > > > nx, px;

		for (iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++)
		{
			n.push_back(next(i));
			p.push_back(prev(i));

			sort(n.back().begin(), n.back().end());
			sort(p.back().begin(), p.back().end());

			nx.push_back(vector<pair<vector<iterator>, vector<iterator> > >());
			px.push_back(vector<pair<vector<iterator>, vector<iterator> > >());
			for (int j = 0; j < (int)n.back().size(); j++)
			{
				nx.back().push_back(pair<vector<iterator>, vector<iterator> >(prev(n.back()[j]), next(n.back()[j])));
				sort(nx.back().back().first.begin(), nx.back().back().first.end());
				sort(nx.back().back().second.begin(), nx.back().back().second.end());
			}
			for (int j = 0; j < (int)p.back().size(); j++)
			{
				px.back().push_back(pair<vector<iterator>, vector<iterator> >(prev(p.back()[j]), next(p.back()[j])));
				sort(px.back().back().first.begin(), px.back().back().first.end());
				sort(px.back().back().second.begin(), px.back().back().second.end());
			}

			for (iterator j = i-1; j >= 0 && !change; j--)
			{
				if (transitions[j.index].behavior == transitions[i.index].behavior)
				{
					/* Find internally conditioned transitions. Transitions are internally conditioned if they are the same type
					 * share all of the same input and output places.
					 */
					if (n[j.index] == n[i.index] && p[j.index] == p[i.index])
					{
						transitions[j.index] = hse::merge(choice, transitions[i.index], transitions[j.index]);
						cut(i);
						change = true;
					}

					/* Find internally parallel transitions. A pair of transitions A and B are internally parallel if
					 * they are the same type, have disjoint sets of input and output places that share a single input
					 * or output transition and have no output or input transitions other than A or B.
					 */
					else if (vector_intersection_size(n[i.index], n[j.index]) == 0 && vector_intersection_size(p[i.index], p[j.index]) == 0 && nx[i.index] == nx[j.index] && px[i.index] == px[j.index])
					{
						transitions[j.index] = hse::merge(parallel, transitions[i.index], transitions[j.index]);
						vector<iterator> tocut;
						tocut.push_back(i);
						tocut.insert(tocut.end(), n[i.index].begin(), n[i.index].end());
						tocut.insert(tocut.end(), p[i.index].begin(), p[i.index].end());
						cut(tocut);
						change = true;
					}
				}
			}
		}
	}
}

void graph::synchronize()
{
	half_synchronization sync;
	for (sync.passive.index = 0; sync.passive.index < (int)transitions.size(); sync.passive.index++)
		if (transitions[sync.passive.index].behavior == transition::passive && !transitions[sync.passive.index].action.is_tautology())
			for (sync.active.index = 0; sync.active.index < (int)transitions.size(); sync.active.index++)
				if (transitions[sync.active.index].behavior == transition::active && !transitions[sync.active.index].action.is_tautology())
					for (sync.passive.cube = 0; sync.passive.cube < (int)transitions[sync.passive.index].action.cubes.size(); sync.passive.cube++)
						for (sync.active.cube = 0; sync.active.cube < (int)transitions[sync.active.index].action.cubes.size(); sync.active.cube++)
							if (similarity_g0(transitions[sync.active.index].action.cubes[sync.active.cube], transitions[sync.passive.index].action.cubes[sync.passive.cube]))
								synchronizations.push_back(sync);
}

void graph::unravel()
{

}

void graph::elaborate()
{

}

}
