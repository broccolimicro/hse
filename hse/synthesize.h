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

// A gate is a pull up and pull down network that drives a specific
// variable/circuit node. The variable in question is specified by this gate's
// location in the array of gates for this circuit, and is not recorded in the
// gate struct specifically. Each gate is synthesized from the fully elaborated
// HSE using the place predicates.
struct gate
{
	// The value of this variable on reset.
	int reset;

	// Each of these are an array of size 2. Index 0 is the pull down
	// network and index 1 is the pull up network.

	// The list of all transitions that drive the node low or high.
	vector<petri::iterator> tids[2];

	// The set of states in which the pull down or pull up network
	// must be active and driving the variable through a transition.
	boolean::cover implicant[2];

	// The set of states in which the pull down or pull up network
	// must be inactive.
	boolean::cover exclusion[2];
	
	// The set of states in which the variable needs to hold a low or
	// a high value.
	boolean::cover holding[2];

	bool is_combinational();
	
	void weaken_espresso();
	void weaken_brute_force();
};

void guard_weakening(graph &proc, prs::production_rule_set *out, ucs::variable_set &v, bool senseless, bool report_progress = false);

/*void build_staticizers(graph &proc);
void build_precharge();
void harden_state();
void build_shared_gates();*/

}
