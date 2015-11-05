/*
 * encoder.cpp
 *
 *  Created on: May 7, 2015
 *      Author: nbingham
 */

#include "encoder.h"
#include "graph.h"

namespace hse
{

conflict::conflict()
{
	sense = -1;
}

conflict::conflict(term_index index, int sense, vector<int> implicant, vector<int> region)
{
	this->sense = sense;
	this->index = index;
	this->implicant = implicant;
	this->region = region;
}

conflict::~conflict()
{

}

suspect::suspect()
{
	sense = -1;
}

suspect::suspect(int sense, vector<int> first, vector<int> second)
{
	this->sense = sense;
	this->first = first;
	this->second = second;
}

suspect::~suspect()
{

}

encoder::encoder()
{
	base = NULL;
}

encoder::encoder(graph *base)
{
	this->base = base;
	check();
}

encoder::~encoder()
{

}

void encoder::check(bool senseless)
{
	if (base == NULL)
		return;

	vector<vector<int> > conflict_regions;
	vector<vector<int> > suspect_regions;
	conflicts.clear();
	suspects.clear();

	vector<vector<vector<int> > > pdeps(base->places.size(), vector<vector<int> >());

	for (int i = 0; i < (int)base->places.size(); i++)
		pdeps[i] = base->get_dependency_tree(hse::iterator(hse::place::type, i));


	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (int i = 0; i < (int)base->transitions.size(); i++)
	{
		vector<int> p = base->prev(petri::transition::type, i);
		vector<vector<int> > idep;

		boolean::cover si = 1;

		if (base->transitions[i].behavior == transition::active || p.size() > 1)
		{
			idep = base->get_dependency_tree(hse::iterator(hse::transition::type, i));
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
				si &= base->places[p[j]].effective;
		}

		// The transition actively affects the state of the system
		if (base->transitions[i].behavior == transition::active)
		{
			for (int j = 0; j < (int)base->transitions[i].local_action.cubes.size(); j++)
			{
				boolean::cube action = base->transitions[i].local_action.cubes[j];
				boolean::cover inverse_action = ~action;

				for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
				{
					// If we aren't specifically checking senses or this transition affects the senses we are checking
					if (senseless || !base->transitions[i].local_action.cubes[j].mask(1-s).is_tautology())
					{
						// we cannot use the variables affected by the transitions in their rules because that would
						// make the rules self-invalidating, so we have to mask them out.
						boolean::cover implicant = si.mask(action.mask()).mask(s);

						// We're going to check this transition against all of the places in the system
						for (int k = 0; k < (int)base->places.size(); k++)
						{
							//vector<int> assigns = base->associated_assigns(vector<int>(1, k));

							// The place is in the same process as the implicant set of states, its not in parallel with the transition we're checking,
							// and it isn't part of any implicant state
							if ((base->is_reachable(petri::iterator(petri::transition::type, i), petri::iterator(petri::place::type, k)) ||
								 base->is_reachable(petri::iterator(petri::place::type, k), petri::iterator(petri::transition::type, i))) &&
								 find(p.begin(), p.end(), k) == p.end() && !base->is_parallel(petri::iterator(petri::place::type, k), petri::iterator(petri::transition::type, i)))
							{
								// Check to make sure they aren't forced to be
								// mutually exclusive by an arbiter
								if (!base->common_arbiter(idep, pdeps[k]))
								{
									// Get only the state encodings for the place for which the transition is invacuous and
									// there is no other vacuous transition that would take a token off the place.
									boolean::cover check = (base->places[k].effective & ~action);

									// The implicant states for any actions that would have the same effect as the transition we're checking
									// can be removed from our check because "it could have fired there anyways"
									vector<int> check_transitions = base->next(petri::place::type, k);
									for (int l = 0; l < (int)check_transitions.size(); l++)
									{
										bool found = false;
										for (int m = 0; m < (int)base->transitions[check_transitions[l]].local_action.cubes.size() && !found; m++)
											// Check if this transition has the same effect
											if (are_mutex(base->transitions[check_transitions[l]].local_action.cubes[m], inverse_action))
											{
												found = true;
												vector<int> check_places = base->prev(petri::transition::type, check_transitions[l]);
												boolean::cover invalid_check = 1;
												// If it does, get its implicant states and remove them from the check
												for (int n = 0; n < (int)check_places.size(); n++)
													invalid_check &= base->places[check_places[n]].effective;

												check &= ~invalid_check;
											}
									}

									// Check if they share a common state encoding given the above checks
									if (!are_mutex(implicant, check))
									{
										// cluster the conflicting places into regions. We want to be able to
										// eliminate entire regions of these conflicts with a single state variable.

										// get the input and output transitions of the place we are comparing against
										vector<int> r = base->neighbors(hse::place::type, k, true);

										vector<int> merge;
										for (int l = 0; l < (int)conflicts.size(); l++)
											if (conflicts[l].index.index == i && conflicts[l].index.term == j && conflicts[l].sense == s && vector_intersection_size(r, conflict_regions[l]) > 0)
												merge.push_back(l);

										if (merge.size() > 0)
										{
											for (int l = 1; l < (int)merge.size(); l++)
											{
												conflicts[merge[0]].region.insert(conflicts[merge[0]].region.end(), conflicts[merge[l]].region.begin(), conflicts[merge[l]].region.end());
												conflict_regions[merge[0]].insert(conflict_regions[merge[0]].end(), conflict_regions[merge[l]].begin(), conflict_regions[merge[l]].end());
											}

											for (int l = (int)merge.size()-1; l > 0; l--)
											{
												conflicts.erase(conflicts.begin() + merge[l]);
												conflict_regions.erase(conflict_regions.begin() + merge[l]);
											}

											conflicts[merge[0]].region.push_back(k);
											conflict_regions[merge[0]].insert(conflict_regions[merge[0]].end(), r.begin(), r.end());
											sort(conflict_regions[merge[0]].begin(), conflict_regions[merge[0]].end());
										}
										else
										{
											conflicts.push_back(conflict(term_index(i, j), s, p, vector<int>(1, k)));
											conflict_regions.push_back(r);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// Get the list of places that are suspect against parallel merges
		// A place is suspect if making it an implicant for an active transition
		// could make it a conflict.
		if (p.size() > 1)
		{
			for (int j = 0; j < (int)base->places.size(); j++)
				if ((base->is_reachable(petri::iterator(petri::transition::type, i), petri::iterator(petri::place::type, j)) ||
					 base->is_reachable(petri::iterator(petri::place::type, j), petri::iterator(petri::transition::type, i))) &&
					find(p.begin(), p.end(), j) == p.end() &&
					!base->is_parallel(petri::iterator(petri::transition::type, i), petri::iterator(petri::place::type, j)) &&
					!base->common_arbiter(idep, pdeps[j]))
					for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
						if (!are_mutex(si.mask(s), base->places[j].effective))
						{
							// cluster the suspected places into regions. We want to be able to
							// avoid entire regions when we try to insert state variables

							// get the input and output transitions of the place we are comparing against
							vector<int> r = base->neighbors(hse::place::type, j, true);

							vector<int> merge;
							for (int l = 0; l < (int)suspects.size(); l++)
								if (suspects[l].first == p && suspects[l].sense == s && vector_intersection_size(r, suspect_regions[l]) > 0)
									merge.push_back(l);

							if (merge.size() > 0)
							{
								for (int l = 1; l < (int)merge.size(); l++)
								{
									suspects[merge[0]].second.insert(suspects[merge[0]].second.end(), suspects[merge[l]].second.begin(), suspects[merge[l]].second.end());
									suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), suspect_regions[merge[l]].begin(), suspect_regions[merge[l]].end());
								}

								for (int l = (int)merge.size()-1; l > 0; l--)
								{
									suspects.erase(suspects.begin() + merge[l]);
									suspect_regions.erase(suspect_regions.begin() + merge[l]);
								}

								suspects[merge[0]].second.push_back(j);
								suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), r.begin(), r.end());
								sort(suspect_regions[merge[0]].begin(), suspect_regions[merge[0]].end());
							}
							else
							{
								suspects.push_back(suspect(s, p, vector<int>(1, j)));
								suspect_regions.push_back(r);
							}
						}
		}
	}

	// get the list of places that are suspect against other places
	for (int i = 0; i < (int)base->places.size(); i++)
		for (int j = 0; j < (int)base->places.size(); j++)
			if (j != i && (base->is_reachable(petri::iterator(petri::place::type, i), petri::iterator(petri::place::type, j)) ||
					       base->is_reachable(petri::iterator(petri::place::type, j), petri::iterator(petri::place::type, i))) &&
				!base->is_parallel(petri::iterator(petri::place::type, i), petri::iterator(petri::place::type, j)) &&
				!base->common_arbiter(pdeps[i], pdeps[j]))
				for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
					if (!are_mutex(base->places[i].effective.mask(s), base->places[j].effective))
					{
						// cluster the conflicting places into regions. We want to be able to
						// eliminate entire regions of these conflicts with a single state variable.

						// get the input and output transitions of the place we are comparing against
						vector<int> r = base->neighbors(hse::place::type, j, true);

						vector<int> merge;
						for (int l = 0; l < (int)suspects.size(); l++)
							if (suspects[l].first == vector<int>(1, i) && suspects[l].sense == s && vector_intersection_size(r, suspect_regions[l]) > 0)
								merge.push_back(l);

						if (merge.size() > 0)
						{
							for (int l = 1; l < (int)merge.size(); l++)
							{
								suspects[merge[0]].second.insert(suspects[merge[0]].second.end(), suspects[merge[l]].second.begin(), suspects[merge[l]].second.end());
								suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), suspect_regions[merge[l]].begin(), suspect_regions[merge[l]].end());
							}

							for (int l = (int)merge.size()-1; l > 0; l--)
							{
								suspects.erase(suspects.begin() + merge[l]);
								suspect_regions.erase(suspect_regions.begin() + merge[l]);
							}

							suspects[merge[0]].second.push_back(j);
							suspect_regions[merge[0]].insert(suspect_regions[merge[0]].end(), r.begin(), r.end());
							sort(suspect_regions[merge[0]].begin(), suspect_regions[merge[0]].end());
						}
						else
						{
							suspects.push_back(suspect(s, vector<int>(1, i), vector<int>(1, j)));
							suspect_regions.push_back(r);
						}
					}
}


}
