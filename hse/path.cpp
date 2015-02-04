/*
 * path.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "path.h"

namespace hse
{
path::path()
{
}

path::~path()
{

}

void path::clear()
{
	hops.clear();
	from.clear();
	to.clear();
}

bool path::is_empty()
{
	for (int i = 0; i < (int)hops.size(); i++)
		if (hops[i] != 0)
			return false;
	return true;
}

vector<int> path::maxima()
{
	vector<int> result;
	for (int i = 0; i < (int)hops.size(); i++)
	{
		if (result.size() == 0)
			result.push_back(i);
		else if (hops[i] > hops[result[0]])
		{
			result.clear();
			result.push_back(i);
		}
		else if (hops[i] == hops[result[0]])
			result.push_back(i);
	}
	return result;
}

int path::max()
{
	int m = 0;
	for (int j = 0; j < (int)hops.size(); j++)
		if (hops[j] > m)
			m = hops[j];
	return m;
}

int path::max(int i)
{
	return hops[i];
}

int path::max(vector<int> i)
{
	int m = 0;
	for (int j = 0; j < (int)i.size(); j++)
		if (hops[i[j]] > m)
			m = hops[i[j]];
	return m;
}

path path::mask()
{
	path result;
	result.from = from;
	result.to = to;
	result.hops.resize(hops.size());
	for (int i = 0; i < (int)hops.size(); i++)
		result.hops[i] = (hops[i] > 0);
	return result;
}

path path::inverse_mask()
{
	path result;
	result.from = from;
	result.to = to;
	result.hops.resize(hops.size());
	for (int i = 0; i < (int)hops.size(); i++)
		result.hops[i] = (hops[i] <= 0);
	return result;
}

void path::zero(int i)
{
	hops[i] = 0;
}

void path::zero(vector<int> i)
{
	for (int j = 0; j < (int)i.size(); j++)
		hops[i[j]] = 0;
}

void path::inc(int i, int v)
{
	hops[i] += v;
}

void path::dec(int i, int v)
{
	hops[i] -= v;
}

path &path::operator=(path p)
{
	from = p.from;
	to = p.to;
	hops = p.hops;
	return *this;
}

path &path::operator+=(path p)
{
	from.insert(from.end(), p.from.begin(), p.from.end());
	to.insert(to.end(), p.to.begin(), p.to.end());
	hops.resize(std::max(hops.size(), p.hops.size()), 0);
	for (int i = 0; i < (int)p.hops.size(); i++)
		hops[i] += p.hops[i];
	return *this;
}

path &path::operator-=(path p)
{
	sort(from.begin(), from.end());
	sort(to.begin(), to.end());
	sort(p.from.begin(), p.from.end());
	sort(p.to.begin(), p.to.end());
	from.resize(set_difference(from.begin(), from.end(), p.from.begin(), p.from.end(), from.begin()) - from.begin());
	to.resize(set_difference(to.begin(), to.end(), p.to.begin(), p.to.end(), to.begin()) - to.begin());
	hops.resize(std::max(hops.size(), p.hops.size()), 0);
	for (int i = 0; i < (int)p.hops.size(); i++)
		hops[i] -= p.hops[i];
	return *this;
}

path &path::operator*=(path p)
{
	hops.resize(min(hops.size(), p.hops.size()), 0);
	for (int i = 0; i < (int)hops.size(); i++)
		hops[i] *= p.hops[i];
	return *this;
}

path &path::operator*=(int n)
{
	for (int i = 0; i < (int)hops.size(); i++)
		hops[i] *= n;
	return *this;
}

path &path::operator/=(int n)
{
	for (int i = 0; i < (int)hops.size(); i++)
		hops[i] /= n;
	return *this;
}

int &path::operator[](int i)
{
	return hops[i];
}

path_set::path_set()
{

}

path_set::~path_set()
{

}

void path_set::merge(const path_set &s)
{
	paths.insert(paths.end(), s.paths.begin(), s.paths.end());
	total += s.total;
}

void path_set::push(path p)
{
	paths.push_back(p);
	total += p;
}

void path_set::clear()
{
	paths.clear();
	total.clear();
}

void path_set::repair()
{
	total.clear();
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
		total += *p;
}

list<path>::iterator path_set::erase(list<path>::iterator i)
{
	total -= *i;
	return paths.erase(i);
}

list<path>::iterator path_set::begin()
{
	return paths.begin();
}

list<path>::iterator path_set::end()
{
	return paths.end();
}

void path_set::zero(int i)
{
	for (list<path>::iterator p = paths.begin(); p != paths.end();)
	{
		p->hops[i] = 0;
		if (p->is_empty())
			p = paths.erase(p);
		else
			p++;
	}
	total.hops[i] = 0;
}

void path_set::zero(vector<int> i)
{
	for (list<path>::iterator p = paths.begin(); p != paths.end();)
	{
		for (int j = 0; j < (int)i.size(); j++)
			p->hops[i[j]] = 0;

		if (p->is_empty())
			p = paths.erase(p);
		else
			p++;
	}
	for (int j = 0; j < (int)i.size(); j++)
		total.hops[i[j]] = 0;
}

void path_set::inc(int i, int v)
{
	total.hops[i] += paths.size()*v;
	for (list<path>::iterator p = paths.begin(); p != paths.end();)
	{
		p->hops[i] += v;
		if (p->is_empty())
			p = paths.erase(p);
		else
			p++;
	}
}

void path_set::dec(int i, int v)
{
	total.hops[i] -= paths.size()*v;
	for (list<path>::iterator p = paths.begin(); p != paths.end();)
	{
		p->hops[i] -= v;
		if (p->is_empty())
			p = paths.erase(p);
		else
			p++;
	}
}

void path_set::inc(list<path>::iterator i, int j, int v)
{
	i->hops[j] += v;
	total.hops[j] += v;
}

void path_set::dec(list<path>::iterator i, int j, int v)
{
	i->hops[j] -= v;
	total.hops[j] -= v;
}

path_set path_set::mask()
{
	path_set result;
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
		result.push(p->mask());
	return result;
}

path_set path_set::inverse_mask()
{
	path_set result;
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
		result.push(p->inverse_mask());
	return result;
}

path_set path_set::coverage(int i)
{
	path_set result;
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
		if (p->hops[i] != 0)
			result.push(*p);
	return result;
}

path_set path_set::avoidance(int i)
{
	path_set result;
	for (list<path>::iterator p = paths.begin(); p != paths.end(); p++)
		if (p->hops[i] == 0)
			result.push(*p);
	return result;
}

path_set &path_set::operator=(path_set p)
{
	paths = p.paths;
	total = p.total;
	return *this;
}

path_set &path_set::operator+=(path_set p)
{
	paths.insert(paths.end(), p.paths.begin(), p.paths.end());
	total += p.total;
	return *this;
}

path_set &path_set::operator*=(path p)
{
	for (list<path>::iterator i = paths.begin(); i != paths.end();)
	{
		*i *= p;
		if (i->is_empty())
			i = paths.erase(i);
		else
			i++;
	}
	total *= p;
	return *this;
}

}

ostream &operator<<(ostream &os, hse::path p)
{
	os << "[" << p.from << "-";

	for (vector<int>::iterator i = p.hops.begin(); i != p.hops.end(); i++)
		os << *i << " ";

	os << ">" << p.to << "]";
	return os;
}

hse::path operator+(hse::path p0, hse::path p1)
{
	p0.from.insert(p0.from.end(), p1.from.begin(), p1.from.end());
	p0.to.insert(p0.to.end(), p1.to.begin(), p1.to.end());
	p0.hops.resize(max(p0.hops.size(), p1.hops.size()), 0);
	for (int i = 0; i < (int)p1.hops.size(); i++)
		p0.hops[i] += p1.hops[i];
	return p0;
}

hse::path operator-(hse::path p0, hse::path p1)
{
	sort(p0.from.begin(), p0.from.end());
	sort(p0.to.begin(), p0.to.end());
	sort(p1.from.begin(), p1.from.end());
	sort(p1.to.begin(), p1.to.end());
	p0.from.resize(set_difference(p0.from.begin(), p0.from.end(), p1.from.begin(), p1.from.end(), p0.from.begin()) - p0.from.begin());
	p0.to.resize(set_difference(p0.to.begin(), p0.to.end(), p1.to.begin(), p1.to.end(), p0.to.begin()) - p0.to.begin());
	p0.hops.resize(max(p0.hops.size(), p1.hops.size()), 0);
	for (int i = 0; i < (int)p1.hops.size(); i++)
		p0.hops[i] -= p1.hops[i];
	return p0;
}

hse::path operator*(hse::path p0, hse::path p1)
{
	p0.hops.resize(min(p0.hops.size(), p1.hops.size()), 0);
	for (int i = 0; i < (int)p0.hops.size(); i++)
		p0.hops[i] *= p1.hops[i];
	return p0;
}

hse::path operator*(hse::path p0, int n)
{
	for (int i = 0; i < (int)p0.hops.size(); i++)
		p0.hops[i] *= n;
	return p0;
}

hse::path operator/(hse::path p0, int n)
{
	for (int i = 0; i < (int)p0.hops.size(); i++)
		p0.hops[i] *= n;
	return p0;
}

ostream &operator<<(ostream &os, hse::path_set p)
{
	list<hse::path>::iterator i;
	for (i = p.paths.begin(); i != p.paths.end(); i++)
		os << *i << endl;
	return os;
}

hse::path_set operator+(hse::path_set p0, hse::path_set p1)
{
	p0.paths.insert(p0.paths.end(), p1.paths.begin(), p1.paths.end());
	p0.total += p1.total;
	return p0;
}

hse::path_set operator*(hse::path_set p0, hse::path p1)
{
	for (list<hse::path>::iterator i = p0.paths.begin(); i != p0.paths.end();)
	{
		*i *= p1;
		if (i->is_empty())
			i = p0.paths.erase(i);
		else
			i++;
	}
	p0.total *= p1;
	return p0;
}

hse::path_set operator*(hse::path p0, hse::path_set p1)
{
	for (list<hse::path>::iterator i = p1.paths.begin(); i != p1.paths.end();)
	{
		*i *= p0;
		if (i->is_empty())
			i = p1.paths.erase(i);
		else
			i++;
	}
	p1.total *= p0;
	return p1;
}
