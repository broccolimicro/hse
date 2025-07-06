/*
 * This file implements the core data structures for representing and manipulating
 * Handshaking Expansion (HSE) graphs. HSE graphs are a type of Petri Net used to
 * model asynchronous circuits, where transitions are augmented with guards and
 * actions that model the behavior of the circuit.
 */

#include "graph.h"
#include "simulator.h"
#include <common/message.h>
#include <common/text.h>
#include <common/math.h>
#include <interpret_boolean/export.h>
#include <petri/simulator.h>
#include <petri/state.h>

#include <common/mapping.h>

namespace hse
{

place::place()
{
	arbiter = false;
	synchronizer = false;
}

place::place(boolean::cover predicate)
{
	this->predicate = predicate;
	this->effective = predicate;
	arbiter = false;
	synchronizer = false;
}

place::~place()
{

}

/**
 * @brief Merge two places and combine their properties
 * 
 * Creates a new place by merging two existing places according to the specified composition rule:
 * - For parallel or sequence compositions: Predicates and effective predicates are combined with AND
 * - For choice compositions: Predicates and effective predicates are combined with OR
 * Arbiter and synchronizer flags are combined with OR in all cases.
 * 
 * @param composition Type of composition (parallel, choice, or sequence)
 * @param p0 First place to merge
 * @param p1 Second place to merge
 * @return A new place that represents the composition of p0 and p1
 */
place place::merge(int composition, const place &p0, const place &p1)
{
	place result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.effective = p0.effective & p1.effective;
		result.predicate = p0.predicate & p1.predicate;
	}
	else if (composition == petri::choice)
	{
		result.effective = p0.effective | p1.effective;
		result.predicate = p0.predicate | p1.predicate;
	}
	result.arbiter = (p0.arbiter or p1.arbiter);
	result.synchronizer = (p0.synchronizer or p1.synchronizer);
	return result;
}

ostream &operator<<(ostream &os, const place &p) {
	os << p.predicate;
	return os;
}

/**
 * @brief Constructor for a transition with specified characteristics
 * 
 * @param assume Assumption that is made about the state after the transition is fired
 * @param guard Condition that must be satisfied for the transition to fire
 * @param local_action Effect of the transition on the local state
 * @param remote_action Effect of the transition on the remote state (variables in another isochronic region)
 */
transition::transition(boolean::cover assume, boolean::cover guard, boolean::cover local_action, boolean::cover remote_action)
{
	this->assume = assume;
	this->guard = guard;
	this->local_action = local_action;
	this->remote_action = remote_action;
	this->ghost = 1;
}

transition::~transition()
{

}

/**
 * @brief Subdivide a transition into a simpler transition containing a single term
 * 
 * Creates a new transition containing only a specific term from the local and remote actions
 * of the original transition. This is useful for breaking down complex transitions.
 * 
 * @param term Index of the term to extract
 * @return A new transition containing only the specified term
 */
transition transition::subdivide(int term) const
{
	if (term < (int)local_action.cubes.size() && term < (int)remote_action.cubes.size())
		return transition(assume, guard, boolean::cover(local_action.cubes[term]), boolean::cover(remote_action.cubes[term]));
	else if (term < (int)local_action.cubes.size())
		return transition(assume, guard, boolean::cover(local_action.cubes[term]));
	else
		return transition(assume, guard);
}

/**
 * @brief Merge two transitions according to a composition rule
 * 
 * Creates a new transition by merging the attributes of two existing transitions:
 * - For parallel or sequence: Attributes are combined with AND
 * - For choice: Attributes are combined with OR
 * 
 * @param composition Type of composition (parallel, choice, or sequence)
 * @param t0 First transition to merge
 * @param t1 Second transition to merge
 * @return A new transition representing the composition
 */
transition transition::merge(int composition, const transition &t0, const transition &t1)
{
	transition result;
	if (composition == petri::parallel || composition == petri::sequence)
	{
		result.assume = t0.assume & t1.assume;
		result.guard = t0.guard & t1.guard;
		result.local_action = t0.local_action & t1.local_action;
		result.remote_action = t0.remote_action & t1.remote_action;
	}
	else if (composition == petri::choice)
	{
		result.assume = t0.assume | t1.assume;
		result.guard = t0.guard | t1.guard;
		result.local_action = t0.local_action | t1.local_action;
		result.remote_action = t0.remote_action | t1.remote_action;
	}
	return result;
}

