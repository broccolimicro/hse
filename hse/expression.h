#pragma once

#include <boolean/cover.h>
#include <common/net.h>

namespace hse {

string emit_composition(boolean::cover expr, ucs::ConstNetlist nets);
string emit_composition(boolean::cube expr, ucs::ConstNetlist nets);
string emit_expression(boolean::cover expr, ucs::ConstNetlist nets);
string emit_expression(boolean::cube expr, ucs::ConstNetlist nets);
string emit_expression_hfactor(boolean::cover expr, ucs::ConstNetlist nets);
string emit_expression_xfactor(boolean::cover expr, ucs::ConstNetlist nets);

}
