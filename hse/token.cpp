/*
 * token.cpp
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include "token.h"
#include <common/text.h>
#include <common/message.h>
#include <interpret_boolean/export.h>
#include <boolean/variable.h>
#include "graph.h"

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

string remote_token::to_string()
{
	return "P" + ::to_string(index);
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

string local_token::to_string(const boolean::variable_set &variables)
{
	return "P" + ::to_string(index) + ":" + export_guard(state, variables).to_string();
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

string reset_token::to_string(const boolean::variable_set &variables)
{
	return "P" + ::to_string(index) + ":" + export_guard(state, variables).to_string();
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
	vacuous = false;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
	this->state = 1;
	vacuous = false;
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

string instability::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	string result = "unstable assignment T" + ::to_string(effect.index) + "." + ::to_string(effect.term) + ":";
	if (g.transitions[effect.index].behavior == hse::transition::active)
		result += export_assignment(g.transitions[effect.index].action[effect.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[effect.index].action[effect.term], v).to_string() + "]";

	result += " cause: {";

	for (int j = 0; j < (int)cause.size(); j++)
	{
		if (j != 0)
			result += "; ";

		result += "T" + ::to_string(cause[j].index) + "." + ::to_string(cause[j].term) + ":";

		if (g.transitions[cause[j].index].behavior == hse::transition::active)
			result += export_assignment(g.transitions[cause[j].index].action[cause[j].term], v).to_string();
		else
			result += "[" + export_guard_xfactor(g.transitions[cause[j].index].action[cause[j].term], v).to_string() + "]";
	}
	result += "}";
	return result;
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

string interference::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	string result = "interfering assignments T" + ::to_string(first.index) + "." + ::to_string(first.term) + ":";
	if (g.transitions[first.index].behavior == hse::transition::active)
		result += export_assignment(g.transitions[first.index].action[first.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[first.index].action[first.term], v).to_string() + "]";
	result += " and T" + ::to_string(second.index) + "." + ::to_string(second.term) + ":";
	if (g.transitions[second.index].behavior == hse::transition::active)
		result += export_assignment(g.transitions[second.index].action[second.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[second.index].action[second.term], v).to_string() + "]";
	return result;
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

string deadlock::to_string(const boolean::variable_set &v)
{
	string result = "deadlock {";
	for (int i = 0; i < (int)tokens.size(); i++)
	{
		if (i != 0)
			result += "  ";
		result += tokens[i].to_string(v);
	}
	result += "}";

	return result;
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
