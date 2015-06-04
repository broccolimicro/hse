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

local_token::local_token()
{
	index = 0;
	remotable = false;
}

local_token::local_token(int index, bool remotable)
{
	this->index = index;
	this->remotable = remotable;
}

local_token::local_token(int index, vector<int> guard, bool remotable)
{
	this->index = index;
	this->guard = guard;
	this->remotable = remotable;
}

local_token::local_token(const reset_token &t)
{
	index = t.index;
	remotable = t.remotable;
}

local_token::~local_token()
{

}

local_token &local_token::operator=(const reset_token &t)
{
	index = t.index;
	remotable = t.remotable;
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

state::state(vector<local_token> tokens, vector<term_index> environment, boolean::cover encodings)
{
	this->environment = environment;
	for (int i = 0; i < (int)tokens.size(); i++)
		this->tokens.push_back(tokens[i]);
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

enabled_transition::enabled_transition()
{
	index = 0;
	term = 0;
	vacuous = false;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
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
		result += export_assignment(g.transitions[effect.index].local_action[effect.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[effect.index].local_action[effect.term], v).to_string() + "]";

	result += " cause: {";

	for (int j = 0; j < (int)cause.size(); j++)
	{
		if (j != 0)
			result += "; ";

		result += "T" + ::to_string(cause[j].index) + "." + ::to_string(cause[j].term) + ":";

		if (g.transitions[cause[j].index].behavior == hse::transition::active)
			result += export_assignment(g.transitions[cause[j].index].local_action[cause[j].term], v).to_string();
		else
			result += "[" + export_guard_xfactor(g.transitions[cause[j].index].local_action[cause[j].term], v).to_string() + "]";
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

interference::interference(term_index first, term_index second) : pair<term_index, term_index>(first, second)
{
}

interference::~interference()
{

}

string interference::to_string(const hse::graph &g, const boolean::variable_set &v)
{
	string result = "interfering assignments T" + ::to_string(first.index) + "." + ::to_string(first.term) + ":";
	if (g.transitions[first.index].behavior == hse::transition::active)
		result += export_assignment(g.transitions[first.index].local_action[first.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[first.index].local_action[first.term], v).to_string() + "]";
	result += " and T" + ::to_string(second.index) + "." + ::to_string(second.term) + ":";
	if (g.transitions[second.index].behavior == hse::transition::active)
		result += export_assignment(g.transitions[second.index].local_action[second.term], v).to_string();
	else
		result += "[" + export_guard_xfactor(g.transitions[second.index].local_action[second.term], v).to_string() + "]";
	return result;
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

deadlock::deadlock(vector<local_token> tokens, vector<term_index> environment, boolean::cover encodings) : state(tokens, environment, encodings)
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
