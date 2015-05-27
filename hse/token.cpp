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

remote_token::remote_token(const reset_token &t)
{
	index = t.index;
}

remote_token::~remote_token()
{

}

remote_token &remote_token::operator=(const reset_token &t)
{
	index = t.index;
	return *this;
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
	remotable = false;
}

local_token::local_token(int index, boolean::cube state)
{
	this->index = index;
	this->state = state;
	remotable = false;
}

local_token::local_token(int index, boolean::cube state, vector<int> guard, bool remotable)
{
	this->index = index;
	this->state = state;
	this->guard = guard;
	this->remotable = remotable;
}

local_token::local_token(const reset_token &t)
{
	index = t.index;
	state = t.state;
	remotable = t.remotable;
}

local_token::~local_token()
{

}

local_token &local_token::operator=(const reset_token &t)
{
	index = t.index;
	state = t.state;
	remotable = t.remotable;
	return *this;
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

reset_token::reset_token()
{
	index = 0;
	state = 1;
	remotable = false;
}

reset_token::reset_token(int index, boolean::cube state)
{
	this->index = index;
	this->state = state;
	this->remotable = false;
}

reset_token::reset_token(int index, boolean::cube state, bool remotable)
{
	this->index = index;
	this->state = state;
	this->remotable = remotable;
}

reset_token::reset_token(const remote_token &t)
{
	index = t.index;
	state = boolean::cube();
	remotable = true;
}

reset_token::reset_token(const local_token &t)
{
	index = t.index;
	state = t.state;
	remotable = t.remotable;
}

reset_token::~reset_token()
{

}

reset_token &reset_token::operator=(const remote_token &t)
{
	index = t.index;
	state = boolean::cube();
	remotable = true;
	return *this;
}

reset_token &reset_token::operator=(const local_token &t)
{
	index = t.index;
	state = t.state;
	remotable = false;
	return *this;
}

bool operator<(reset_token i, reset_token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state < j.state);
}

bool operator>(reset_token i, reset_token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state > j.state);
}

bool operator<=(reset_token i, reset_token j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.state <= j.state);
}

bool operator>=(reset_token i, reset_token j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.state >= j.state);
}

bool operator==(reset_token i, reset_token j)
{
	return (i.index == j.index && i.state == j.state);
}

bool operator!=(reset_token i, reset_token j)
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

enabled_environment::enabled_environment()
{
	index = 0;
	term = 0;
}

enabled_environment::enabled_environment(int index)
{
	this->index = index;
	this->term = 0;
}

enabled_environment::~enabled_environment()
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
