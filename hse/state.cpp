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
	index = -1;
	term = -1;
}

term_index::term_index(int index)
{
	this->index = index;
	term = -1;
}

term_index::term_index(int index, int term)
{
	this->index = index;
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

string term_index::to_string(const graph &g)
{
	return "T" + ::to_string(index) + "." + ::to_string(term) + ":" + export_expression_xfactor(g.transitions[index].guard[term], g).to_string() + " -> " + export_composition(g.transitions[index].local_action[term], g).to_string();
}

petri::iterator term_index::iter() const {
	return petri::iterator(transition::type, index);
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
	vacuous = true;
	stable = true;
	guard = 1;
	depend = 1;
	sequence = 1;
	fire_at = 0;
}

enabled_transition::enabled_transition(int index)
{
	this->index = index;
	vacuous = true;
	stable = true;
	guard = 1;
	depend = 1;
	sequence = 1;
	fire_at = 0;
}

enabled_transition::enabled_transition(int index, int term)
{
	this->index = index;
	vacuous = true;
	stable = true;
	guard = 1;
	depend = 1;
	sequence = 1;
	fire_at = 0;
}

enabled_transition::~enabled_transition()
{

}

string enabled_transition::to_string(const graph &g)
{
	return "T" + ::to_string(index) + ":" + export_expression_xfactor(g.transitions[index].guard, g).to_string() + " -> " + export_composition(g.transitions[index].local_action, g).to_string();
}

bool operator<(enabled_transition i, enabled_transition j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.history < j.history);
}

bool operator>(enabled_transition i, enabled_transition j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.history > j.history);
}

bool operator<=(enabled_transition i, enabled_transition j)
{
	return (i.index < j.index) ||
		   (i.index == j.index && i.history <= j.history);
}

bool operator>=(enabled_transition i, enabled_transition j)
{
	return (i.index > j.index) ||
		   (i.index == j.index && i.history >= j.history);
}

bool operator==(enabled_transition i, enabled_transition j)
{
	return (i.index == j.index && i.history == j.history);
}

bool operator!=(enabled_transition i, enabled_transition j)
{
	return (i.index != j.index || i.history != j.history);
}

token::token()
{
	index = 0;
	guard = 1;
	assume = 1;
	cause = -1;
	sequence = 1;
}

token::token(petri::token t)
{
	index = t.index;
	guard = 1;
	assume = 1;
	cause = -1;
	sequence = 1;
}

token::token(int index, boolean::cover assume, boolean::cover guard, boolean::cover sequence, int cause)
{
	this->index = index;
	this->guard = guard;
	this->assume = assume;
	this->sequence = sequence;
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

state::state(vector<petri::token> tokens, boolean::cover encodings)
{
	this->tokens = tokens;
	this->encodings = encodings;
}

state::state(vector<hse::token> tokens, boolean::cover encodings)
{
	for (int i = 0; i < (int)tokens.size(); i++)
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

state state::convert(map<petri::iterator, vector<petri::iterator> > translate) const
{
	state result;

	for (int i = 0; i < (int)tokens.size(); i++)
	{
		map<petri::iterator, vector<petri::iterator> >::iterator loc = translate.find(petri::iterator(place::type, tokens[i].index));
		if (loc != translate.end())
			for (int j = 0; j < (int)loc->second.size(); j++)
				result.tokens.push_back(petri::token(loc->second[j].index));
	}

	result.encodings = encodings;

	return result;
}

bool state::is_subset_of(const state &s)
{
	return (tokens == s.tokens && encodings.is_subset_of(s.encodings));
}

string state::to_string(const graph &g)
{
	string result = "{";
	for (int i = 0; i < (int)tokens.size(); i++)
	{
		if (i != 0)
			result += " ";
		result += ::to_string(tokens[i].index);
	}
	result += "} " + export_expression_hfactor(encodings, g).to_string();
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

bool operator==(state s1, state s2)
{
	return s1.tokens == s2.tokens && s1.encodings == s2.encodings;
}

bool operator!=(state s1, state s2)
{
	return s1.tokens != s2.tokens || s1.encodings != s2.encodings;
}

}
