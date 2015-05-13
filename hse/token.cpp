/*
 * token.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "token.h"
#include "common/text.h"
#include "common/message.h"

namespace hse
{

token::token()
{
	index = 0;
	state = 1;
}

token::token(int index, boolean::cube state)
{
	this->index = index;
	this->state = state;
}

token::~token()
{

}

bool operator<(token i, token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state < j.state);
}

bool operator>(token i, token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state > j.state);
}

bool operator<=(token i, token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state <= j.state);
}

bool operator>=(token i, token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state >= j.state);
}

bool operator==(token i, token j)
{
	return (i.index == j.index && i.state == j.state);
}

bool operator!=(token i, token j)
{
	return (i.index != j.index || i.state != j.state);
}

enabled_transition::enabled_transition()
{
	index = 0;
	term = 0;
	state = 1;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
	this->state = 1;
}

enabled_transition::~enabled_transition()
{

}


}
