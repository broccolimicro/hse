/*
 * path.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include "graph.h"

#ifndef hse_path_h
#define hse_path_h

namespace hse
{
struct path
{
	path();
	~path();

	vector<int> from, to;
	vector<int> hops;

	void clear();
	bool is_empty();

	vector<int> maxima();
	int max();
	int max(int i);
	int max(vector<int> i);

	path mask();
	path inverse_mask();

	void zero(int i);
	void zero(vector<int> i);
	void inc(int i, int v = 1);
	void dec(int i, int v = 1);

	path &operator=(path p);
	path &operator+=(path p);
	path &operator-=(path p);
	path &operator*=(path p);
	path &operator*=(int n);
	path &operator/=(int n);

	int &operator[](int i);
};

struct path_set
{
	path_set();
	~path_set();

	list<path> paths;
	path total;

	void merge(const path_set &s);
	void push(path p);
	void clear();
	void repair();

	list<path>::iterator erase(list<path>::iterator i);
	list<path>::iterator begin();
	list<path>::iterator end();

	void zero(int i);
	void zero(vector<int> i);
	void inc(int i, int v = 1);
	void dec(int i, int v = 1);
	void inc(list<path>::iterator i, int j, int v = 1);
	void dec(list<path>::iterator i, int j, int v = 1);

	path_set mask();
	path_set inverse_mask();

	path_set coverage(int i);
	path_set avoidance(int i);

	path_set &operator=(path_set p);
	path_set &operator+=(path_set p);
	path_set &operator*=(path p);
};

}

ostream &operator<<(ostream &os, hse::path p);

hse::path operator+(hse::path p0, hse::path p1);
hse::path operator-(hse::path p0, hse::path p1);
hse::path operator*(hse::path p0, hse::path p1);
hse::path operator/(hse::path p1, int n);
hse::path operator*(hse::path p1, int n);

ostream &operator<<(ostream &os, hse::path_set p);

hse::path_set operator+(hse::path_set p0, hse::path_set p1);
hse::path_set operator*(hse::path_set p0, hse::path p1);
hse::path_set operator*(hse::path p0, hse::path_set p1);

#endif
