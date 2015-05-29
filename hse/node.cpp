/*
 * node.cpp
 *
 *  Created on: Feb 3, 2015
 *      Author: nbingham
 */

#include "node.h"
#include "graph.h"
#include <interpret_boolean/export.h>

namespace hse
{
place::place()
{

}

place::place(boolean::cover predicate)
{
	this->predicate = predicate;
	this->effective = predicate;
}

place::~place()
{

}

transition::transition()
{
	behavior = active;
	action = 1;
}

transition::transition(int behavior, boolean::cover action)
{
	this->behavior = behavior;
	this->action = action;
}

transition::~transition()
{

}

transition transition::subdivide(int term)
{
	return transition(behavior, boolean::cover(action.cubes[term]));
}

place merge(int relation, place p0, place p1)
{
	place result;
	if (relation == parallel)
	{
		result.effective = p0.effective & p1.effective;
		result.predicate = p0.predicate & p1.predicate;
	}
	else if (relation == choice)
	{
		result.effective = p0.effective | p1.effective;
		result.predicate = p0.predicate | p1.predicate;
	}
	return result;
}

transition merge(int relation, transition t0, transition t1)
{
	transition result;
	if (relation == parallel)
	{
		result.action = t0.action & t1.action;
		result.behavior = t0.behavior;
	}
	else if (relation == choice)
	{
		result.action = t0.action | t1.action;
		result.behavior = t0.behavior;
	}
	return result;
}

iterator::iterator()
{
	type = -1;
	index = -1;
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

bool iterator::operator==(int i) const
{
	return index == i;
}

bool iterator::operator!=(int i) const
{
	return index != i;
}

bool iterator::operator<(int i) const
{
	return index < i;
}

bool iterator::operator>(int i) const
{
	return index > i;
}

bool iterator::operator<=(int i) const
{
	return index <= i;
}

bool iterator::operator>=(int i) const
{
	return index >= i;
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

term_index::term_index()
{
	index = -1;
	term = -1;
}

term_index::term_index(int index, int term)
{
	this->index = index;
	this->term = term;
}

term_index::~term_index()
{

}

string term_index::to_string(const graph &g, const boolean::variable_set &v)
{
	if (g.transitions[index].behavior == transition::active)
		return "T" + ::to_string(index) + "." + ::to_string(term) + ":" + export_internal_choice(g.transitions[index].action[term], v).to_string();
	else
		return "T" + ::to_string(index) + "." + ::to_string(term) + ":" + export_disjunction(g.transitions[index].action[term], v).to_string();
}

bool operator<(term_index i, term_index j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.term < j.term);
}

bool operator>(term_index i, term_index j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.term > j.term);
}

bool operator<=(term_index i, term_index j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.term <= j.term);
}

bool operator>=(term_index i, term_index j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.term >= j.term);
}

bool operator==(term_index i, term_index j)
{
	return (i.index == j.index && i.term == j.term);
}

bool operator!=(term_index i, term_index j)
{
	return (i.index != j.index || i.term != j.term);
}

half_synchronization::half_synchronization()
{
	active.index = 0;
	active.cube = 0;
	passive.index = 0;
	passive.cube = 0;
}

half_synchronization::~half_synchronization()
{

}

synchronization_region::synchronization_region()
{

}

synchronization_region::~synchronization_region()
{

}

}
