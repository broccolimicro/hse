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

global_token::global_token()
{
	index = 0;
}

global_token::~global_token()
{

}

bool operator<(global_token i, global_token j)
{
	return i.index < j.index;
}

bool operator>(global_token i, global_token j)
{
	return i.index > j.index;
}

bool operator<=(global_token i, global_token j)
{
	return i.index <= j.index;
}

bool operator>=(global_token i, global_token j)
{
	return i.index >= j.index;
}

bool operator==(global_token i, global_token j)
{
	return i.index == j.index;
}

bool operator!=(global_token i, global_token j)
{
	return i.index != j.index;
}

local_token::local_token()
{
	index = 0;
	state = 1;
}

local_token::local_token(int index, boolean::cube state)
{
	this->index = index;
	this->state = state;
}

local_token::~local_token()
{

}

bool operator<(local_token i, local_token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state < j.state);
}

bool operator>(local_token i, local_token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state > j.state);
}

bool operator<=(local_token i, local_token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state <= j.state);
}

bool operator>=(local_token i, local_token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state >= j.state);
}

bool operator==(local_token i, local_token j)
{
	return (i.index == j.index && i.state == j.state);
}

bool operator!=(local_token i, local_token j)
{
	return (i.index != j.index || i.state != j.state);
}

term_index::term_index()
{
	index = 0;
	term = 0;
}

term_index::term_index(int index, int term)
{
	this->index = index;
	this->term = term;
}

term_index::~term_index()
{

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
