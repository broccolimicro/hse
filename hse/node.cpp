/*
 * node.cpp
 *
 *  Created on: Feb 3, 2015
 *      Author: nbingham
 */

#include "node.h"

namespace hse
{
place::place()
{

}

place::~place()
{

}

place place::parallel_merge(place p0, place p1)
{
	place result;
	result.effective = p0.effective & p1.effective;
	result.predicate = p0.predicate & p1.predicate;
	return result;
}

place place::conditional_merge(place p0, place p1)
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

transition transition::parallel_merge(transition t0, transition t1)
{
	transition result;
	result.action = t0.action & t1.action;
	result.type = t0.type;
	return result;
}

transition transition::conditional_merge(transition t0, transition t1)
{
	transition result;
	result.action = t0.action | t1.action;
	result.type = t0.type;
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
}
