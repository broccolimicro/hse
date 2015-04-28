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
enum relation
{
	parallel = 0,
	choice = 1,
	sequence = 2
};

struct place
{
	place();
	~place();

	static const int type = 0;
	boolean::cover predicate;
	boolean::cover effective;
};

struct transition
{
	transition();
	transition(int behavior, boolean::cover action = 1);
	~transition();

	enum
	{
		passive = 0,
		active = 1
	};

	static const int type = 1;
	boolean::cover action;
	int behavior;
};

place merge(int relation, place p0, place p1);
transition merge(int relation, transition t0, transition t1);

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

	bool operator==(int i) const;
	bool operator!=(int i) const;
	bool operator<(int i) const;
	bool operator>(int i) const;
	bool operator<=(int i) const;
	bool operator>=(int i) const;
};

struct arc
{
	arc();
	arc(iterator from, iterator to);
	~arc();

	iterator from;
	iterator to;
};

struct half_synchronization
{
	half_synchronization();
	~half_synchronization();

	struct
	{
		int index;
		int cube;
	} active, passive;
};

}

#endif
