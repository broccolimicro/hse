/*
 * simulator.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: nbingham
 */

#include "simulator.h"
#include "common/text.h"
#include "common/message.h"

namespace hse
{

simulator::simulator()
{
	base = NULL;
}

simulator::simulator(graph *base)
{
	this->base = base;
	for (int i = 0; i < (int)base->source.size(); i++)
		local.tokens.push_back(local_token(base->source[i].index, base->reset));
}

simulator::~simulator()
{

}

/* I want to be able to support global-local hybrid splits and merges. i.e. a branch of a parallel split
 * can be global while another is local. In previous versions, I only supported a very basic implementation
 * of this where entire processes were designated environment and were allowed to have global tokens on them.
 * But this only takes advantage of half of the possible optimizations. So right now there are a few things
 * to figure out.
 *
 * First, during a parallel split, how do we determine which branch gets a global token and which gets a local
 * token? The niave approach would be to say that every branch gets an execution with a local token where all
 * of its environment has a global token. However due to the reasons listed below, this would be a difficult
 * game. The next approach would be to say that as long as it only interacts with a local branch, its fine,
 * but then if you need to elaborate the branch with the global token, you've just created a bunch of extra work
 * for yourself because by definition, the branch that originally had the local token will interact with much more
 * than that one other branch.
 *
 * Second, during a parallel merge, how do we determine the state of the tokens? I'm guessing that for a merge
 * of all-global tokens, just use the global state of the system. For a merge of all-local tokens, calculate
 * the state by ANDing their individual state information together into one. For a hybrid merge, calculate
 * the state from just the local tokens. The all-local merge and all-global merge have been proven to work when
 * implemented that way before, so the only thing that could be an issue is the hybrid merge. The hybrid merge
 * results in local tokens. Ultimately collapsing the global tokens that enable the transition. We would have to
 * iterate backward from the global tokens that enable that transition and remove any transitions from the body
 * that are a result of those tokens. All of this assumes that it doesn't make sense for global tokens to maintain
 * state information. However, if they do maintain state information, then this question is moot. The new question
 * becomes how do we maintain the state information of a global token? The answer then becomes the same way that we
 * would a local token. The only difference is that the tales use the global state information while the heads use
 * local state information. This would make everything significantly easier. This also means that the local token
 * struct and the global token struct are the same thing and we are left with just a flat list of tokens. So how do
 * we tell the difference between a local token and a global head token?
 *
 * Third, how do we handle cases where a hybrid merge is not yet enabled, but another global transition takes the global
 * tokens away from the hybrid merge? Every time there is a conditional split in a global token, we would have to duplicate
 * the execution for every branch, including the branches that are not enabled. But then we would get deadlock in some cases.
 * The issue stems from the fact that a global token can make the difference in the outcome of the conditional. But in that
 * case, there would be no issue because we could just split the execution and have no deadlock... So I can actually narrow
 * this particular issue to non-deterministic conditional branches. No, nvm... Since glboal tokens always go first, it doesn't
 * take into account the ordering of events, so if the local token was able to go first, we would have taken one branch but
 * since it wasn't we took a different branch. This particular situation can happen at any conditional, even at internal choice.
 * This makes it painfully difficult to deal with.
 *
 * Finally, should I even use the simulator to elaborate the state space. i.e. should I even be working on this?
 * Why shouldn't I use synchronization regions? Regions by themselves actually don't work. They suffer from the same problem
 * that guard strengthening does where you would need to know which branch of a conditional was taken for a particular cube
 * in the predicate. However, It could help to alleviate the computational complexity of the simulation-based elaborator
 * by grouping transitions and reducing their number.
 *
 * Another problem arises when you have two parallel branches each covered by global tokens that have synchronization
 * actions between them. Handling this kind of situation is not the easiest thing. Lets say that we have two parallel
 * branches with a full synchronization action between them. Each branch has three sections of transitions. Branch 1
 * has sections A, B, and C. Branch 2 has sections D, E, F. There is a half synchronization action between the last
 * transition in A and the first transition in E and between the last transition in E and the first transition in C like
 * so.
 *
 *  C     F
 *  - <-- -
 *  B     E
 *  - --> -
 *  A     D
 *
 * If we were to ignore this synchronization action, the resulting transitions that a local token on another parallel branch
 * would experience is:
 * AD | AE | AF | BD | BE | BF | CD | CE | CF
 * However, the actual environment should be:
 * AD | BD | BE | BF | CF
 *
 * So the question becomes, how do we determine this in the general case?
 *
 * What if, instead, we were to just turn the HSE into a petri net? That one operation would be np-complete, but once its done
 * further issues would be minimized. Elaborating the state space would be polynomial time because there wouldn't be any synchronization
 * actions and any choice in the system would be non-deterministic. State variable insertion would be much simpler, as would guard
 * strengthening/weakening.
 *
 */

/** enabled()
 *
 * Returns a vector of indices representing the transitions
 * that this marking enabled and the term of each transition
 * that's enabled.
 */
int simulator::enabled(bool sorted)
{
	if (!sorted)
		sort(local.tokens.begin(), local.tokens.end());

	vector<enabled_transition> result;
	vector<int> disabled;

	// Get the list of transitions have have a sufficient number of local at the input places
	for (int i = 0; i < (int)base->arcs[place::type].size(); i++)
	{
		// Check to see if we haven't already determined that this transition can't be enabled
		if (find(disabled.begin(), disabled.end(), base->arcs[place::type][i].to.index) == disabled.end())
		{
			// Find the index of this transition (if any) in the result pool
			int k;
			for (k = 0; k < (int)result.size() && result[k].index != base->arcs[place::type][i].to.index; k++);

			// Check to see if there is any token at the input place of this arc and make sure that
			// this token has not already been consumed by this particular transition
			// Also since we only need one token per arc, we can stop once we've found a token
			bool found = false;
			for (int j = 0; j < (int)local.tokens.size() && !found; j++)
				if (base->arcs[place::type][i].from.index == local.tokens[j].index && (k >= (int)result.size() || find(result[k].local_tokens.begin(), result[k].local_tokens.end(), j) == result[k].local_tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (k >= (int)result.size())
						result.push_back(enabled_transition(base->arcs[place::type][i].to.index));

					result[k].local_tokens.push_back(j);
				}

			/*for (int j = 0; j < (int)global.head.size() && !found; j++)
				if (base->arcs[place::type][i].from.index == global.head[j].index && (k >= (int)result.size() || find(result[k].global_tokens.begin(), result[k].global_tokens.end(), j) == result[k].global_tokens.end()))
				{
					// We are safe to add this to the list of possibly enabled transitions
					found = true;
					if (k >= (int)result.size())
						result.push_back(enabled_transition(base->arcs[place::type][i].to.index));

					result[k].global_tokens.push_back(j);
				}*/

			// If we didn't find a token at the input place, then we know that this transition can't
			// be enabled. So lets remove this from the list of possibly enabled transitions and
			// remember as much in the disabled list.
			if (!found)
			{
				disabled.push_back(base->arcs[place::type][i].to.index);
				if (k < (int)result.size())
					result.erase(result.begin() + k);
			}
		}
	}

	// Now we have a list of transitions that have enough tokens to consume. We need to figure out their state
	for (int i = 0; i < (int)result.size(); i++)
		for (int j = 0; j < (int)result[i].local_tokens.size(); j++)
			result[i].state &= local.tokens[result[i].local_tokens[j]].state;

	// Now we need to check all of the terms against the state
	for (int i = (int)result.size()-1; i >= 0; i--)
	{
		for (int j = base->transitions[result[i].index].action.size()-1; j > 0; j--)
			if (base->transitions[result[i].index].behavior == transition::active || (base->transitions[result[i].index].action[j] & result[i].state) != 0)
			{
				result.push_back(result[i]);
				result.back().term = j;
			}

		if (base->transitions[result[i].index].action.size() > 0 && (base->transitions[result[i].index].behavior == transition::active || (base->transitions[result[i].index].action[0] & result[i].state) != 0))
			result[i].term = 0;
		else
			result.erase(result.begin() + i);
	}

	sort(result.begin(), result.end());

	// Check for interfering transitions
	for (int i = 0; i < (int)local.ready.size(); i++)
		if (base->transitions[local.ready[i].index].behavior == transition::active)
			for (int j = i+1; j < (int)local.ready.size(); j++)
				if (base->transitions[local.ready[j].index].behavior == transition::active &&
					boolean::are_mutex(base->transitions[local.ready[i].index].action[local.ready[i].term], base->transitions[local.ready[j].index].action[local.ready[j].term]))
					error("", "interfering transitions T" + to_string(local.ready[i].index) + "." + to_string(local.ready[i].term) + " and T" + to_string(local.ready[j].index) + "." + to_string(local.ready[j].term), __FILE__, __LINE__);

	// Check for unstable transitions
	for (int i = 0, j = 0; i < (int)local.ready.size() && j < (int)result.size();)
	{
		if (local.ready[i] < result[j])
		{
			error("", "unstable transition T" + to_string(local.ready[i].index) + "." + to_string(local.ready[i].term), __FILE__, __LINE__);
			i++;
		}
		else if (result[j] < local.ready[i])
			j++;
		else if (local.ready[i] == result[j])
		{
			i++;
			j++;
		}
	}

	local.ready = result;
	return local.ready.size();
}

void simulator::fire(int index)
{
	enabled_transition t = local.ready[index];

	// Update the tokens
	vector<int> next = base->next(transition::type, t.index);
	sort(t.local_tokens.rbegin(), t.local_tokens.rend());
	for (int i = 0; i < (int)t.local_tokens.size(); i++)
		local.tokens.erase(local.tokens.begin() + t.local_tokens[i]);

	// Update the state
	if (base->transitions[t.index].behavior == transition::active)
	{
		t.state = boolean::local_transition(t.state, base->transitions[t.index].action[t.term]);
		for (int i = 0; i < (int)local.tokens.size(); i++)
			local.tokens[i].state = boolean::remote_transition(local.tokens[i].state, base->transitions[t.index].action[t.term]);
	}
	else if (base->transitions[t.index].behavior == transition::passive)
		t.state &= base->transitions[t.index].action[t.term];

	for (int i = 0; i < (int)next.size(); i++)
		local.tokens.push_back(local_token(next[i], t.state));

	// disable any transitions that were dependent on at least one of the same tokens
	// This is only necessary to check for unstable transitions in the enabled() function
	for (int i = (int)local.ready.size()-1; i >= 0; i--)
		if (vector_intersection_size(local.ready[i].local_tokens, t.local_tokens) > 0)
			local.ready.erase(local.ready.begin() + i);
}

int simulator::enabled_global(bool sorted)
{


}

int simulator::disabled_global(bool sorted)
{

}

void simulator::begin(int index)
{

}

void simulator::end(int index)
{

}

simulator::state simulator::get_state()
{
	simulator::state result;

	result.encoding = 1;
	for (int i = 0; i < (int)local.tokens.size(); i++)
	{
		result.tokens.push_back(local.tokens[i].index);
		result.encoding &= local.tokens[i].state;
	}

	sort(result.tokens.begin(), result.tokens.end());

	return result;
}

vector<int> simulator::get_choice(int i)
{
	vector<int> result;
	for (int j = 0; j < (int)local.ready.size(); j++)
		if (i != j && vector_intersection_size(local.ready[j].local_tokens, local.ready[i].local_tokens) > 0)
			result.push_back(j);
	return result;
}

bool operator<(simulator::state s1, simulator::state s2)
{
	return (s1.tokens < s2.tokens) || (s1.tokens == s2.tokens && s1.encoding < s2.encoding);
}

bool operator>(simulator::state s1, simulator::state s2)
{
	return (s1.tokens > s2.tokens) || (s1.tokens == s2.tokens && s1.encoding > s2.encoding);
}

bool operator<=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens < s2.tokens) || (s1.tokens == s2.tokens && s1.encoding <= s2.encoding);
}

bool operator>=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens > s2.tokens) || (s1.tokens == s2.tokens && s1.encoding >= s2.encoding);
}

bool operator==(simulator::state s1, simulator::state s2)
{
	return (s1.tokens == s2.tokens && s1.encoding == s2.encoding);
}

bool operator!=(simulator::state s1, simulator::state s2)
{
	return (s1.tokens != s2.tokens || s1.encoding != s2.encoding);
}

}
