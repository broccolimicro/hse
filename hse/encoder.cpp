/*
 * encoder.cpp
 *
 *  Created on: May 7, 2015
 *      Author: nbingham
 */

#include "encoder.h"
#include <common/standard.h>

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

	// The implicant set of states of a transition conflicts with a set of states represented by a single place if
	for (int i = 0; i < (int)base->transitions.size(); i++)
	{
		vector<int> p = base->prev(transition::type, i);
		boolean::cover si = 1;

		if (base->transitions[i].behavior == transition::active || p.size() > 1)
		{
			// The the state encodings for the implicant set of states of the transition.
			// these are the state encodings for which there is no vacuous transition that would
			// take a token off of any of the places.
			for (int j = 0; j < (int)p.size(); j++)
				si &= base->places[p[j]].effective;
		}

		// The transition actively affects the state of the system
		if (base->transitions[i].behavior == transition::active)
		{
			for (int j = 0; j < (int)base->transitions[i].action.cubes.size(); j++)
			{
				boolean::cube action = base->transitions[i].action.cubes[j];
				boolean::cover inv_action = ~action;

				for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
				{
					// we cannot use the variables affected by the transitions in their rules because that would
					// make the rules self-invalidating, so we have to mask them out.
					if (senseless || !base->transitions[i].action.cubes[j].mask(1-s).is_tautology())
					{
						boolean::cover implicant = si.mask(action.mask()).mask(s);

						for (int k = 0; k < (int)base->places.size(); k++)
						{
							// The place is in the same process as the implicant set of states and it isn't part of any implicant state
							if ((base->is_reachable(iterator(transition::type, i), iterator(place::type, k)) || base->is_reachable(iterator(place::type, k), iterator(transition::type, i))) &&
								find(p.begin(), p.end(), k) == p.end() && !base->is_parallel(iterator(place::type, k), iterator(transition::type, i)))
							{
								// Get only the state encodings for the place for which the transition is invacuous and
								// there is no other vacuous transition that would take a token off the place.
								boolean::cover check = (base->places[k].effective & ~action);

								vector<int> check_transitions = base->next(hse::place::type, k);
								for (int l = 0; l < (int)check_transitions.size(); l++)
								{
									bool found = false;
									for (int m = 0; m < (int)base->transitions[check_transitions[l]].action.cubes.size() && !found; m++)
										if (are_mutex(base->transitions[check_transitions[l]].action.cubes[m], inv_action))
										{
											found = true;
											vector<int> check_places = base->prev(hse::transition::type, check_transitions[l]);
											boolean::cover invalid_check = 1;
											for (int n = 0; n < (int)check_places.size(); n++)
												invalid_check &= base->places[check_places[n]].effective;

											check &= ~invalid_check;
										}
								}

								// They share a common state encoding such that
								//  - the transition isn't vacuous in that state encoding
								//  - there isn't a vacuous transition in either state that
								//    would put the system into a different state.
								if (!are_mutex(implicant, check))
								{
									// cluster the conflicting places into regions. We want to be able to
									// eliminate entire regions of these conflicts with a single state variable.

									vector<int> r;
									for (int m = 0; m < (int)base->arcs[place::type].size(); m++)
										if (base->arcs[place::type][m].from.index == k)
											r.push_back(base->arcs[place::type][m].to.index);
									for (int m = 0; m < (int)base->arcs[transition::type].size(); m++)
										if (base->arcs[transition::type][m].to.index == k)
											r.push_back(base->arcs[transition::type][m].from.index);

									sort(r.begin(), r.end());

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

		if (p.size() > 1)
		{
			for (int j = 0; j < (int)base->places.size(); j++)
				if (find(p.begin(), p.end(), j) == p.end() && (base->is_reachable(iterator(place::type, i), iterator(place::type, j)) || base->is_reachable(iterator(place::type, j), iterator(place::type, i))) &&
					!base->is_parallel(iterator(place::type, i), iterator(place::type, j)))
					for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
						if (!are_mutex(si.mask(s), base->places[j].effective))
						{
							// cluster the conflicting places into regions. We want to be able to
							// eliminate entire regions of these conflicts with a single state variable.

							vector<int> r;
							for (int m = 0; m < (int)base->arcs[place::type].size(); m++)
								if (base->arcs[place::type][m].from.index == j)
									r.push_back(base->arcs[place::type][m].to.index);
							for (int m = 0; m < (int)base->arcs[transition::type].size(); m++)
								if (base->arcs[transition::type][m].to.index == j)
									r.push_back(base->arcs[transition::type][m].from.index);

							sort(r.begin(), r.end());

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

	for (int i = 0; i < (int)base->places.size(); i++)
		for (int j = 0; j < (int)base->places.size(); j++)
			if (j != i && (base->is_reachable(iterator(place::type, i), iterator(place::type, j)) || base->is_reachable(iterator(place::type, j), iterator(place::type, i))) &&
				!base->is_parallel(iterator(place::type, i), iterator(place::type, j)))
				for (int s = senseless ? -1 : 0; senseless ? s == -1 : s < 2; s++)
					if (!are_mutex(base->places[i].effective.mask(s), base->places[j].effective))
					{
						// cluster the conflicting places into regions. We want to be able to
						// eliminate entire regions of these conflicts with a single state variable.

						vector<int> r;
						for (int m = 0; m < (int)base->arcs[place::type].size(); m++)
							if (base->arcs[place::type][m].from.index == j)
								r.push_back(base->arcs[place::type][m].to.index);
						for (int m = 0; m < (int)base->arcs[transition::type].size(); m++)
							if (base->arcs[transition::type][m].to.index == j)
								r.push_back(base->arcs[transition::type][m].from.index);

						sort(r.begin(), r.end());

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

void encoder::print_conflicts(int sense)
{
	for (int i = 0; i < (int)conflicts.size(); i++)
	{
		if (conflicts[i].sense == sense)
		{
			printf("T%d.%d:{", conflicts[i].index.index, conflicts[i].index.term);
			for (int j = 0; j < (int)conflicts[i].implicant.size(); j++)
			{
				if (j != 0)
					printf(", ");
				printf("P%d", conflicts[i].implicant[j]);
			}
			printf("}\t->\t{");
			for (int j = 0; j < (int)conflicts[i].region.size(); j++)
			{
				if (j != 0)
					printf(", ");
				printf("P%d", conflicts[i].region[j]);
			}
			printf("}\n");
		}
	}
	printf("\n");
}

void encoder::print_suspects(int sense)
{
	for (int i = 0; i < (int)suspects.size(); i++)
	{
		if (suspects[i].sense == sense)
		{
			printf("{");
			for (int j = 0; j < (int)suspects[i].first.size(); j++)
			{
				if (j != 0)
					printf(", ");
				printf("P%d", suspects[i].first[j]);
			}
			printf("}\t->\t{");
			for (int j = 0; j < (int)suspects[i].second.size(); j++)
			{
				if (j != 0)
					printf(", ");
				printf("P%d", suspects[i].second[j]);
			}
			printf("}\n");
		}
	}
	printf("\n");
}

}
