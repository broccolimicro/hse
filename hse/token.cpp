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

enabled_transition::enabled_transition()
{
	index = 0;
	term = 0;
	vacuous = false;
	stable = true;
	guard = 1;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
	vacuous = false;
	stable = true;
	guard = 1;
}

enabled_transition::~enabled_transition()
{

}

bool operator<(enabled_transition i, enabled_transition j)
{
	return ((term_index)i < (term_index)j) ||
		   ((term_index)i == (term_index)j && i.history < j.history);
}

bool operator>(enabled_transition i, enabled_transition j)
{
	return ((term_index)i > (term_index)j) ||
		   ((term_index)i == (term_index)j && i.history > j.history);
}

bool operator<=(enabled_transition i, enabled_transition j)
{
	return ((term_index)i < (term_index)j) ||
		   ((term_index)i == (term_index)j && i.history <= j.history);
}

bool operator>=(enabled_transition i, enabled_transition j)
{
	return ((term_index)i > (term_index)j) ||
		   ((term_index)i == (term_index)j && i.history >= j.history);
}

bool operator==(enabled_transition i, enabled_transition j)
{
	return ((term_index)i == (term_index)j && i.history == j.history);
}

bool operator!=(enabled_transition i, enabled_transition j)
{
	return ((term_index)i != (term_index)j || i.history != j.history);
}

enabled_environment::enabled_environment()
{
	index = 0;
	term = 0;
	guard = 1;
}

enabled_environment::enabled_environment(int index)
{
	this->index = index;
	this->term = 0;
	this->guard = 1;
}

enabled_environment::~enabled_environment()
{

}

local_token::local_token()
{
	index = 0;
	remotable = false;
	guard = 1;
}

local_token::local_token(int index, bool remotable)
{
	this->index = index;
	this->remotable = remotable;
	this->guard = 1;
}

local_token::local_token(int index, boolean::cover guard, vector<enabled_transition> prev, bool remotable)
{
	this->index = index;
	this->guard = guard;
	this->prev = prev;
	this->remotable = remotable;
}

local_token::local_token(const reset_token &t)
{
	index = t.index;
	remotable = t.remotable;
	guard = 1;
}

local_token::~local_token()
{

}

local_token &local_token::operator=(const reset_token &t)
{
	index = t.index;
	remotable = t.remotable;
	guard = 1;
	return *this;
}

string local_token::to_string()
{
	return "P" + ::to_string(index);
}

bool operator<(local_token i, local_token j)
{
	return i.index < j.index;
}

bool operator>(local_token i, local_token j)
{
	return i.index > j.index;
}

bool operator<=(local_token i, local_token j)
{
	return i.index < j.index;
}

bool operator>=(local_token i, local_token j)
{
	return i.index > j.index;
}

bool operator==(local_token i, local_token j)
{
	return i.index == j.index;
}

bool operator!=(local_token i, local_token j)
{
	return i.index != j.index;
}

remote_token::remote_token()
{
	index = 0;
	guard = 1;
}

remote_token::remote_token(int index, boolean::cover guard)
{
	this->index = index;
	this->guard = guard;
}

remote_token::remote_token(const reset_token &t)
{
	index = t.index;
	guard = 1;
}

remote_token::~remote_token()
{

}