/**
 * @brief Determine if two transitions can be merged
 * 
 * Transitions are mergeable under specific conditions depending on the composition type:
 * - For sequence: t0's local action must be a tautology
 * - For choice/parallel: Either guards and assumptions must be equal, or local actions must be equal
 * 
 * @param composition Type of composition (parallel, choice, or sequence)
 * @param t0 First transition to check
 * @param t1 Second transition to check
 * @return True if transitions can be merged, false otherwise
 */
bool transition::mergeable(int composition, const transition &t0, const transition &t1)
{
	if (composition == petri::sequence) {
		return t0.local_action.is_tautology();
	}

	// choice or parallel
	return (t0.guard == t1.guard and t0.assume == t1.assume) or
				 (t0.local_action == t1.local_action);
}

/**
 * @brief Check if a transition is infeasible (can never fire)
 * 
 * A transition is infeasible if its guard is null or its local action is null.
 * This is a conservative check that returns true only when definitely infeasible.
 * 
 * @return True if the transition is definitely infeasible, false if it might be feasible
 */
bool transition::is_infeasible() const
{
	return guard.is_null() or local_action.is_null();
}

/**
 * @brief Check if a transition is vacuous (has no effect)
 * 
 * A transition is vacuous if its assume, guard, and local_action are all tautologies,
 * meaning it will always be enabled and have no effect on the state of the system.
 * This is a conservative check that returns true only when definitely vacuous.
 * 
 * @return True if the transition is definitely vacuous, false if it might have an effect
 */
bool transition::is_vacuous() const
{
	return assume.is_tautology() and guard.is_tautology() and local_action.is_tautology();
}

ostream &operator<<(ostream &os, const transition &t) {
	os << t.guard << "->" << t.local_action;
	return os;
}


net::net() {
	is_ghost = false;
	region = 0;
}

net::net(string name, int region, bool is_ghost) {
	this->name = name;
	this->region = region;
	this->is_ghost = is_ghost;
}

net::~net() {
}

string net::to_string() const {
	return name + (region != 0 ? "'" + ::to_string(region) : "");
}

graph::graph()
{
}

graph::~graph()
{

}

/**
 * @brief Find the index of a net with the given name and region
 * 
 * Searches for a net by exact name and region match.
 * 
 * @param name The name of the net to find
 * @return The index of the net if found, -1 otherwise
 */
int graph::netIndex(string name) const {
	int region = 0;
	size_t tic = name.rfind('\'');
	if (tic != string::npos) {
		region = std::stoi(name.substr(tic+1));
		name = name.substr(0, tic);
	}

	for (int i = 0; i < (int)nets.size(); i++) {
		if (nets[i].name == name and nets[i].region == region) {
			return i;
		}
	}
	return -1;
}

/**
 * @brief Find or create a net with the given name and region
 * 
 * First tries to find an exact match for the net. If not found and define is true
 * or nets with the same name exist in other regions, creates a new net and connects
 * it to other nets with the same name.
 * 
 * @param name The name of the net to find or create
 * @param region The region for the net
 * @param define Whether to create the net if not found
 * @return The index of the found or created net, or -1 if not found and not created
 */
int graph::netIndex(string name, bool define) {
	int region = 0;
	size_t tic = name.rfind('\'');
	if (tic != string::npos) {
		region = std::stoi(name.substr(tic+1));
		name = name.substr(0, tic);
	}

	vector<int> remote;
	// First try to find the exact net
	for (int i = 0; i < (int)nets.size(); i++) {
		if (nets[i].name == name) {
			remote.push_back(i);
			if (nets[i].region == region) {
				return i;
			}
		}
	}

	// If not found but define is true or we found nets with the same
	// name, create a new net and connect it to the other nets with the
	// same name
	if (define or not remote.empty()) {
		int uid = create(net(name, region));
		for (int i = 0; i < (int)remote.size(); i++) {
			connect_remote(uid, remote[i]);
		}
		return uid;
	}
	return -1;
}

