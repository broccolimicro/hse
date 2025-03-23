#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <hse/graph.h>
#include <hse/state.h>
#include <hse/simulator.h>

#include "helpers.h"

using namespace hse;
using namespace std;

TEST(Simulator, SimpleLoop) {
	// Create a graph with places, transitions, and initial state
	graph g = parse_hse_string("x-; *[x+; x-]");
	EXPECT_EQ(g.netCount(), 1);
  
	int x = g.netIndex("x");
	EXPECT_EQ(x, 0);
	EXPECT_EQ(g.netAt(x), (pair<string, int>("x", 0)));

	// Create simulator
	simulator sim(&g, g.reset[0]);
  
	// Verify initial state
	EXPECT_EQ(sim.tokens.size(), 1u);
	EXPECT_EQ(sim.encoding, boolean::cover(x, 0)); 
	
	// Get enabled transitions
	// Verify that the correct transition is enabled
	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 1u);
	EXPECT_EQ(sim.ready[0].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second], boolean::cube(x, 1));
	
	// Find and fire transition t0
	sim.fire(0); 
	
	// Verify token moved from p0 to p1
	EXPECT_EQ(sim.tokens.size(), 1u);
	EXPECT_EQ(sim.encoding, boolean::cover(x, 1)); 
	
	// Get newly enabled transitions
	// Verify t1 is now enabled
	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 1u);
	EXPECT_EQ(sim.ready[0].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second], boolean::cube(x, 0));
	
	// Find and fire transition t1
	sim.fire(0); 
	
	// Verify token moved from p1 to p0
	EXPECT_EQ(sim.tokens.size(), 1u);
	EXPECT_EQ(sim.encoding, boolean::cover(x, 0)); 
	
	// Back to beginning and we can loop indefinitely
	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 1u);
	EXPECT_EQ(sim.ready[0].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second], boolean::cube(x, 1));
}
/*
// Test 3: Reset State Test
TEST(Simulator, ResetState) {
	// Create a graph with initial state
	graph g;
	
	auto p = g.create(place(), 3);
	auto t = g.create(transition(), 2);
	
	g.connect({p[0], t[0], p[1], t[1], p[2]});
	
	// Set initial state with token on p0
	vector<petri::token> tokens = {petri::token(p[0].index)};
	state initial_state(tokens, boolean::cover(1));
	g.reset.push_back(initial_state);
	
	// Create simulator and run some transitions
	simulator sim(&g, initial_state);
	
	// Get enabled transitions
	int enabled_count = sim.enabled();
	EXPECT_GT(enabled_count, 0);
	
	// Find and fire transition t0
	int t0_ready_index = -1;
	for (int i = 0; i < (int)sim.ready.size(); i++) {
		if (sim.loaded[sim.ready[i].first].index == t[0].index) {
			t0_ready_index = i;
			break;
		}
	}
	EXPECT_GE(t0_ready_index, 0);
	
	sim.fire(t0_ready_index);
	
	// Verify we're at p1
	state current_state = sim.get_state();
	bool has_p1_token = false;
	for (const auto& token : current_state.tokens) {
		if (token.index == p[1].index) {
			has_p1_token = true;
			break;
		}
	}
	EXPECT_TRUE(has_p1_token);
	
	// Create a new simulator with same graph, which should reset to initial state
	simulator sim2(&g, initial_state);
	
	// Verify state matches initial configuration
	current_state = sim2.get_state();
	bool has_p0_token = false;
	for (const auto& token : current_state.tokens) {
		if (token.index == p[0].index) {
			has_p0_token = true;
			break;
		}
	}
	EXPECT_TRUE(has_p0_token);
	
	// Make sure no token at p1 or p2
	for (const auto& token : current_state.tokens) {
		EXPECT_NE(token.index, p[1].index);
		EXPECT_NE(token.index, p[2].index);
	}
}

// Test 4: Concurrent Transitions Test
TEST(Simulator, ConcurrentTransitions) {
	// Create a graph with parallel structure
	graph g;
	
	// Structure:
	//	 p0	 .
	//	/  \	.
	//   t0  t1   .
	//   |	|   .
	//   p1   p2  .
	//	\  /	.
	//	 t2	 .
	//	 |	  .
	//	 p3	 .
	auto p = g.create(place(), 4);
	auto t = g.create(transition(), 3);
	
	g.connect({p[0], t[0], p[1], t[2], p[3]});
	g.connect({p[0], t[1], p[2], t[2]});
	
	// Set initial state with token on p0
	vector<petri::token> tokens = {petri::token(p[0].index)};
	state initial_state(tokens, boolean::cover(1));
	g.reset.push_back(initial_state);
	
	// Create simulator
	simulator sim(&g, initial_state);
	int enabled_count = sim.enabled();
	
	// Both t0 and t1 should be enabled
	EXPECT_EQ(enabled_count, 2);
	
	// Create two copies of simulator to test both paths
	simulator sim1 = sim;
	simulator sim2 = sim;
	
	// Find t0 in first simulator and fire it
	int t0_ready_index = -1;
	for (int i = 0; i < (int)sim1.ready.size(); i++) {
		if (sim1.loaded[sim1.ready[i].first].index == t[0].index) {
			t0_ready_index = i;
			break;
		}
	}
	EXPECT_GE(t0_ready_index, 0);
	sim1.fire(t0_ready_index);
	
	// Verify token at p1 in sim1
	state state1 = sim1.get_state();
	bool has_p1_token = false;
	for (const auto& token : state1.tokens) {
		if (token.index == p[1].index) {
			has_p1_token = true;
			break;
		}
	}
	EXPECT_TRUE(has_p1_token);
	
	// Find t1 in second simulator and fire it
	int t1_ready_index = -1;
	for (int i = 0; i < (int)sim2.ready.size(); i++) {
		if (sim2.loaded[sim2.ready[i].first].index == t[1].index) {
			t1_ready_index = i;
			break;
		}
	}
	EXPECT_GE(t1_ready_index, 0);
	sim2.fire(t1_ready_index);
	
	// Verify token at p2 in sim2
	state state2 = sim2.get_state();
	bool has_p2_token = false;
	for (const auto& token : state2.tokens) {
		if (token.index == p[2].index) {
			has_p2_token = true;
			break;
		}
	}
	EXPECT_TRUE(has_p2_token);
	
	// Fire t2 in both simulators
	// First find and fire t2 in sim1
	sim1.enabled();
	int t2_ready_index1 = -1;
	for (int i = 0; i < (int)sim1.ready.size(); i++) {
		if (sim1.loaded[sim1.ready[i].first].index == t[2].index) {
			t2_ready_index1 = i;
			break;
		}
	}
	EXPECT_GE(t2_ready_index1, 0);
	sim1.fire(t2_ready_index1);
	
	// Then find and fire t2 in sim2
	sim2.enabled();
	int t2_ready_index2 = -1;
	for (int i = 0; i < (int)sim2.ready.size(); i++) {
		if (sim2.loaded[sim2.ready[i].first].index == t[2].index) {
			t2_ready_index2 = i;
			break;
		}
	}
	EXPECT_GE(t2_ready_index2, 0);
	sim2.fire(t2_ready_index2);
	
	// Both should end up in the same final state with token at p3
	state final_state1 = sim1.get_state();
	state final_state2 = sim2.get_state();
	
	bool has_p3_token1 = false;
	for (const auto& token : final_state1.tokens) {
		if (token.index == p[3].index) {
			has_p3_token1 = true;
			break;
		}
	}
	
	bool has_p3_token2 = false;
	for (const auto& token : final_state2.tokens) {
		if (token.index == p[3].index) {
			has_p3_token2 = true;
			break;
		}
	}
	
	EXPECT_TRUE(has_p3_token1);
	EXPECT_TRUE(has_p3_token2);
	
	// Check if both final states have the same tokens
	// Note: This may not be a full state equality check, but checks token positions
	EXPECT_EQ(final_state1.tokens.size(), final_state2.tokens.size());
	
	// Sort both token vectors for comparison
	vector<int> indices1, indices2;
	for (const auto& token : final_state1.tokens) {
		indices1.push_back(token.index);
	}
	for (const auto& token : final_state2.tokens) {
		indices2.push_back(token.index);
	}
	
	sort(indices1.begin(), indices1.end());
	sort(indices2.begin(), indices2.end());
	
	EXPECT_EQ(indices1, indices2);
}

// Test 5: State Comparison Test
TEST(Simulator, StateComparison) {
	// Create states to compare
	vector<petri::token> tokens1 = {petri::token(0), petri::token(1)};
	vector<petri::token> tokens2 = {petri::token(2), petri::token(3)};
	vector<petri::token> tokens3 = {petri::token(1), petri::token(4)};
	
	state s1(tokens1, boolean::cover(1));
	state s2(tokens2, boolean::cover(1));
	state s3(tokens3, boolean::cover(1));
	
	// Test equality
	state s1_copy = s1;
	EXPECT_TRUE(s1 == s1_copy);
	EXPECT_FALSE(s1 == s2);
	
	// Test inequality
	EXPECT_TRUE(s1 != s2);
	EXPECT_FALSE(s1 != s1_copy);
	
	// Test is_subset_of
	// Create states with subset relationships
	vector<petri::token> subset_tokens = {petri::token(0)};
	state subset(subset_tokens, boolean::cover(1));
	
	EXPECT_TRUE(subset.is_subset_of(s1));
	EXPECT_FALSE(s1.is_subset_of(subset));
}

// Test 6: Multiple Tokens Test
TEST(Simulator, MultipleTokens) {
	// Create a graph with places
	graph g;
	auto p = g.create(place(), 2);
	auto t = g.create(transition(), 1);
	
	g.connect({p[0], t[0], p[1]});
	
	// Create a state with multiple tokens on the same place
	vector<petri::token> tokens = {petri::token(p[0].index), petri::token(p[0].index)};
	state s(tokens, boolean::cover(1));
	
	// Check the number of tokens (either 1 or 2 depending on implementation)
	if (s.tokens.size() == 2) {
		// Implementation allows duplicate tokens
		EXPECT_EQ(s.tokens.size(), 2);
		
		// Check both tokens are at p[0]
		int p0_token_count = 0;
		for (const auto& token : s.tokens) {
			if (token.index == p[0].index) {
				p0_token_count++;
			}
		}
		EXPECT_EQ(p0_token_count, 2);
	} else {
		// Implementation treats tokens as a set (no duplicates)
		EXPECT_EQ(s.tokens.size(), 1);
		EXPECT_EQ(s.tokens[0].index, p[0].index);
	}
}

// Test 7: Token Iterator Test
TEST(Simulator, TokenIterator) {
	// Create a state with tokens
	vector<petri::token> tokens = {petri::token(0), petri::token(1), petri::token(2)};
	state s(tokens, boolean::cover(1));
	
	// Count tokens manually
	EXPECT_EQ(s.tokens.size(), 3);
	
	// Collect token indices for comparison
	vector<int> expected_indices = {0, 1, 2};
	vector<int> actual_indices;
	
	for (const auto& token : s.tokens) {
		actual_indices.push_back(token.index);
	}
	
	// Sort to make comparison deterministic
	sort(actual_indices.begin(), actual_indices.end());
	
	EXPECT_EQ(actual_indices.size(), expected_indices.size());
	for (size_t i = 0; i < actual_indices.size(); i++) {
		EXPECT_EQ(actual_indices[i], expected_indices[i]);
	}
}*/ 
