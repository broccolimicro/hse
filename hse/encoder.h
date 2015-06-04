/*
 * encoder.h
 *
 *  Created on: May 7, 2015
 *      Author: nbingham
 */

#include "graph.h"

#ifndef hse_encoder_h
#define hse_encoder_h

namespace hse
{

struct conflict
{
	conflict();
	conflict(term_index index, int sense, vector<int> implicant, vector<int> region);
	~conflict();

	term_index index;
	int sense;
	vector<int> implicant;
	vector<int> region;
};

struct suspect
{
	suspect();
	suspect(int sense, vector<int> first, vector<int> second);
	~suspect();

	int sense;
	vector<int> first;
	vector<int> second;
};

struct encoder
{
	encoder();
	encoder(graph *base);
	~encoder();

	vector<conflict> conflicts;
	vector<suspect> suspects;
	graph *base;

	void check(bool senseless = false);
};

}

#endif
