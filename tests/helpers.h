#pragma once

#include <hse/graph.h>
#include <parse/parse.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse/default/new_line.h>
#include <parse_chp/composition.h>
#include <interpret_hse/import.h>
#include <string>

namespace hse {

graph parse_hse_string(const std::string &hse_str);

} // namespace hse
