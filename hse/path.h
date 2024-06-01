#pragma once

#include <common/standard.h>
#include <petri/graph.h>

namespace hse
{

// A path records a sequence of step from any node or region of the graph to
// any other node or region of the graph. This is used to understand the
// structure of the graph for state variable insertion.
struct path
{
	// num_places is the total number of places in the graph
	// num_transitions is the total number of transitions in the graph
	path(int num_places, int num_transitions);
	~path();

	// The start and end of the path
	vector<petri::iterator> from, to;

	// This vector is resized to contain an integer for every place and
	// transition in the graph with places listed first, then transitions. Each
	// integer counts the number of times a particular path passes through that
	// place or transition. As a result, this structure can also be used to
	// accumulate multiple paths through the graph from one node or region to
	// another. 
	vector<int> hops;

	// The total number of places in the graph
	int num_places;
	// The total number of transitions in the graph
	int num_transitions;

	// Convertions between petri::iterator and an index into path::hops.
	int idx(petri::iterator i);
	petri::iterator iter(int i);

	void clear();
	bool is_empty();

	vector<petri::iterator> maxima();
	int max();
	int max(int i);
	int max(vector<int> i);

	path mask();
	path inverse_mask();

	void zero(petri::iterator i);
	void zero(vector<petri::iterator> i);
	void inc(petri::iterator i, int v = 1);
	void dec(petri::iterator i, int v = 1);

	path &operator=(path p);
	path &operator+=(path p);
	path &operator-=(path p);
	path &operator*=(path p);
	path &operator*=(int n);
	path &operator/=(int n);

	int &operator[](petri::iterator i);
};

// A path set helps to manage multiple paths from one place or region to
// another to ensure the state variable insertion algorithm is able to cut them
// all with state transitions.
struct path_set
{
	path_set(int num_places, int num_transitions);
	~path_set();

	// The list of paths in this set.
	list<path> paths;

	// The accumulated total visit counts for all paths. This is updated when
	// paths are added to or removed from the set.
	path total;

	void merge(const path_set &s);
	void push(path p);
	void clear();
	void repair();

	list<path>::iterator erase(list<path>::iterator i);
	list<path>::iterator begin();
	list<path>::iterator end();

	void zero(petri::iterator i);
	void zero(vector<petri::iterator> i);
	void inc(petri::iterator i, int v = 1);
	void dec(petri::iterator i, int v = 1);
	void inc(list<path>::iterator i, petri::iterator j, int v = 1);
	void dec(list<path>::iterator i, petri::iterator j, int v = 1);

	path_set mask();
	path_set inverse_mask();

	path_set coverage(petri::iterator i);
	path_set avoidance(petri::iterator i);

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

