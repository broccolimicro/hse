/*
 * node.h
 *
 *  Created on: Feb 3, 2015
 *      Author: nbingham
 */

#include <boolean/cover.h>

#ifndef hse_node_h
#define hse_node_h

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
}

#endif
