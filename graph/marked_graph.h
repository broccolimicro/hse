/*
 * marked_graph.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include "graph.h"

#ifndef graph_marked_graph_h
#define graph_marked_graph_h

namespace graph
{

struct token
{
	graph::iterator index;
};

struct marked_graph;

struct marking
{
	vector<token*> tokens;
	marked_graph *base;
};

struct marked_graph : graph
{
	marking M0;
};

}

#endif
