/*
 * state.cpp
 *
 *  Created on: Jun 23, 2015
 *      Author: nbingham
 */

#include <common/text.h>
#include <interpret_boolean/export.h>
#include "state.h"
#include "graph.h"

namespace hse
{

term_index::term_index()
{
	term = -1;
}

term_index::term_index(int index, int term) : petri::term_index(index)
{
	this->term = term;
}

term_index::~term_index()
{

}

void term_index::hash(hasher &hash) const
{
	hash.put(&index);
	hash.put(&term);
}

string term_index::to_string(const graph &g, const ucs::variable_set &v)
{
	if (g.transitions[index].behavior == transition::active)
		return "T" + ::to_string(index) + "." + ::to_string(term) + ":" + export_composition(g.transitions[index].local_action[term], v).to_string();
	else
		return "T" + ::to_string(index) + "." + ::to_string(term) + ":[" + export_expression_xfactor(g.transitions[index].local_action[term], v).to_string() + "]";
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
	vacuous = false;
	stable = true;
	remotable = true;
	guard = 1;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
	vacuous = false;
	stable = true;
	remotable = true;
	guard = 1;
}

enabled_transition::enabled_transition(int index, int term)
{
	this->index = index;
	this->term = term;
	vacuous = false;
	stable = true;
	remotable = true;
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
	cause = -1;
}

local_token::local_token(int index, bool remotable)
{
	this->index = index;
	this->remotable = remotable;
	this->guard = 1;
	this->cause = -1;
}

local_token::local_token(int index, boolean::cover guard, int cause, bool remotable)
{
	this->index = index;
	this->guard = guard;
	this->cause = cause;
	this->remotable = remotable;
}

local_token::local_token(const reset_token &t)
{
	index = t.index;
	remotable = t.remotable;
	guard = 1;
	cause = -1;
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

string reset_token::to_string(const ucs::variable_set &variables)
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
		if (tokens[i].cause < 0)
			this->tokens.push_back(reset_token(tokens[i]));

	sort(this->tokens.begin(), this->tokens.end());
	sort(this->environment.begin(), this->environment.end());
	this->encodings = encodings;
}

state::~state()
{

}

void state::hash(hasher &hash) const
{
	hash.put(&tokens);
	hash.put(&environment);
}

state state::merge(int composition, const state &s0, const state &s1)
{
	state result;

	result.tokens.resize(s0.tokens.size() + s1.tokens.size());
	::merge(s0.tokens.begin(), s0.tokens.end(), s1.tokens.begin(), s1.tokens.end(), result.tokens.begin());
	result.tokens.resize(unique(result.tokens.begin(), result.tokens.end()) - result.tokens.begin());

	result.environment.resize(s0.environment.size() + s1.environment.size());
	::merge(s0.environment.begin(), s0.environment.end(), s1.environment.begin(), s1.environment.end(), result.environment.begin());
	result.environment.resize(unique(result.environment.begin(), result.environment.end()) - result.environment.begin());

	if (composition == petri::parallel)
		result.encodings = s0.encodings & s1.encodings;
	else if (composition == petri::choice)
		result.encodings = s0.encodings | s1.encodings;

	return result;
}

state state::collapse(int composition, int index, const state &s)
{
	state result;

	bool remotable = true;
	for (int i = 0; i < (int)s.tokens.size(); i++)
		remotable = (remotable && s.tokens[i].remotable);
	result.tokens.push_back(reset_token(index, remotable));

	result.encodings = s.encodings;

	return result;
}

state state::convert(map<petri::iterator, petri::iterator> translate) const
{
	state result;

	for (int i = 0; i < (int)tokens.size(); i++)
	{
		map<petri::iterator, petri::iterator>::iterator loc = translate.find(petri::iterator(place::type, tokens[i].index));
		if (loc != translate.end())
			result.tokens.push_back(reset_token(loc->second.index, tokens[i].remotable));
	}

	for (int i = 0; i < (int)environment.size(); i++)
	{
		map<petri::iterator, petri::iterator>::iterator loc = translate.find(petri::iterator(transition::type, environment[i].index));
		if (loc != translate.end())
			result.environment.push_back(term_index(loc->second.index, environment[i].term));
	}

	result.encodings = encodings;

	return result;
}

bool state::is_subset_of(const state &s)
{
	return (tokens == s.tokens && environment == s.environment && encodings.is_subset_of(s.encodings));
}

string state::to_string(const ucs::variable_set &variables)
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
	result += "} " + export_expression_hfactor(encodings, variables).to_string();
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

}
