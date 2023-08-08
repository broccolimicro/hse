/*
 * encoder.h
 *
 *  Created on: May 7, 2015
 *      Author: nbingham
 */

#include <common/standard.h>
#include "state.h"

#ifndef hse_encoder_h
#define hse_encoder_h

namespace hse
{

struct graph;

struct conflict
{
	conflict();
	conflict(term_index index, int sense, vector<petri::iterator> implicant, vector<petri::iterator> region);
	~conflict();

	// index of the cube in the transition and the transition in graph
	term_index index;

	// up conflict or down conflict or non-directional
	int sense;

	// input places leading to transition
	vector<petri::iterator> implicant;

	// region of nodes the transition can fire in incorrectly
	vector<petri::iterator> region;
};

struct suspect
{
	suspect();
	suspect(int sense, vector<petri::iterator> first, vector<petri::iterator> second);
	~suspect();

	int sense;
	vector<petri::iterator> first;
	vector<petri::iterator> second;
};

struct encoder
{
	encoder();
	encoder(graph *base);
	~encoder();

	vector<vector<petri::iterator> > conflict_regions;
	vector<vector<petri::iterator> > suspect_regions;

	vector<conflict> conflicts;
	vector<suspect> suspects;
	graph *base;

	void add_conflict(int tid, int term, int sense, vector<petri::iterator> prev, petri::iterator node);
	void add_suspect(vector<petri::iterator> i, petri::iterator j, int sense);
	void check(bool senseless = false, bool report_progress = false);
};

}

#endif
