#include "expression.h"
#include <parse_cog/expression.h>
#include <interpret_boolean/export.h>

namespace hse {

string emit_composition(boolean::cover expr, ucs::ConstNetlist nets) {
	return export_composition<parse_cog::simple_composition>(expr, nets).to_string();
}

string emit_composition(boolean::cube expr, ucs::ConstNetlist nets) {
	return export_composition<parse_cog::simple_composition>(expr, nets).to_string();
}

string emit_expression(boolean::cover expr, ucs::ConstNetlist nets) {
	return export_expression<parse_cog::expression>(expr, nets).to_string();
}

string emit_expression(boolean::cube expr, ucs::ConstNetlist nets) {
	return export_expression<parse_cog::expression>(expr, nets).to_string();
}

string emit_expression_hfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return export_expression_hfactor<parse_cog::expression>(expr, nets).to_string();
}

string emit_expression_xfactor(boolean::cover expr, ucs::ConstNetlist nets) {
	return export_expression_xfactor<parse_cog::expression>(expr, nets).to_string();
}

}
