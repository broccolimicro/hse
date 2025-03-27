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

TEST(Simulator, Parallel) {
	// Create a graph with places, transitions, and initial state
	graph g = parse_hse_string("x-,y-; *[x+,y+; x-,y-]");
	EXPECT_EQ(g.netCount(), 2);
  
	int x = g.netIndex("x");
	int y = g.netIndex("y");
	EXPECT_GE(x, 0);
	EXPECT_GE(y, 0);

	// Create simulator
	simulator sim(&g, g.reset[0]);
  
	// Verify initial state
	EXPECT_EQ(sim.tokens.size(), 4u);
	EXPECT_EQ(sim.encoding, boolean::cover(x, 0) & boolean::cover(y, 0)); 
	
	// Get enabled transitions
	// Verify that the correct transition is enabled
	EXPECT_EQ(sim.enabled(), 2);
	EXPECT_EQ(sim.ready.size(), 2u);
	EXPECT_EQ(sim.loaded.size(), 2u);
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second] | g.transitions[sim.loaded[sim.ready[1].first].index].local_action.cubes[sim.ready[1].second], boolean::cover(x, 1) | boolean::cover(y, 1));
	
	// Find and fire transition t0
	sim.fire(0); 

	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 1u);
	EXPECT_GE(sim.ready[0].first, 0);

	sim.fire(0);

	EXPECT_EQ(sim.enabled(), 2);
	EXPECT_EQ(sim.ready.size(), 2u);
	EXPECT_EQ(sim.loaded.size(), 2u);
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second] | g.transitions[sim.loaded[sim.ready[1].first].index].local_action.cubes[sim.ready[1].second], boolean::cover(x, 0) | boolean::cover(y, 0));

	sim.fire(0); 

	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 1u);
	EXPECT_GE(sim.ready[0].first, 0);

	sim.fire(0);

	// Back to beginning
	EXPECT_EQ(sim.enabled(), 2);
	EXPECT_EQ(sim.ready.size(), 2u);
	EXPECT_EQ(sim.loaded.size(), 2u);
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second] | g.transitions[sim.loaded[sim.ready[1].first].index].local_action.cubes[sim.ready[1].second], boolean::cover(x, 1) | boolean::cover(y, 1));
}

TEST(Simulator, Choice) {
	// Create a graph with places, transitions, and initial state
	graph g = parse_hse_string("x-,y-; *[[1->x+:1->y+]; x-,y-]");
	EXPECT_EQ(g.netCount(), 2);
  
	int x = g.netIndex("x");
	int y = g.netIndex("y");
	EXPECT_GE(x, 0);
	EXPECT_GE(y, 0);

	// Create simulator
	simulator sim(&g, g.reset[0]);
  
	// Verify initial state
	EXPECT_EQ(sim.tokens.size(), 2u);
	EXPECT_EQ(sim.encoding, boolean::cover(x, 0) & boolean::cover(y, 0)); 
	
	// Get enabled transitions
	// Verify that the correct transition is enabled
	EXPECT_EQ(sim.enabled(), 2);
	EXPECT_EQ(sim.ready.size(), 2u);
	EXPECT_EQ(sim.loaded.size(), 2u);
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second] | g.transitions[sim.loaded[sim.ready[1].first].index].local_action.cubes[sim.ready[1].second], boolean::cover(x, 1) | boolean::cover(y, 1));
	
	// Find and fire transition t0
	sim.fire(0); 

	EXPECT_EQ(sim.enabled(), 1);
	EXPECT_EQ(sim.ready.size(), 1u);
	EXPECT_EQ(sim.loaded.size(), 2u);
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);

	sim.fire(0); 

	// Other transition is vacuous, we should be back to beginning
	EXPECT_EQ(sim.enabled(), 2);
	EXPECT_EQ(sim.ready.size(), 2u);
	EXPECT_EQ(sim.loaded.size(), 3u); // x+, y+, vacuous x-
	EXPECT_GE(sim.ready[0].first, 0);
	EXPECT_GE(sim.ready[1].first, 0);
	EXPECT_EQ(g.transitions[sim.loaded[sim.ready[0].first].index].local_action.cubes[sim.ready[0].second] | g.transitions[sim.loaded[sim.ready[1].first].index].local_action.cubes[sim.ready[1].second], boolean::cover(x, 1) | boolean::cover(y, 1));
}

