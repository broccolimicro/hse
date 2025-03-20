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
	void elaborate(graph &g, bool annotate_ghosts = false, bool record_predicates = true, bool report_progress = false);
	graph to_state_graph(graph &g, bool report_progress = false);
	graph to_petri_net(graph &g, bool report_progress = false);
}

#endif
