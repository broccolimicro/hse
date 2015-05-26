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

remote_token::remote_token()
{
	index = 0;
}

remote_token::remote_token(int index)
{
	this->index = index;
}

remote_token::~remote_token()
{

}

bool operator<(remote_token i, remote_token j)
{
	return i.index < j.index;
}

bool operator>(remote_token i, remote_token j)
{
	return i.index > j.index;
}

bool operator<=(remote_token i, remote_token j)
{
	return i.index <= j.index;
}

bool operator>=(remote_token i, remote_token j)
{
	return i.index >= j.index;
}

bool operator==(remote_token i, remote_token j)
{
	return i.index == j.index;
}

bool operator!=(remote_token i, remote_token j)
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

local_token::local_token(int index, boolean::cube state, vector<term_index> guard)
{
	this->index = index;
	this->state = state;
	this->guard = guard;
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

instability::instability()
{

}

instability::instability(term_index effect, vector<term_index> cause)
{
	this->effect = effect;
	this->cause = cause;
}

instability::~instability()
{

}

bool operator<(instability i, instability j)
{
	return (i.effect < j.effect) ||
		   (i.effect == j.effect && i.cause < j.cause);
}

bool operator>(instability i, instability j)
{
	return (i.effect > j.effect) ||
		   (i.effect == j.effect && i.cause > j.cause);
}

bool operator<=(instability i, instability j)
{
	return (i.effect < j.effect) ||
		   (i.effect == j.effect && i.cause <= j.cause);
}

bool operator>=(instability i, instability j)
{
	return (i.effect > j.effect) ||
		   (i.effect == j.effect && i.cause >= j.cause);
}

bool operator==(instability i, instability j)
{
	return (i.effect == j.effect && i.cause == j.cause);
}

bool operator!=(instability i, instability j)
{
	return (i.effect != j.effect || i.cause != j.cause);
}

interference::interference()
{

}

interference::interference(term_index first, term_index second)
{
	this->first = first;
	this->second = second;
}

interference::~interference()
{

}

bool operator<(interference i, interference j)
{
	return (i.first < j.first) ||
		   (i.first == j.first && i.second < j.second);
}

bool operator>(interference i, interference j)
{
	return (i.first > j.first) ||
		   (i.first == j.first && i.second > j.second);
}

bool operator<=(interference i, interference j)
{
	return (i.first < j.first) ||
		   (i.first == j.first && i.second <= j.second);
}

bool operator>=(interference i, interference j)
{
	return (i.first > j.first) ||
		   (i.first == j.first && i.second >= j.second);
}

bool operator==(interference i, interference j)
{
	return (i.first == j.first && i.second == j.second);
}

bool operator!=(interference i, interference j)
{
	return (i.first != j.first || i.second != j.second);
}

deadlock::deadlock()
{

}

deadlock::deadlock(vector<local_token> tokens)
{
	this->tokens = tokens;
}

deadlock::~deadlock()
{

}

bool operator<(deadlock i, deadlock j)
{
	return i.tokens < j.tokens;
}

bool operator>(deadlock i, deadlock j)
{
	return i.tokens > j.tokens;
}

bool operator<=(deadlock i, deadlock j)
{
	return i.tokens <= j.tokens;
}

bool operator>=(deadlock i, deadlock j)
{
	return i.tokens >= j.tokens;
}

bool operator==(deadlock i, deadlock j)
{
	return i.tokens == j.tokens;
}

bool operator!=(deadlock i, deadlock j)
{
	return i.tokens != j.tokens;
}

}
