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

void guard_weakening(graph &proc, prs::production_rule_set *out, const ucs::variable_set &v, bool senseless, bool report_progress = false);
void guard_strengthening(graph &proc, prs::production_rule_set *out);

}
