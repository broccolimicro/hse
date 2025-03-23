
#include <hse/graph.h>
#include <parse/tokenizer.h>
#include <parse/parse.h>

#include "helpers.h"

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

	hg.post_process(true);
	hg.check_variables();
  
  return hg;
}

} // namespace hse 
