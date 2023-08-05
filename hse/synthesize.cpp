#include "synthesize.h"

namespace hse
{

void guard_weakening(graph &proc, prs::production_rule_set *out, const ucs::variable_set &v, bool senseless, bool report_progress)
{
	vector<vector<vector<int> > > pdeps(proc.places.size(), vector<vector<int> >());

	for (int i = 0; i < (int)proc.places.size(); i++)
		pdeps[i] = proc.get_dependency_tree(hse::iterator(hse::place::type, i));


	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (int i = 0; i < (int)proc.transitions.size(); i++)
	{
		if (report_progress)
			progress("", "T" + ::to_string(i) + "/" + ::to_string(proc.transitions.size()), __FILE__, __LINE__);

		vector<int> p = proc.prev(petri::transition::type, i);
		vector<vector<int> > idep;

		boolean::cover si = 1;

		if (!proc.transitions[i].local_action.is_tautology() || p.size() > 1)
		{
			idep = proc.get_dependency_tree(hse::iterator(hse::transition::type, i));
			/*cout << "T" << i << ": {";
			for (int x = 0; x < (int)idep.size(); x++)
			{
				if (x != 0)
					cout << " ";
				cout << "(";
				for (int y = 0; y < (int)idep[x].size(); y++)
				{
					if (i != 0)
						cout << " ";
					cout << idep[x][y];
				}
				cout << ")";
			}
			cout << "}" << endl;*/
			// The state encodings for the implicant set of states of the transition.
			// these are the state encodings for which there is no vacuous transition that would
			// take a token off of any of the places.
			for (int j = 0; j < (int)p.size(); j++)
				si &= proc.places[p[j]].predicate;
		}

		// The transition actively affects the state of the system
		if (!proc.transitions[i].local_action.is_tautology())
		{
			for (int j = 0; j < (int)proc.transitions[i].local_action.cubes.size(); j++)
			{
				boolean::cover guard = proc.transitions[i].guard;
				boolean::cube action = proc.transitions[i].local_action.cubes[j];
				boolean::cover inverse_action = ~action;

				for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
				{
					// If we aren't specifically checking senses or this transition affects the senses we are checking
					if (senseless || !proc.transitions[i].local_action.cubes[j].mask(1-s).is_tautology())
					{
						// we cannot use the variables affected by the transitions in their rules because that would
						// make the rules self-invalidating, so we have to mask them out.
						boolean::cover implicant = (si & guard).mask(action.mask()).mask(s);
						boolean::cover exclusion;
						// We're going to check this transition against all of the places in the system
						for (int k = 0; k < (int)proc.places.size(); k++)
						{
							//vector<int> assigns = proc.associated_assigns(vector<int>(1, k));

							// The place is in the same process as the implicant set of states, its not in parallel with the transition we're checking,
							// and it isn't part of any implicant state
							if ((proc.is_reachable(petri::iterator(petri::transition::type, i), petri::iterator(petri::place::type, k)) ||
								 proc.is_reachable(petri::iterator(petri::place::type, k), petri::iterator(petri::transition::type, i))) &&
								 find(p.begin(), p.end(), k) == p.end() && !proc.is_parallel(petri::iterator(petri::place::type, k), petri::iterator(petri::transition::type, i)))
							{
								// Check to make sure they aren't forced to be
								// mutually exclusive by an arbiter
								if (!proc.common_arbiter(idep, pdeps[k]))
								{
									// Get only the state encodings for the place for which the transition is invacuous and
									// there is no other vacuous transition that would take a token off the place.
									exclusion |= proc.places[k].effective & ~action;

									// The implicant states for any actions that would have the same effect as the transition we're checking
									// can be removed from our check because "it could have fired there anyways"
									vector<int> check_transitions = proc.next(petri::place::type, k);
									for (int l = 0; l < (int)check_transitions.size(); l++) {
										for (int m = 0; m < (int)proc.transitions[check_transitions[l]].local_action.cubes.size(); m++) {
											// Check if this transition has the same effect
											if (!are_mutex(proc.transitions[check_transitions[l]].local_action.cubes[m], inverse_action)) {
												vector<int> check_places = proc.prev(petri::transition::type, check_transitions[l]);
												boolean::cover check = 1;
												// If it does, get its implicant states and remove them from the check
												for (int n = 0; n < (int)check_places.size(); n++)
													check &= proc.places[check_places[n]].predicate;
												check &= proc.transitions[check_transitions[l]].guard;

												cout << "check " << export_expression(check, v).to_string() << " " << export_expression(inverse_action, v).to_string() << endl;
												exclusion |= check;
											}
										}
									}
								}
							}
						}

						cout << "exclusion " << export_expression(exclusion, v).to_string() << endl;
					 	cout << "rule "	<< export_expression(implicant, v).to_string() << " -> " << export_composition(action, v).to_string() << endl;
						boolean::espresso(implicant, ~exclusion & ~implicant, exclusion);
					 	cout << "rule "	<< export_expression(implicant, v).to_string() << " -> " << export_composition(action, v).to_string() << endl;
						out->rules.push_back(prs::production_rule());
						out->rules.back().guard = implicant;
						out->rules.back().local_action = action;
					}
				}
			}
		}
	}
}

void guard_strengthening(graph proc, prs::production_rule_set *out)
{
}

}
