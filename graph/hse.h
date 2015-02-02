/*
 * hse.h
 *
 *  Created on: Feb 1, 2015
 *      Author: nbingham
 */

#include "petri.h"

#ifndef graph_hse_h
#define graph_hse_h

namespace graph
{
struct hse_place : petri_place
{
	boolean::cover effective;
};

struct hse_transition : petri_transition
{
	bool active;
};

struct hse : petri_net
{

};
}

#endif
