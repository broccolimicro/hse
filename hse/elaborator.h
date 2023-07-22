/*
 * elaborator.h
 *
 *  Created on: Aug 13, 2015
 *      Author: nbingham
 */

#include "graph.h"
#include "simulator.h"

#ifndef elaborator_h
#define elaborator_h

namespace hse
{
	// Generated through simulation
	void elaborate(graph &g, const ucs::variable_set &variables, bool find_parallel = true, bool report_progress = false);
	graph to_state_graph(const graph &g, const ucs::variable_set &variables, bool report_progress = false);
	graph to_petri_net(const graph &g, const ucs::variable_set &variables, bool report_progress = false);
}

#endif
