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

term_index::term_index(int index) : petri::term_index(index)
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
	vacuous = true;
	stable = true;
	guard = 1;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	this->term = 0;
	vacuous = true;
	stable = true;
	guard = 1;
}

enabled_transition::enabled_transition(int index, int term)
{
	this->index = index;
	this->term = term;
	vacuous = true;
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

token::token()
{
	index = 0;
	guard = 1;
	cause = -1;
}

token::token(petri::token t)
{
	index = t.index;
	guard = 1;
	cause = -1;
}

token::token(int index)
{
	this->index = index;
	this->guard = 1;
	this->cause = -1;
}

token::token(int index, boolean::cover guard, int cause)
{
	this->index = index;
	this->guard = guard;
	this->cause = cause;
}

token::~token()
{

}

string token::to_string()
{
	return "P" + ::to_string(index);
}

state::state()
{
	encodings = 1;
}

state::state(vector<petri::token> tokens, boolean::cube encodings)
{
	this->tokens = tokens;
	this->encodings = encodings;
}

state::state(vector<hse::token> tokens, boolean::cube encodings)
{
	for (int i = 0; i < (int)tokens.size(); i++)
		if (tokens[i].cause < 0)
			this->tokens.push_back(tokens[i]);

	sort(this->tokens.begin(), this->tokens.end());
	this->encodings = encodings;
}

state::~state()
{

}

void state::hash(hasher &hash) const
{
	hash.put(&tokens);
}

state state::merge(const state &s0, const state &s1)
{
	state result;

	result.tokens.resize(s0.tokens.size() + s1.tokens.size());
	::merge(s0.tokens.begin(), s0.tokens.end(), s1.tokens.begin(), s1.tokens.end(), result.tokens.begin());
	result.tokens.resize(unique(result.tokens.begin(), result.tokens.end()) - result.tokens.begin());

	result.encodings = s0.encodings & s1.encodings;

	return result;
}

state state::collapse(int index, const state &s)
{
	state result;

	result.tokens.push_back(petri::token(index));
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
			result.tokens.push_back(petri::token(loc->second.index));
	}

	result.encodings = encodings;

	return result;
}

bool state::is_subset_of(const state &s)
{
	return (tokens == s.tokens && encodings.is_subset_of(s.encodings));
}

string state::to_string(const ucs::variable_set &variables)
{
	string result = "{";
	for (int i = 0; i < (int)tokens.size(); i++)
	{
		if (i != 0)
			result += " ";
		result += ::to_string(tokens[i].index);
	}
	result += "} " + export_expression_hfactor(encodings, variables).to_string();
	return result;
}

ostream &operator<<(ostream &os, state s)
{
	os << "{";
	for (int i = 0; i < (int)s.tokens.size(); i++)
	{
		if (i != 0)
			os << " ";
		os << s.tokens[i].index;
	}
	os << "} " << s.encodings;
	return os;
}

bool operator<(state s1, state s2)
{
	return (s1.tokens < s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.encodings < s2.encodings);
}

bool operator>(state s1, state s2)
{
	return (s1.tokens > s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.encodings > s2.encodings);
}

bool operator<=(state s1, state s2)
{
	return (s1.tokens < s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.encodings <= s2.encodings);
}

bool operator>=(state s1, state s2)
{
	return (s1.tokens > s2.tokens) ||
		   (s1.tokens == s2.tokens && s1.encodings >= s2.encodings);
}

bool operator==(state s1, state s2)
{
	return s1.tokens == s2.tokens && s1.encodings == s2.encodings;
}

bool operator!=(state s1, state s2)
{
	return s1.tokens != s2.tokens || s1.encodings != s2.encodings;
}

}
