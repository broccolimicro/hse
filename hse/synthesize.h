/*
 * synthesize.h
 *
 *  Created on: Aug 5, 2023
 *      Author: nbingham
 */

#pragma once

#include "graph.h"
#include <prs/production_rule.h>
#include <interpret_boolean/export.h>

namespace hse
{

struct gate
{
	int reset;
	vector<petri::iterator> tids[2];
	boolean::cover implicant[2];
	boolean::cover exclusion[2];
	boolean::cover holding[2];
	boolean::cover keeper[2];

	bool is_combinational();
	
	void weaken_espresso();
	void weaken_brute_force(int var, ucs::variable_set &v);
};

void guard_weakening(graph &proc, prs::production_rule_set *out, ucs::variable_set &v, bool senseless, bool report_progress = false);

}
