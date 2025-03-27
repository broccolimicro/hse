#include "helpers.h"

#include <parse/tokenizer.h>
#include <parse/parse.h>
#include <parse/default/block_comment.h>
#include <parse/default/line_comment.h>
#include <parse/default/new_line.h>
#include <parse_chp/composition.h>
#include <interpret_hse/import.h>
#include <string>

#include <interpret_boolean/export.h>

namespace hse {

// @brief Parse an HSE string and convert it to an HSE graph
// @param hse_str The HSE string to parse
// @return The resulting HSE graph
graph parse_hse_string(const std::string &hse_str) {
  graph hg;
  
  // Setup tokenizer
  tokenizer tokens;
  tokens.register_token<parse::block_comment>(false);
  tokens.register_token<parse::line_comment>(false);
  parse_chp::composition::register_syntax(tokens);
  
  // Insert the string into the tokenizer
  tokens.insert("string_input", hse_str, nullptr);
  
  // Parse the tokens
  tokens.increment(false);
  tokens.expect<parse_chp::composition>();
  if (tokens.decrement(__FILE__, __LINE__)) {
    parse_chp::composition syntax(tokens);
    hg.merge(hse::parallel, hse::import_hse(syntax, 0, &tokens, true));
  }

	hg.post_process(true, false, false);
	hg.check_variables();
  
  return hg;
}

void print_enabled(const graph &g, const simulator &sim) {
	for (int i = 0; i < (int)sim.ready.size(); i++) {
		printf("(%d) T%d.%d:%s->%s\n", i, sim.loaded[sim.ready[i].first].index, sim.ready[i].second, export_expression(g.transitions[sim.loaded[sim.ready[i].first].index].guard, g).to_string().c_str(), export_composition(g.transitions[sim.loaded[sim.ready[i].first].index].local_action[sim.ready[i].second], g).to_string().c_str());
		if (sim.loaded[sim.ready[i].first].vacuous) {
			printf("\tvacuous");
		}
		if (!sim.loaded[sim.ready[i].first].stable) {
			printf("\tunstable");
		}
		printf("\n");
	}
	printf("\n");
}

} // namespace hse 