/**
 * @brief Get the name and region of a net by index
 * 
 * @param uid The index of the net
 * @return A pair containing the name and region of the net
 */
string graph::netAt(int uid) const {
	if (uid >= (int)nets.size()) {
		return "";
	}
	return nets[uid].name + (nets[uid].region != 0 ?
		"'" + ::to_string(nets[uid].region) : "");
}

int graph::netCount() const {
	return (int)nets.size();
}

/**
 * @brief Create a new net in the graph
 * 
 * Adds a new net to the graph and initializes its remote connections.
 * If the net is a ghost net, adds it to the ghost_nets list.
 * 
 * @param n The net to create
 * @return The index of the newly created net
 */
int graph::create(net n) {
	int uid = nets.size();
	nets.push_back(n);
	nets.back().remote.push_back(uid);
	if (nets.back().is_ghost) {
		ghost_nets.push_back(uid);
	}
	return uid;
}

/**
 * @brief Connect two nets as remote counterparts
 * 
 * Establishes a remote connection between two nets, making them share
 * the same remote list. This is used to connect nets with the same name
 * across different regions.
 * 
 * @param from Index of the first net
 * @param to Index of the second net
 */
void graph::connect_remote(int from, int to) {
	nets[from].remote.insert(nets[from].remote.end(), nets[to].remote.begin(), nets[to].remote.end());
	sort(nets[from].remote.begin(), nets[from].remote.end());
	nets[from].remote.erase(unique(nets[from].remote.begin(), nets[from].remote.end()), nets[from].remote.end());
	nets[to].remote = nets[from].remote;
}

/**
 * @brief Get all remote net groups
 * 
 * A remote group collects all of the isochronic regions of a net. It is a set of nets that are connected
 * to each other via remote connections. This function identifies all distinct remote groups in the graph.
 * 
 * @return A vector of vectors, where each inner vector contains the indices of nets in one remote group
 */
vector<vector<int> > graph::remote_groups() {
	vector<vector<int> > groups;

	for (int i = 0; i < (int)nets.size(); i++) {
		bool found = false;
		for (int j = 0; j < (int)groups.size() and not found; j++) {
			found = (find(groups[j].begin(), groups[j].end(), i) != groups[j].end());
		}
		if (not found) {
			groups.push_back(nets[i].remote);
		}
	}

	return groups;
}

/**
 * @brief Get a reference to a transition by term index
 * 
 * @param idx A term_index identifying a specific transition
 * @return Reference to the transition
 */
hse::transition &graph::at(term_index idx) {
	return transitions[idx.index];
}

/**
 * @brief Get a reference to a specific term in a transition's local action
 * 
 * @param idx A term_index identifying a specific term in a transition
 * @return Reference to the term (boolean cube)
 */
boolean::cube &graph::term(term_index idx) {
	return transitions[idx.index].local_action[idx.term];
}

petri::mapping graph::merge(graph g) {
	mapping netMap((int)g.nets.size());

	// Add all of the nets and look for duplicates
	int count = (int)nets.size();
	for (int i = 0; i < (int)g.nets.size(); i++) {
		netMap.nets[i] = (int)nets.size();
		vector<int> remote;
		for (int j = 0; j < count; j++) {
			if (nets[j].name == g.nets[i].name) {
				if (nets[j].region == g.nets[i].region) {
					netMap.nets[i] = j;
				}
				remote.push_back(j);
			}
		}

		if (netMap.nets[i] >= (int)nets.size()) {
			nets.push_back(g.nets[i]);
			nets.back().remote = remote;
			if (nets.back().is_ghost) {
				ghost_nets.push_back(i);
			}
		}
	}

	// Fill in the remote nets
	for (int i = 0; i < (int)netMap.nets.size(); i++) {
		int k = netMap.nets[i];
		for (int j = 0; j < (int)g.nets[i].remote.size(); j++) {
			nets[k].remote.push_back(netMap.map(g.nets[i].remote[j]));
		}
		sort(nets[k].remote.begin(), nets[k].remote.end());
		nets[k].remote.erase(unique(nets[k].remote.begin(), nets[k].remote.end()), nets[k].remote.end());
	}

	for (int i = 0; i < (int)g.transitions.size(); i++) {
		g.transitions[i].local_action.apply(netMap);
		g.transitions[i].remote_action.apply(netMap);
		g.transitions[i].guard.apply(netMap);
	}

	for (int i = 0; i < (int)g.places.size(); i++) {
		g.places[i].predicate.apply(netMap);
		g.places[i].effective.apply(netMap);
	}

	// Remap all expressions to new variables
	return super::merge(g);
}

