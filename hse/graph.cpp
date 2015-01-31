/*
 * graph.cpp
 *
 *  Created on: Jan 31, 2015
 *      Author: nbingham
 */

#include <common/text.h>
#include "graph.h"

namespace hse
{
index::index()
{
	value = 0;
}

index::index(int value, bool is_place)
{
	this->value = is_place ? value : (value | 0x80000000);
}

index::~index()
{
}

bool index::is_place() const
{
	return (value >= 0);
}

bool index::is_transition() const
{
	return (value < 0);
}

string index::to_string() const
{
	return (value >= 0 ? "P" : "T") + ::to_string(value & 0x7FFFFFFF);
}

int index::idx() const
{
	return (value & 0x7FFFFFFF);
}

index &index::operator=(index i)
{
	value = i.value;
	return *this;
}

index &index::operator--()
{
	--value;
	return *this;
}

index &index::operator++()
{
	++value;
	return *this;
}

index &index::operator--(int)
{
	value--;
	return *this;
}

index &index::operator++(int)
{
	value++;
	return *this;
}

}
