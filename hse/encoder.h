#pragma once

#include <common/standard.h>
#include "state.h"
#include <petri/path.h>

namespace hse
{

struct graph;

// Our goal is to convert the HSE graph into production rules. In doing so, we
// lose the syntactic constraints on event orderings and replace them
// exclusively with conditions on the current state encoding. In order to do
// this, we need to make sure that there is enough information in those state
// encodings to disambiguate states in which a transition should fire from
// states in which a transition should not. When there is not enough
// information to do so, then that is a state conflict. More specifically, a
// conflict is a contiguous region of places in which a transition can fire
// when it should not. Our goal is to eliminate these conflicts by creating a
// state variable in which the variable's value is different between those two
// regions. Transitions on this new variable create a graph cut that separates
// the states in which the transition should fire from the states in which it
// should not.
struct conflict
{
	conflict();
	conflict(term_index index, int sense, vector<petri::iterator> region, boolean::cover encoding);
	~conflict();

	// index of the transition in the graph and the cube in the transition
	term_index index;
	boolean::cover encoding;

	// CMOS logic is inverted. PMOS transistors pull the output high when the
	// gate is low and NMOS transistors pull the output low when the gate is
	// high. Therefore, upgoing transitions must use inverted literals (~x -> y+)
	// and downgoing transitions must use non-inverted literals (x -> y-).

	// There are three types of conflicts depending on how we're approaching the problem:
	
	enum {
		// a non-directional conflict ignores the fact that CMOS logic is inverting
		// and uses all of the state information in the predicates of the two
		// places to determine whether they are sufficiently disambiguated.
		NONE = -1,
		// a downgoing conflict ignores all inverted literals in the predicates of
		// the two places.
		DOWN = 0,
		// an upgoing conflict ignores all non-inverted literals in the predicates
		// of the two places.
		UP = 1
	};
	int sense;

	// region of places the transition can fire in incorrectly
	vector<petri::iterator> region;
};

// This structure is responsible for checking an HSE for conflicts and then
// solving those conflicts with state-variable insertion.
struct encoder
{
	encoder();
	encoder(graph *base, ucs::variable_set *variables);
	~encoder();

	graph *base;
	ucs::variable_set *variables;

	// conflict_regions is like conflict::region except that it includes all of
	// the neighboring nodes so that we can check for intersection to make
	// clustering easier. These two members are only to be used by the
	// add_conflict() function. In a way, they are just temporary variables.
	vector<vector<petri::iterator> > conflict_regions;

	// The list of conflicts found in the HSE by check().
	vector<conflict> conflicts;


	void add_conflict(int tid, int term, int sense, petri::iterator node, boolean::cover encoding);

	void check(bool senseless = false, bool report_progress = false);

	int score_insertion(int sense, vector<petri::iterator> pos, const petri::path_set &dontcare);
	int find_insertion(int sense, vector<petri::iterator> pos, const petri::path_set &dontcare, vector<vector<petri::iterator> > *result);
	int find_insertions(int sense, vector<vector<petri::iterator> > pos, const petri::path_set &dontcare, vector<vector<petri::iterator> > *result);

	void insert_state_variables();
};

}