/**
 * @brief Calculate the combined predicate of multiple places
 * 
 * Computes the boolean cover representing the conjunction of all predicates
 * from the given places and input places from the given transitions.
 * 
 * @param pos Vector of iterators pointing to places
 * @return The combined predicate as a boolean cover
 */
boolean::cover graph::predicate(petri::region pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
		}
	}
	result.hide(ghost_nets);
	
	return result;
}

/**
 * @brief Calculate the effective predicate for a place
 * 
 * The effective predicate of a place excludes state encodings that enable any
 * outgoing transition's guard. The effective predicate of a transition is the
 * conjunction of the effective predicates of its input places excluding the
 * guard and action. This helps in synthesizing production rules.
 * 
 * @param i Iterator pointing to a place
 * @param prev Optional pointer to store the preceding places
 * @return The effective predicate as a boolean cover
 */
boolean::cover graph::effective(petri::iterator i, vector<petri::iterator> *prev) const {
	if (i.type == petri::place::type) {
		return places[i.index].effective;
	}
	
	boolean::cover pred = 1;
	for (auto arc = arcs[1-i.type].begin(); arc != arcs[1-i.type].end(); arc++) {
		if (arc->to.index == i.index) {
			pred &= places[arc->from.index].predicate;
			if (prev != nullptr) {
				prev->push_back(arc->from);
			}
		}
	}
	pred &= ~transitions[i.index].guard & ~transitions[i.index].local_action;
	pred.hide(ghost_nets);
	return pred;
}

/**
 * @brief Calculate the state encoding for a set of nodes
 * 
 * The implicant of a node is the state of the system when a token is at that node.
 * For a place, it is the predicate. For a transition, it is the combined predicate
 * of the input places which pass the guard. The implicant of multiple nodes is the
 * conjunction of the implicants of the nodes.
 * 
 * @param pos Vector of iterators pointing to places
 * @return The implicant as a boolean cover
 */
boolean::cover graph::implicant(petri::region pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
			result &= transitions[i->index].guard;
		}
	}
	result.hide(ghost_nets);
	
	return result;
}

/**
 * @brief Calculate the effective implicant for a set of nodes
 * 
 * The effective implicant excludes state encodings that pass the guards
 * of any outgoing transitions or in which the transition is vacuous.
 * 
 * @param pos Vector of iterators pointing to places
 * @return The effective implicant as a boolean cover
 */
boolean::cover graph::effective_implicant(petri::region pos) const {
	boolean::cover result = 1;
	for (auto i = pos.begin(); i != pos.end(); i++) {
		if (i->type == petri::place::type) {
			result &= places[i->index].predicate;
			for (auto arc = arcs[i->type].begin(); arc != arcs[i->type].end(); arc++) {
				if (arc->from.index == i->index) {
					result &= ~transitions[arc->to.index].guard;
				}
			}
		} else {
			for (auto arc = arcs[1-i->type].begin(); arc != arcs[1-i->type].end(); arc++) {
				if (arc->to.index == i->index) {
					result &= places[arc->from.index].predicate;
				}
			}
			result &= transitions[i->index].guard & ~transitions[i->index].local_action;
		}
	}
	result.hide(ghost_nets);
	return result;
}

/**
 * @brief Filter out states from the current encoding that are not affected by the transition.
 * 
 * @param i Iterator pointing to a transition
 * @param encoding The current state encoding to filter
 * @param action The action to verify
 * @return The filtered encoding as a boolean cover
 */
