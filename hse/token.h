/*
 * token.h
 *
 *  Created on: Feb 2, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include <common/text.h>
#include <boolean/cube.h>
#include <boolean/variable.h>

#include "node.h"

#ifndef hse_token_h
#define hse_token_h

namespace hse
{

struct reset_token;

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct local_token
{
	local_token();
	local_token(int index, bool remotable);
	local_token(int index, vector<int> guard, bool remotable);
	local_token(const reset_token &t);
	~local_token();

	int index;
	vector<int> guard;
	bool remotable;

	local_token &operator=(const reset_token &t);
	string to_string();
};

bool operator<(local_token i, local_token j);
bool operator>(local_token i, local_token j);
bool operator<=(local_token i, local_token j);
bool operator>=(local_token i, local_token j);
bool operator==(local_token i, local_token j);
bool operator!=(local_token i, local_token j);

/* A remote token is a token that does not maintain its own state but acts only as an environment for a local token.
 */
struct remote_token
{
	remote_token();
	remote_token(int index);
	remote_token(const reset_token &t);
	~remote_token();

	int index;

	remote_token &operator=(const reset_token &t);
	string to_string();
};

bool operator<(remote_token i, remote_token j);
bool operator>(remote_token i, remote_token j);
bool operator<=(remote_token i, remote_token j);
bool operator>=(remote_token i, remote_token j);
bool operator==(remote_token i, remote_token j);
bool operator!=(remote_token i, remote_token j);

/* A local token maintains its own local state information and cannot access global state
 * except through transformations applied to its local state.
 */
struct reset_token
{
	reset_token();
	reset_token(int index, bool remotable);
	reset_token(const remote_token &t);
	reset_token(const local_token &t);
	~reset_token();

	int index;
	bool remotable;

	reset_token &operator=(const remote_token &t);
	reset_token &operator=(const local_token &t);
	string to_string(const boolean::variable_set &variables);
};

bool operator<(reset_token i, reset_token j);
bool operator>(reset_token i, reset_token j);
bool operator<=(reset_token i, reset_token j);
bool operator>=(reset_token i, reset_token j);
bool operator==(reset_token i, reset_token j);
bool operator!=(reset_token i, reset_token j);

struct state
{
	state();
	state(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings);
	state(vector<local_token> tokens, vector<term_index> environment, boolean::cover encodings);
	~state();

	vector<reset_token> tokens;
	vector<term_index> environment;
	boolean::cover encodings;

	state &merge(int relation, const state &s);
	bool is_subset_of(const state &s);
	string to_string(const boolean::variable_set &variables);
};

bool operator<(state s1, state s2);
bool operator>(state s1, state s2);
bool operator<=(state s1, state s2);
bool operator>=(state s1, state s2);
bool operator==(state s1, state s2);
bool operator!=(state s1, state s2);

template<int num_buckets>
struct state_map
{
	state_map() {count = 0;}
	~state_map() {}

	vector<state> buckets[num_buckets];
	int count;

	uint32_t SuperFastHash (const char * data, int len) {
	uint32_t hash = len, tmp;
	int rem;

	    if (len <= 0 || data == NULL) return 0;

	    rem = len & 3;
	    len >>= 2;

	    /* Main loop */
	    for (;len > 0; len--) {
	        hash  += (*((const uint16_t *) (data)));
	        tmp    = ((*((const uint16_t *) (data+2))) << 11) ^ hash;
	        hash   = (hash << 16) ^ tmp;
	        data  += 2*sizeof (uint16_t);
	        hash  += hash >> 11;
	    }

	    /* Handle end cases */
	    switch (rem) {
	        case 3: hash += (*((const uint16_t *) (data)));
	                hash ^= hash << 16;
	                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
	                hash += hash >> 11;
	                break;
	        case 2: hash += (*((const uint16_t *) (data)));
	                hash ^= hash << 11;
	                hash += hash >> 17;
	                break;
	        case 1: hash += (signed char)*data;
	                hash ^= hash << 10;
	                hash += hash >> 1;
	    }

	    /* Force "avalanching" of final 127 bits */
	    hash ^= hash << 3;
	    hash += hash >> 5;
	    hash ^= hash << 4;
	    hash += hash >> 17;
	    hash ^= hash << 25;
	    hash += hash >> 6;

	    return hash;
	}

	unsigned int hash(state s)
	{
		return SuperFastHash((const char*)s.tokens.data(), s.tokens.size()*sizeof(reset_token)) ^
				SuperFastHash((const char*)s.environment.data(), s.environment.size()*sizeof(term_index));
	}

	bool insert(state s)
	{
		int bucket = hash(s)%num_buckets;
		vector<state>::iterator i = lower_bound(buckets[bucket].begin(), buckets[bucket].end(), s);
		if (i != buckets[bucket].end() && *i == s)
		{
			if (s.encodings.is_subset_of(i->encodings))
				return true;
			else
			{
				i->merge(choice, s);
				return false;
			}
		}
		else
		{
			buckets[bucket].insert(i, s);
			count++;
			return false;
		}
	}

	string check()
	{
		int max_diff = 0;
		for (int i = 0; i < num_buckets; i++)
			if ((int)buckets[i].size() > max_diff)
				max_diff = buckets[i].size();
		return ::to_string(max_diff) + "/" + ::to_string(count);
	}
};

/* This stores all the information necessary to fire an enabled transition: the local
 * and remote tokens that enable it, and the total state of those tokens.
 */
struct enabled_transition : term_index
{
	enabled_transition();
	enabled_transition(int index);
	~enabled_transition();

	vector<int> tokens;
	vector<term_index> history;
	vector<int> guard;
	bool vacuous;
};

struct enabled_environment : term_index
{
	enabled_environment();
	enabled_environment(int index);
	~enabled_environment();

	vector<int> tokens;
};

struct graph;

struct instability
{
	instability();
	instability(term_index effect, vector<term_index> cause);
	~instability();

	term_index effect;
	vector<term_index> cause;

	string to_string(const hse::graph &g, const boolean::variable_set &v);
};

bool operator<(instability i, instability j);
bool operator>(instability i, instability j);
bool operator<=(instability i, instability j);
bool operator>=(instability i, instability j);
bool operator==(instability i, instability j);
bool operator!=(instability i, instability j);

struct interference : pair<term_index, term_index>
{
	interference();
	interference(term_index first, term_index second);
	~interference();

	string to_string(const hse::graph &g, const boolean::variable_set &v);
};

struct deadlock : state
{
	deadlock();
	deadlock(const state &s);
	deadlock(vector<reset_token> tokens, vector<term_index> environment, boolean::cover encodings);
	deadlock(vector<local_token> tokens, vector<term_index> environment, boolean::cover encodings);
	~deadlock();

	string to_string(const boolean::variable_set &v);
};

}

#endif