remote_token &remote_token::operator=(const reset_token &t)
{
	index = t.index;
	guard = 1;
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

reset_token::reset_token()
{
	index = 0;
	remotable = false;
}

reset_token::reset_token(int index, bool remotable)
{
	this->index = index;
	this->remotable = remotable;
}

reset_token::reset_token(const remote_token &t)
{
	index = t.index;
	remotable = true;
}

reset_token::reset_token(const local_token &t)
{
	index = t.index;
	remotable = t.remotable;
}

reset_token::~reset_token()
{

}

reset_token &reset_token::operator=(const remote_token &t)
{
	index = t.index;
	remotable = true;
	return *this;
}

reset_token &reset_token::operator=(const local_token &t)
{
	index = t.index;
	remotable = t.remotable;
	return *this;
}

string reset_token::to_string(const boolean::variable_set &variables)
{
	return "P" + ::to_string(index);
}

ostream &operator<<(ostream &os, vector<reset_token> t)
{
	os << "{";
	for (int i = 0; i < (int)t.size(); i++)
	{
		if (i != 0)
			os << " ";
		os << t[i].index << "," << t[i].remotable;
	}
	os << "}";
	return os;
}

bool operator<(reset_token i, reset_token j)
{
	return i.index < j.index;
}

bool operator>(reset_token i, reset_token j)
{
	return i.index > j.index;
}

bool operator<=(reset_token i, reset_token j)
{
	return i.index <= j.index;
}

bool operator>=(reset_token i, reset_token j)
{
	return i.index >= j.index;
}

bool operator==(reset_token i, reset_token j)
{
	return i.index == j.index;
}

bool operator!=(reset_token i, reset_token j)
{
	return i.index != j.index;
}

state::state()
{
	encodings = 1;
}

state::state(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings)
{
	this->tokens = tokens;
	this->environment = environment;
	this->encodings = encodings;
}

state::state(vector<local_token> tokens, deque<enabled_environment> environment, boolean::cover encodings)
{
	for (int i = 0; i < (int)environment.size(); i++)
		this->environment.push_back((term_index)environment[i]);
	for (int i = 0; i < (int)tokens.size(); i++)
		this->tokens.push_back(reset_token(tokens[i]));
	sort(this->tokens.begin(), this->tokens.end());
	sort(this->environment.begin(), this->environment.end());
	this->encodings = encodings;
}

state::~state()
{

}

state &state::merge(int relation, const state &s)
{
	if (tokens.size() == 0)
	{
		tokens = s.tokens;
		environment = s.environment;
		encodings = s.encodings;
	}
	else if (s.tokens.size() != 0)
	{
		if (relation == parallel)
		{
			vector<reset_token> oldtokens;
			swap(tokens, oldtokens);
			tokens.resize(oldtokens.size() + s.tokens.size());
			::merge(oldtokens.begin(), oldtokens.end(), s.tokens.begin(), s.tokens.end(), tokens.begin());
			vector<term_index> oldenvironment;
			swap(environment, oldenvironment);
			environment.resize(oldenvironment.size() + s.environment.size());
			::merge(oldenvironment.begin(), oldenvironment.end(), s.environment.begin(), s.environment.end(), environment.begin());
			environment.resize(unique(environment.begin(), environment.end()) - environment.begin());
			encodings &= s.encodings;
		}
		else if (relation == choice)
		{
			if (tokens != s.tokens && environment != s.environment)
				return *this;

			encodings |= s.encodings;
		}
	}

	return *this;
}

bool state::is_subset_of(const state &s)
{
	return (tokens == s.tokens && environment == s.environment && encodings.is_subset_of(s.encodings));
}

string state::to_string(const boolean::variable_set &variables)
{
	string result = "{";
	for (int i = 0; i < (int)tokens.size(); i++)
	{
		if (i != 0)
			result += " ";
		result += tokens[i].to_string(variables);
	}
	result += "} {";
	for (int i = 0; i < (int)environment.size(); i++)
	{
		if (i != 0)
			result += " ";
		result += "T" + ::to_string(environment[i].index) + "." + ::to_string(environment[i].term);
	}
	result += "} " + export_guard_hfactor(encodings, variables).to_string();
	return result;
}

bool operator<(state s1, state s2)
{
	return (s1.tokens < s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.environment < s2.environment);
}

bool operator>(state s1, state s2)
{
	return (s1.tokens > s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.environment > s2.environment);
}

bool operator<=(state s1, state s2)
{
	return (s1.tokens < s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.environment <= s2.environment);
}

bool operator>=(state s1, state s2)
{
	return (s1.tokens > s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.environment >= s2.environment);
}

bool operator==(state s1, state s2)
{
	return s1.tokens == s2.tokens && s1.environment == s2.environment;
}

bool operator!=(state s1, state s2)
{
	return s1.tokens != s2.tokens || s1.environment != s2.environment;
}

instability::instability()
{
}

instability::instability(const enabled_transition &cause) : enabled_transition(cause)
{
}

instability::~instability()
{

}

string instability::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	string result;
	if (g.transitions[index].behavior == hse::transition::active)
		result = "unstable assignment " + enabled_transition::to_string(g, v);
	else
		result = "unstable guard " + enabled_transition::to_string(g, v);

	result += " cause: {";

	for (int j = 0; j < (int)history.size(); j++)
	{
		if (j != 0)
			result += "; ";

		result += history[j].to_string(g, v);
	}
	result += "}";
	return result;
}

interference::interference()
{

}

interference::interference(const enabled_transition &first, const enabled_transition &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

interference::~interference()
{

}

string interference::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	if (!first.stable || !second.stable)
		return "weakly interfering assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
	else
		return "interfering assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

mutex::mutex()
{

}

mutex::mutex(const enabled_transition &first, const enabled_transition &second)
{
	if (first < second)
	{
		this->first = first;
		this->second = second;
	}
	else
	{
		this->first = second;
		this->second = first;
	}
}

mutex::~mutex()
{

}

string mutex::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	return "vacuous firings break mutual exclusion for assignments " + first.to_string(g, v) + " and " + second.to_string(g, v);
}

deadlock::deadlock()
{

}

deadlock::deadlock(const state &s) : state(s)
{

}

deadlock::deadlock(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings) : state(tokens, environment, encodings)
{

}

deadlock::deadlock(vector<local_token> tokens, deque<enabled_environment> environment, boolean::cover encodings) : state(tokens, environment, encodings)
{

}

deadlock::~deadlock()
{

}

string deadlock::to_string(const boolean::variable_set &v)
{
	return "deadlock detected at state " + state::to_string(v);
}

}