boolean::cover graph::filter_vacuous(petri::iterator i, boolean::cover encoding, boolean::cube action) const
{
	// I need to select encodings that are either affected by a transition that
	// wouldn't otherwise happen, or that enable an interfering transition.
	// action is not vacuous and action is not covered by equal actions in the output transition
	// or action conflicts with opposing actions in the output transition
	if (i.type == petri::transition::type) {
		// No guards to contend with for transitions
		boolean::cube other = transitions[i.index].local_action.subcube();
		if (interfere(action, other).is_null()) {
			// First, check if action conflicts with opposing actions in the output transition
			// if so, then return all encodings
			return encoding;
		}

		if (other.is_subset_of(action)) {
			// Then, check if action is covered by equal actions in the output transition
			// if so, then return no encodings
			return boolean::cover(0);
		}
		
		// Then, filter out all encodings for which action is vacuous by anding against not_action
		return encoding & ~action;
	}

	boolean::cover result(0);
	// This is a place, so we need to check all of the output transitions
	boolean::cover wait = encoding & ~action;
	for (auto arc = arcs[i.type].begin(); arc != arcs[i.type].end(); arc++) {
		if (arc->from.index == i.index) {
			boolean::cover predicate = encoding & transitions[arc->to.index].guard;
			result |= filter_vacuous(arc->to, predicate, action);
			wait &= ~transitions[arc->to.index].guard;
		}
	}
	result |= wait;
	return result;
}

/**
 * @brief Calculate the minimal guard needed to exclude other conditional branches
 * 
 * This computes the most minimal guard needed to exclude other conditional branches
 * from the execution of the current transition
 * 
 * @param index Index of the transition
 * @return The minimal guard as a boolean cover
 */
boolean::cover graph::exclusion(int index) const {
	boolean::cover result;
	vector<int> p = prev(transition::type, index);
	
	for (int i = 0; i < (int)p.size(); i++) {
		vector<int> n = next(place::type, p[i]);
		if (n.size() > 1) {
			for (int j = 0; j < (int)n.size(); j++) {
				if (n[j] != index) {
					result |= transitions[n[j]].guard;
				}
			}
		}
	}
	return result;
}

boolean::cover graph::arbitration(int index) const {
	boolean::cover result;
	vector<int> p = prev(transition::type, index);
	
	for (int i = 0; i < (int)p.size(); i++) {
		if (places[p[i]].arbiter or places[p[i]].synchronizer) {
			vector<int> n = next(place::type, p[i]);
			if (n.size() > 1) {
				for (int j = 0; j < (int)n.size(); j++) {
					if (n[j] != index) {
						result |= transitions[n[j]].local_action;
					}
				}
			}
		}
	}
	return ~result;
}


/**
 * @brief Check if two transitions share a common arbiter
 * 
 * Determine if the two transitions share an arbitered input place.
 * 
 * @param a Iterator pointing to the first transition
 * @param b Iterator pointing to the second transition
 * @return True if they share a common arbiter, false otherwise
 */
bool graph::common_arbiter(petri::iterator a, petri::iterator b) const
{
	vector<petri::iterator> left, right;
	if (a.type == petri::place::type) {
		/*if (places[a.index].arbiter) {
			left.push_back(a);
		} else {*/
			return false;
		//}
	}
 
	if (b.type == petri::place::type) {
		/*if (places[b.index].arbiter) {
			right.push_back(b);
		} else {*/
			return false;
		//}
	}

	if (a.type == petri::transition::type) {
		vector<petri::iterator> p = prev(a);
		for (int i = 0; i < (int)p.size(); i++) {
			if (places[p[i].index].arbiter or places[p[i].index].synchronizer) {
				left.push_back(p[i]);
			}
		}
	}
	if (left.size() == 0) {
		return false;
	}

	if (b.type == petri::transition::type) {
		vector<petri::iterator> p = prev(b);
		for (int i = 0; i < (int)p.size(); i++) {
			if (places[p[i].index].arbiter or places[p[i].index].synchronizer) {
				right.push_back(p[i]);
			}
		}
	}
	if (right.size() == 0) {
		return false;
	}

	return vector_intersection_size(left, right) > 0;
}

/**
 * @brief Update masks for all places in the graph
 * 
 * Masks limit the predicate to only variables observable in the process.
 * This function ensures all masks are properly set for all places.
 */
void graph::update_masks() {
	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++) {
		places[i.index].mask = 1;
	}

	for (petri::iterator i = begin(petri::place::type); i < end(petri::place::type); i++)
	{
		for (petri::iterator j = begin(petri::transition::type); j < end(petri::transition::type); j++) {
			if (is_reachable(j, i)) {
				places[i.index].mask = places[i.index].mask.combine_mask(transitions[j.index].assume.mask()).combine_mask(transitions[j.index].guard.mask()).combine_mask(transitions[j.index].local_action.mask()).combine_mask(transitions[j.index].ghost.mask());
			}
		}
		places[i.index].mask = places[i.index].mask.flip();
	}
}

/**
 * @brief Post-process the graph to optimize and validate its structure
 * 
 * Performs various optimizations and validations on the graph:
 * - Validates proper nesting of control structures (if requested)
 * - Computes split groups for parallel and choice compositions
 * - Merges redundant nodes
 * - Removes infeasible transitions
 * - Updates masks and state information
 * 
 * @param proper_nesting Whether to enforce proper nesting of control structures
 * @param aggressive Whether to use aggressive optimization strategies
 */
void graph::post_process(bool proper_nesting, bool aggressive, bool annotate, bool debug)
{
	for (int i = 0; i < (int)transitions.size(); i++) {
		transitions[i].remote_action = transitions[i].local_action.remote(remote_groups());
	}

	if (debug) cout << "entering hse::graph::post_process()" << endl;

	// Handle Reset Behavior
	bool change = true;
	while (change)
	{
		super::reduce(proper_nesting, aggressive, debug);
		change = false;

		if (debug) cout << "\tpropagating reset " << to_string(reset) << endl;

		for (int i = 0; i < (int)reset.size(); i++)
		{
			simulator::super sim(this, reset[i]);
			sim.enabled();

			change = false;
			for (int j = 0; j < (int)sim.ready.size() && !change; j++)
			{
				bool firable = transitions[sim.ready[j].index].local_action.cubes.size() <= 1;
				for (int k = 0; k < (int)sim.ready[j].tokens.size() && firable; k++)
				{
					for (int l = 0; l < (int)arcs[petri::transition::type].size() && firable; l++)
						if (arcs[petri::transition::type][l].to.index == sim.tokens[sim.ready[j].tokens[k]].index)
							firable = false;
					for (int l = 0; l < (int)arcs[petri::place::type].size() && firable; l++)
						if (arcs[petri::place::type][l].from.index == sim.tokens[sim.ready[j].tokens[k]].index && arcs[petri::place::type][l].to.index != sim.ready[j].index)
							firable = false;
				}

				if (firable) {
					if (debug) cout << "\tfiring " << j << endl;

					petri::enabled_transition t = sim.fire(j);
					reset[i].tokens = sim.tokens;
					for (int k = (int)transitions[t.index].local_action.size()-1; k >= 0; k--) {
						for (int l = (int)transitions[t.index].guard.size()-1; l >= 0; l--) {
							int idx = i;
							if (k > 0 || l > 0) {
								idx = reset.size();
								reset.push_back(reset[i]);
							}

							reset[idx].encodings &= transitions[t.index].guard.cubes[l];
							reset[idx].encodings = local_assign(reset[idx].encodings, transitions[t.index].local_action.cubes[k], true);
							reset[idx].encodings = remote_assign(reset[idx].encodings, transitions[t.index].remote_action.cubes[k], true);
						}
					}

					change = true;
				}
			}
		}
	}

	change = true;
	while (change) {
		super::reduce(proper_nesting, aggressive, debug);

		if (debug) cout << "\treducing structure" << endl;

		// Remove skips
		change = false;
		for (petri::iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++) {
			if (transitions[i.index].local_action.is_tautology()) {
				vector<petri::iterator> n = next(i); // places
				if (n.size() > 1) {
					if (debug) cout << "\tremoving skip " << i << endl;
					vector<petri::iterator> p = prev(i); // places
					vector<vector<petri::iterator> > pp;
					for (int j = 0; j < (int)p.size(); j++) {
						pp.push_back(prev(p[j]));
					}

					for (int k = (int)arcs[petri::transition::type].size()-1; k >= 0; k--) {
						if (arcs[petri::transition::type][k].from == i) {
							erase_arc(petri::iterator(petri::transition::type, k));
						}
					}

					vector<petri::iterator> copies;
					copies.push_back(i);
					for (int k = 0; k < (int)n.size(); k++) {
						if (k > 0) {
							copies.push_back(copy(i));
							for (int l = 0; l < (int)p.size(); l++) {
								petri::iterator x = copy(p[l]);
								connect(pp[l], x);
								connect(x, copies.back());
							}
						}
						connect(copies.back(), n[k]);
					}
					change = true;
				}
			}
		}
		if (change)
			continue;

		// If there is a guard at the end of a conditional branch, then we unzip
		// the conditional merge by one transition (make copies of the next
		// transition on each branch and move the merge down the sequence). This
		// allows us to merge that guard at the end of the conditional branch into
		// the transition.
		for (petri::iterator i(place::type, 0); i < (int)places.size() && !change; i++) {
			vector<petri::iterator> p = prev(i);
			vector<petri::iterator> active, passive;
			for (int k = 0; k < (int)p.size(); k++) {
				if (transitions[p[k].index].local_action.is_tautology()) {
					passive.push_back(p[k]);
				} else {
					active.push_back(p[k]);
				}
			}

			if (passive.size() > 1 || (passive.size() == 1 && active.size() > 0)) {
				if (debug) cout << "unzipping choice " << i << endl;
				vector<petri::iterator> copies;
				if ((int)active.size() == 0) {
					copies.push_back(i);
				}

				vector<petri::iterator> n = next(i);
				vector<vector<petri::iterator> > nn;
				for (int l = 0; l < (int)n.size(); l++) {
					nn.push_back(next(n[l]));
				}
				vector<vector<petri::iterator> > np;
				for (int l = 0; l < (int)n.size(); l++) {
					np.push_back(prev(n[l]));
					np.back().erase(std::remove(np.back().begin(), np.back().end(), i), np.back().end()); 
				}

				for (int k = 0; k < (int)passive.size(); k++) {
					// Disconnect this transition
					for (int l = (int)arcs[petri::transition::type].size()-1; l >= 0; l--) {
						if (arcs[petri::transition::type][l].from == passive[k]) {
							erase_arc(petri::iterator(petri::transition::type, l));
						}
					}

					if (k >= (int)copies.size()) {
						copies.push_back(create(places[i.index]));
						for (int l = 0; l < (int)n.size(); l++) {
							petri::iterator x = copy(n[l]);
							connect(copies.back(), x);
							connect(x, nn[l]);
							connect(np[l], x);
						}
					}

					connect(passive[k], copies[k]);
				}

				change = true;
			}
		}
		if (change)
			continue;

		for (petri::iterator i(transition::type, 0); i < (int)transitions.size() && !change; i++) {
			if (transitions[i.index].local_action == 1) {
				if (debug) cout << "forwarding passive transition " << i << endl;
				vector<petri::iterator> nn = next(next(i)); // transitions
				for (int l = 0; l < (int)nn.size(); l++) {
					transitions[nn[l].index] = transition::merge(petri::sequence, transitions[i.index], transitions[nn[l].index]);
				}

				pinch(i);
				change = true;
			}
		}
		if (change)
			continue;
	}

	if (debug) cout << "exiting hse::graph::post_process()" << endl;

	if (annotate) {
		annotate_conditional_branches();
	}

	// Determine the actual starting location of the tokens given the state information
	update_masks();
}

/**
 * @brief Verify that all variables are both assigned and read
 */
void graph::check_variables()
{
	vector<int> vars;
	for (int i = 0; i < (int)nets.size(); i++) {
		vector<int> written;
		vector<int> read;

		for (int j = 0; j < (int)transitions.size(); j++) {
			vars = transitions[j].remote_action.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				written.push_back(j);
			}

			vars = transitions[j].guard.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				read.push_back(j);
			}

			vars = transitions[j].assume.vars();
			if (find(vars.begin(), vars.end(), i) != vars.end()) {
				read.push_back(j);
			}
		}

		if (written.size() == 0 && read.size() > 0)
			warning("", nets[i].to_string() + " never assigned", __FILE__, __LINE__);
		else if (written.size() == 0 && read.size() == 0 and not nets[i].is_ghost)
			warning("", "unused net " + nets[i].to_string(), __FILE__, __LINE__);
	}
}

/**
 * @brief Identify nodes that need to be checked for state conflicts.
 *  
 * A node is relevant if it is reachable from the current set of places and
 * is not in parallel with the transition we're checking.
 * 
 * @param curr Vector of iterators pointing to places
 * @return Vector of iterators pointing to all relevant nodes
 */
vector<petri::iterator> graph::relevant_nodes(vector<petri::iterator> curr)
{
	vector<petri::iterator> result;
	// We're going to check this transition against all of the places in the system
	for (petri::iterator j = begin(petri::place::type); j != end(petri::place::type); j++) {
		// The place is in the same process as the implicant set of states,
		// and its not in parallel with the transition we're checking,
		// and they aren't forced to be mutually exclusive by an arbiter
	
		bool relevant = false;
		for (int i = 0; i < (int)curr.size() and not relevant; i++) {
			relevant = (is_reachable(curr[i], j) or is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			relevant = (j != curr[i] and not common_arbiter(curr[i], j) and not is(parallel, curr[i], j));
		}

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			for (int k = 0; k < (int)arcs[transition::type].size() and curr[i].type == transition::type and relevant; k++) {
				relevant = (arcs[transition::type][k].from.index != curr[i].index or arcs[transition::type][k].to.index != j.index);
			}
		}

		if (relevant) {
			result.push_back(j);
		}
	}

	// check the states inside each transition
	for (petri::iterator j = begin(petri::transition::type); j != end(petri::transition::type); j++) {
		// The place is in the same process as the implicant set of states,
		// and its not in parallel with the transition we're checking,
		// and they aren't forced to be mutually exclusive by an arbiter
		bool relevant = false;
		for (int i = 0; i < (int)curr.size() and not relevant; i++) {
			relevant = (is_reachable(curr[i], j) or is_reachable(j, curr[i]));
		}

		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			relevant = (j != curr[i] and not common_arbiter(curr[i], j) and not is(parallel, curr[i], j));
		}
		
		for (int i = 0; i < (int)curr.size() and relevant; i++) {
			for (int k = 0; k < (int)arcs[place::type].size() and curr[i].type == place::type and relevant; k++) {
				relevant = (arcs[place::type][k].from.index != curr[i].index or arcs[place::type][k].to.index != j.index);
			}
		}

		if (relevant) {
			result.push_back(j);
		}
	}

	return result;
}

/**
 * @brief Annotate conditional branches in the graph
 * 
 * Adds ghost nets to conditional branches to help with state propagation
 * and synthesis. Ghost nets are used to separate the states of different
 * branches of a conditional within the state space so that we can weave
 * them together in different ways using state variables.
 */
void graph::annotate_conditional_branches() {
	if (not ghost_nets.empty()) {
		for (petri::iterator i = begin(petri::transition::type); i != end(petri::transition::type); i++) {
			transitions[i.index].ghost = 1;
		}
		ghost_nets.clear();
	}

	for (petri::iterator i = begin(petri::place::type); i != end(petri::place::type); i++) {
		places[i.index].ghost_nets.clear();

		vector<petri::iterator> n = next(i);
		if ((int)n.size() > 1) {
			int count = log2i((int)n.size());
			count += ((int)n.size() > (1<<count));
			for (int j = 0; j < count; j++) {
				string name = ghost_prefix + to_string(i.index) + "_" + to_string(places[i.index].ghost_nets.size());
				int uid = netIndex(name);
				if (uid < 0) {
					uid = create(net(name, 0, true));
				}
				places[i.index].ghost_nets.push_back(uid);
			}

			boolean::cover total;
			for (int j = 0; j < (int)n.size(); j++) {
				if (j == (int)n.size()-1) {
					transitions[n[j].index].ghost &= ~total;
				} else {
					boolean::cover ghost = boolean::encode_binary(j, places[i.index].ghost_nets);
					transitions[n[j].index].ghost &= ghost;
					total |= ghost;
				}
			}
		}
	}
}

}
