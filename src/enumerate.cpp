#include <set>
#include <map>
#include <algorithm>
#include <fstream>
#include "syntax.hpp"
#include "work.hpp"
#include "build_graph.hpp"
#include "analyze_graph.hpp"
#include "enumerate.hpp"

/////////////////////////// TOP LEVEL ENUMERATE //////////////////////////////////////////////
bool enumerate_loop_merges(std::stack<work_item> &s,
						  work_item &current,
						  vector<optim_fun_chk> const &checks,
						  vector<optim_fun> const &optimizations,
						  vector<rewrite_fun> & rewrites,
						  std::vector<algo> &algos) {
	// Given a the current work item, decide if the graph has been completely
	// evaluated for loop merges and if so return false.  In this case code will
	// be generated.
	// If the graph has not been completely evaluated for loop merging, the work
	// item tells this routine where to look for further optimizations.  Any aditional
	// optimizations found will be pushed onto the stack as new work items.
	
	graph& g = *current.g; 
	
	// get current working level subgraphs
	vector<subgraph*> subs;
	get_current_subgraphs(g, current.nesting, subs);
	
	if (subs.size() == 0) {
		// all nesting levels have been evaluated
		// generate code
		return false;
	}
	
	// map all subgraphs at working level and deeper to subgraphs
	// at working level.
	// sg_map:
	// represent all child subgraphs with current working sg's
	// i.e. if sg1 contains sg2 and sg3 and sg2 contains sg4
	// then sg2,sg3,sg3 all map to sg1.
	map<subgraph_id, subgraph_id> sg_map;
	for (unsigned int i = 0; i < subs.size(); ++i) {
		sg_map[subs[i]->uid] = subs[i]->uid;
		map_subgraphs(subs[i]->uid, subs[i], sg_map);
	}
	
	// Not all combinations of loop merging are legal because of
	// data dependencies.  Those must be found by finding all
	// edges between subgraphs and then evaluating the edges
	// between the subgraphs.
	std::map<subgraph_id,std::set<subgraph_id> > depend;
	for (unsigned int i = 0; i < subs.size(); ++i) {
		//std::cout << "Evaluating: " << subs[i] << "\n";
		subgraph_id puid = subs[i]->parent == 0 ? pair<int,int>(0,0) : subs[i]->parent->uid;
		evaluate_edges(subs[i]->uid, puid, subs[i], subs, 
					   sg_map, g,depend);
	}
    
	map<subgraph_id, std::set<subgraph_id> > common_itr;
	// for all subgraphs at current level of evaluation,
	// create map of sg1 -> {} where sg1 can merge with all
	// sg's in the set
	for (unsigned int i = 0; i < subs.size(); ++i) {
		common_itr[subs[i]->uid] = std::set<subgraph_id>();
	}
	// fill the mapping
	// This is based on common iteration only.
	for (unsigned int i = 0; i < subs.size(); ++i) {
		for (unsigned int j = i+1; j < subs.size(); ++j) {
            string iterations0 = subs[i]->sg_iterator.conditions.begin()->right;
            string iterations1 = subs[j]->sg_iterator.conditions.begin()->right;
			if (iterations0.compare(iterations1) == 0) {
				common_itr[subs[i]->uid].insert(subs[j]->uid);
				common_itr[subs[j]->uid].insert(subs[i]->uid);
			}
		}
	}
    
	// mergable:
	// list of sets of unique cliques
	std::vector<std::set<subgraph_id>* > mergable;
	enumerate(common_itr, depend, mergable);  
    
	// collect list of vertices at current working level
	std::vector<vertex> workingVertices;
	for (unsigned int i = 0; i != g.num_vertices(); ++i) {
		if (g.info(i).op == deleted) {
			continue;
		}
		subgraph *p = g.find_parent(i);
		int depth;
		if (p == NULL)
			depth = 1;
		else
			depth = p->depth() + 1;
		
		if (g.adj(i).size() > 0 && depth == current.nesting)
			workingVertices.push_back(i);
	}
	
	// generate work items
	generate_work_items(g, mergable, workingVertices, checks, 
						optimizations, s, current,rewrites,algos);
	
	return true;
}

////////////////////////// END TOP LEVEL ENUMERATE ///////////////////////////////////////////

//////////////////////////// ENUMERATE MERGING COMBINATIONS //////////////////////////////////

void get_current_subgraphs_r(vector<subgraph*> &sgs, int nesting, vector<subgraph*> & subs
							 ,int current) {
	// recursively find all children subgraphs.
	
	for (unsigned int i = 0; i != sgs.size(); ++i) {
		if (nesting == current) {
			subs.push_back(sgs[i]);
			continue;
		}
		
		get_current_subgraphs_r(sgs[i]->subs,nesting,subs,current+1);
	}
}

void get_current_subgraphs(graph &g, int nesting, vector<subgraph*> & subs) {
	// find all subgraphs at the current level of nesting.
	if (nesting == 1) {
		for (unsigned int i = 0; i != g.subgraphs.size(); i++) {
			subs.push_back(g.subgraphs[i]);
		}
	}
	else {
		get_current_subgraphs_r(g.subgraphs, nesting, subs, 1);
	}
}

bool contains(subgraph_id val, std::vector<subgraph*> &sgs) {
	// returns true if vector sgs contains val
	// false otherwise.
	for (unsigned int i = 0; i < sgs.size(); ++i) {
		if (sgs[i]->uid == val)
			return true;
	}
	return false;
}

bool contains_set(subgraph_id val, std::set<subgraph_id> &sgs) {
	// returns true if set sgs contains val
	// false otherwise
	std::set<subgraph_id>::iterator i = sgs.begin();
	for (; i != sgs.end(); ++i) {
		if (*i == val)
			return true;
	}
	return false;
}

void evaluate_edge_list(std::vector<vertex> &verts, graph &g, 
		std::map<subgraph_id, subgraph_id> &sg_map,
		std::vector<subgraph*> &sgs, subgraph_id curr, 
		subgraph_id curr_parent, 
		std::map<subgraph_id,std::set<subgraph_id> > &depend) {
	
	// Given a list of vertices, specifically the vertices contained within
	// the current subgraph, follow all edges to either
	// a verticec in the current working level or to another subgraph in the
	// current working level.  From the type of the operand of the adjacent
	// vertice, we can determine a data dependance or not.
	
	for (unsigned int i = 0; i < verts.size(); ++i) {
		const std::vector<vertex> &adj = g.adj(verts[i]);
		for (size_t j = 0; j < adj.size(); ++j) {
			// if sg_map does not have this, then move on
			
			subgraph_id id = g.find_parent(adj[j]) == 0 ? pair<int,int>(0,0) : g.find_parent(adj[j])->uid;
			subgraph_id rep = sg_map[id];
			
			// internal edge
			if (rep == curr)
				continue;
			
			// vertex that may represent data dependence
			// a temporary, sumto, or output
			// how far and how aggresively does this need to be followed?
			// sg1 -> tmp -> sg2; pipeline
			// sg1 -> sumto -> sg2; dependency
			if (rep == curr_parent) {
				vertex v = adj[j];
				for (size_t k = 0; k < g.adj(v).size(); ++k) {
					id = g.find_parent(g.adj(v)[k]) == 0 ? pair<int,int>(0,0) : g.find_parent(g.adj(v)[k])->uid;
					subgraph_id rep2 = sg_map[id];
					//std::cout << v << "\t" << g.adj(v)[k] << "\n";
					
					if (rep2 == curr)
						std::cout << "FINISH: enumerate.cpp: evaluate_edge_list: unexpected edge\n";
					else if (contains(rep2, sgs)) {
						switch (g.info(v).op) {
						case sumto:
							//std::cout << "DEP - sumto\n";
							depend[rep2].insert(curr);
							break;
						case temporary:
						case output:
							//std::cout << "Should be pipeable " << v << "\n";
							break;
							
						default:
							std::cout << g.info(v).op << "\n";
							std::cout << "FINISH: enumerate.cpp: evaluate_edge_list: unexpected operation\n";
						}
					}
				}
			}
			
			// edge to another subgraph
			if (contains(rep, sgs))
				std::cout << "FINISH: enumerate.cpp: evaluate_edge_list: edge between subgraphs\n";
		}
	}
}

void evaluate_edges(subgraph_id curr, subgraph_id curr_parent, subgraph *working,
		std::vector<subgraph*> &sgs, std::map<subgraph_id, subgraph_id> &sg_map, 
		graph &g, std::map<subgraph_id,std::set<subgraph_id> > &depend) {
	// given - current subgraph, currents parent
	// given - list of all subgraphs at same level as current
	// given - map of how all subgraphs more nested than current
	//    level are related to current level sg's.
	//    specifically children map to current level parent
	
	// This routine finds data dependencies between subgraphs and
	// returns then in depend.
	
	// this only looks at out edges, because in edges that are of
	// concern will be from
	// another graph and will be caught from that graph
	
	// if an edge is found to a vertice that is not in sg_map
	// and is not parent, then the edge leaves the current level of
	// evaluation and does not effect this search.
	
	// NOTE: This using only the vertices directly contained within
	// the current working subgrah to find out edges of the subgraph.
	// This is working for now, but partitioning may have the ability
	// to break this.  It may be the case that when edges from children
	// subgraphs exit this subgraph, the dependency does not exist at this
	// subgraph.  Also this is for a limited amount of pruning.  If
	// dependencies are missed, extra work will be done, but the correctness
	// will not be effected.
	
	// for each of curr's vertices
	evaluate_edge_list(working->vertices, g, sg_map, sgs, curr, 
			curr_parent,depend);
	for (size_t i = 0; i < working->subs.size(); ++i) {
		evaluate_edges(curr, curr_parent, working->subs[i], sgs, sg_map, 
				g, depend);
	}
}

void map_subgraphs(subgraph_id map_to , subgraph* map_from, 
		std::map<subgraph_id, subgraph_id> &sg_map) {
	// map all of the child subgraphs to sg
	for (size_t i = 0; i < map_from->subs.size(); ++i) {
		sg_map[map_from->subs[i]->uid] = map_to;
		map_subgraphs(map_to, map_from->subs[i], sg_map);
	}
}

std::set<subgraph_id> intersect(std::set<subgraph_id>A, std::set<subgraph_id>B) {
	// set intersection.
	std::set<subgraph_id> C;
	std::set<subgraph_id>::iterator it = A.begin();
	for (; it != A.end(); it++) {
		if (B.find(*it) != B.end()) {
			C.insert(*it);
		}
	}
	return C;
}


bool dependance(std::map<subgraph_id,std::set<subgraph_id> > &depend,
		std::set<subgraph_id> &R) {
	// return true if there is a dependance between any of the subgraphs
	// in the set R.  Else; return false
	std::set<subgraph_id>::iterator i = R.begin();
	for (; i != R.end(); i++) {
		std::set<subgraph_id> &deps = depend[*i];
		std::set<subgraph_id>::iterator j = deps.begin();
		for (; j != deps.end(); j++) {
			if (contains_set(*j, R))
				return true;
		}
	}

	return false;
}

void BK (std::set<subgraph_id> R, std::set<subgraph_id> P, std::set<subgraph_id> X,
		std::map<subgraph_id, std::set<subgraph_id> > common_itr,
		std::map<subgraph_id,std::set<subgraph_id> > &depend,
		std::vector<std::set<subgraph_id>* > &mergable) {
	// Bron-Kerbosch Algorithm modified to enumerate all cliques.
	
	if (R.size() > 1) {
		// This is the only pruning.  If a clique with a direct
		// data dependance exists, discard that clique.
		//std::set<int>::iterator j = R.begin();
		//for (;j != R.end(); ++j)
		//	std::cout << *j << ",";
		//std::cout << "\n";
		
		if (!dependance(depend, R)) {
			mergable.push_back(new std::set<subgraph_id>(R));
		}
		
	}
	
	if (P.size() == 0 && X.size() == 0) {
	}
	else {
		while (P.size() != 0) {
			std::set<subgraph_id>::iterator it = P.begin();
			subgraph_id curr = *it;
			P.erase(it);			
			std::set<subgraph_id> Rnew(R);
			Rnew.insert(curr);
			std::set<subgraph_id> Pnew = intersect(P,common_itr[curr]);
			std::set<subgraph_id> Xnew = intersect(X,common_itr[curr]);
			BK(Rnew,Pnew,Xnew,common_itr,depend,mergable);
			X.insert(curr);
		}
	}
}

void enumerate(std::map<subgraph_id,std::set<subgraph_id> > common_itr,
		std::map<subgraph_id,std::set<subgraph_id> > &depend,
		std::vector<std::set<subgraph_id>* > &mergable) {
	// Use the Bron-Kerbosch Algorithm to enumerate all possible
    // combinations loop mergings.
    // NOTE: This will not enumerate unconnected components of 
    // the graph.  i.e. sg1<->sg2 and sg3<->sg4 will return
    // sg1,sg2,sg3,sg4
    // {sg1 sg2}
    // {sg3,sg4}
    // But sg1,sg2,{sg3,sg4} and {sg1,sg2},{sg3,sg4} must manually
    // be enumerated.
	
	std::set<subgraph_id> vertices;
	std::map<subgraph_id, std::set<subgraph_id> >::iterator it = common_itr.begin();
	for (; it != common_itr.end(); it++) {
		vertices.insert(it->first);
	}

	BK(std::set<subgraph_id>(), vertices, std::set<subgraph_id>(),common_itr,
			depend, mergable);
}

//////////////////////// END ENUMERATE /////////////////////////////////////////////////

//////////////////////// Apply Optimizations ////////////////////////////////////////////

void union_set(std::set<std::pair<subgraph_id,subgraph_id> >&into, std::set<std::pair<subgraph_id,subgraph_id> > &other) {
	// union of two sets
	std::pair<subgraph_id,subgraph_id> w;
	std::set<std::pair<subgraph_id,subgraph_id> >::iterator i = other.begin();
	for (;i != other.end(); ++i) {
		// order pairs
		w = *i;
		if (w.first > w.second)
			w = std::pair<subgraph_id,subgraph_id>(w.second,w.first);
		into.insert(w);
	}
}

int rewrite_graph(graph *g, std::set<subgraph_id> toMerge,
				   std::vector<vertex> &workingVertices,
				   vector<optim_fun_chk> const &checks,
				   vector<optim_fun> const &optimizations,
				   vector<rewrite_fun> & rewrites,
				   std::vector<algo> &algos) {
	// given a set of unique subgraph identifiers, rewrite the graph
	// so that each subgraph in the set is merged.  Return the number
	// of subgraphs left in toMerge.  On total failure return -1	
	
	// To consider
	// 1) subgraphs that may be merges more than one way.
	
	// merges - mapping of optimization id -> sets of pairs
	// the sets of pairs are subgraph identifiers
	// If more than one optimization can produce the same subgraph
	// id pair, we need to ensure either
	//   a) we get the same end graph using either optimization
	//   b) if we do not get the same end graph, we need both
	
	//std::cout << "to merge" << std::endl;
	//for (subgraph_id im : toMerge) {
		//std::cout << im.first << "_" << im.second << ",";
	//}
	//std::cout << "\n";
//
	//std::cout << "working vertices:" << std::endl;
	//for (vertex w : workingVertices) {
		//std::cout << w << ",";
	//}
	//std::cout << std::endl;
	
	std::map<int, std::set<std::pair<subgraph_id,subgraph_id> > >merges;
	for (size_t i = 0; i != checks.size(); ++i) {
		merges[i] = std::set<std::pair<subgraph_id,subgraph_id> >();
	}
	
	//  This is a look to see if two optimizations will ever merge
	// the same two subgraphs.
	// If this pops up, see above.
	// If this is never observed, this check can go.
	for (size_t i = 0; i != checks.size(); ++i) {
		for (size_t j = 0; j != workingVertices.size(); ++j) {
			vertex u = workingVertices[j];
			std::set<std::pair<subgraph_id,subgraph_id> > r;
			// optimization checks must return a set of subgraph id pairs
			r = checks[i](u, *g, toMerge);
			if (r.size()) {
				size_t expected = merges[i].size() + r.size();
				union_set(merges[i], r);
				
				if (expected != merges[i].size()) {
					std::cout << "FINISH: enumerate.cpp: rewrite_graph: multiple optimziation"
					<< " give same merging\n";
					std::cout << expected << "\t" << merges[i].size() << "\n";
				}
			}
		}
	}
	
	bool change = true;
	while ((toMerge.size() > 1) && change) {
		
		change = false;
		for (unsigned int i = 0; i != checks.size(); ++i) {
			for (size_t j = 0; j != g->num_vertices();/*workingVertices.size();*/ ++j) {
				vertex u = workingVertices[j];
                
                u = j; 
                if (g->info(u).op == deleted)
                    continue;

				std::set<std::pair<subgraph_id,subgraph_id> > r;
				std::set<std::pair<subgraph_id,subgraph_id> >::iterator rt;
				// checks must return a set of pairs that can merge.
				// if that set is empty, do nothing
				// else apply the optimization
				r = checks[i](u, *g, toMerge);
				
				//std::cout << "vertex:" << u << "\t" << "amt merge:" <<  r.size() << "\t" << "opt:" << i << "\n";
				
				if (r.size()) {
					rt = r.begin();
					std::pair<subgraph_id,subgraph_id> y = *rt;
					
					//std::cout << "Merging " << u << "\t" << 
					//y.first.first << "_" << y.first.second << "\t" << 
					//y.second.first << "_" << y.second.second << "\t"
                    //<< i << std::endl;
					
					// optimizations returns the new subgraph id that replaced
					// the two id's in y.
					subgraph_id updated = optimizations[i](u,*g,y);
					if (updated == std::pair<int,int>(-1,-1)) {
						std::cout << "ERROR: enumerate.cpp: rewrite_graph: optimization failed\n";
						exit(0);
						return -1;
					}
					
                    //std::ofstream out("lower96.dot");
					//print_graph(out, *g);
					//exit(0);
                    
					// graph maintenance
					lower_graph(algos, *g);
					apply_rewrites(rewrites, *g);
					
					
					// optimizations returns the new subgraph uid
					// toMerge must be LOCALLY updated to represent this
					// both id's in 'y' must be removed from toMerge
					// and 'updated' added.
					toMerge.erase(y.first);
					toMerge.erase(y.second);
					toMerge.insert(updated);
					
					change = true;
				}
			}
		}	
	}
	set<pair<int, int> >::iterator b;
	/*
	std::cout << "returning" << std::endl;
	for (b = toMerge.begin(); b!= toMerge.end(); ++b) {
		std::cout << "(" << b->first << "," << b->second << ")";
	}
	std::cout << std::endl;
	*/
	
	return toMerge.size();
}

void disconnected_comp(graph *g, std::vector<std::set<subgraph_id>* >::iterator ft0,
				  std::set<subgraph_id> &beenMerged,
				  std::vector<std::set<subgraph_id>* >::iterator end,
				  std::vector<vertex> &workingVertices, 
				  vector<optim_fun_chk> const &checks, 
				  vector<optim_fun> const &optimizations,
				  vector<rewrite_fun> &rewrites,
				  vector<algo> &algos,
				  std::stack<work_item> &s,
				  std::string history,
				  int nesting) {
	
	// The method used to enumerate all possible loop fusions
	// can not enumerate all possible combinations of disconnected
	// components.  Consider sg1,sg2,sg3,sg4 where 1 and 2 can merge
	// and 3 and 4 can merge and 1,2,3 and 4 can merge.  The merging of
	// 1,2 and separetly 3,4 is different than the merging of 1,2,3,4.
	//
	// This routine looks at other cliques that do not share currently
	// processed subgraphs and enumerates these.  For example if clique
	// 1,2 and clique 3,4 existed, this routine will merge 1,2 and then
	// fuse 3,4.
	// This occurs recurively adding to the list ofsubgraphs that have
	// already been merged.
	
	std::vector<std::set<subgraph_id>* >::iterator ft1 = ft0;
	ft1++;
	for (;ft1 != end; ft1++) {
		if (intersect(beenMerged,*(*ft1)).size() == 0) {
			graph *newG  = new graph(*g);
			
			if (rewrite_graph(newG, *(*ft1), workingVertices, checks, 
							  optimizations,rewrites,algos) != 1) {
				delete newG;
				//std::cout << "\tGroup is not profitable or cannot be merged\n";
				continue;
			}
			
			// create history string
			std::set<subgraph_id>::iterator it = (*ft1)->begin();
			string lhistory(history);
			lhistory += ",";
			for (; ; ) {
				lhistory += boost::lexical_cast<string>(it->first) + boost::lexical_cast<string>(it->second);
				++it;
				if (it != (*ft1)->end())
					lhistory += ".";
				else
					break;
			}
			
			s.push(work_item(newG, 0,0, lhistory, nesting));	
			
			//recurse
			std::set<subgraph_id> lbeenMerged(beenMerged);
			lbeenMerged.insert((*ft1)->begin(),(*ft1)->end());
			disconnected_comp(newG, ft1, lbeenMerged, end, workingVertices, 
							  checks, optimizations,rewrites,algos,s, lhistory, 
							  nesting);
		}
	}
}

void generate_work_items(graph &g, std::vector<std::set<subgraph_id>* > &mergable,
			std::vector<vertex> &workingVertices,
			vector<optim_fun_chk> const &checks,
			vector<optim_fun> const &optimizations,
			std::stack<work_item> &s,
			work_item &old_wi,
			vector<rewrite_fun> & rewrites,
			std::vector<algo> &algos) {
	
	// convert a list of subgraph cliques into graphs and push onto
	// work stack as work items.  Enumeration of disconnected cliques is
	// handled here.

//#define MAXMERGE
#ifdef MAXMERGE
// used for maxmerge only	
	int maxMerge = 0;
	int stackSize = s.size();
#endif
	
	std::vector<std::set<subgraph_id>* >::iterator ft0 = mergable.begin();
	for (; ft0 != mergable.end(); ++ft0) {
		graph *newG = new graph(g);
		
		//std::set<int>::iterator ft1 = (*ft0)->begin();
		//for (;ft1 != (*ft0)->end(); ++ft1)
		//	std::cout << *ft1 << ",";
		//std::cout << "\n";
		
		
		// merge the loops of the current clique (*ft0)
		// if something in the clique is not mergable, -1 will be returned
		// and that graph can be destroyed
		// This is currently happening because cliques coming into this routine
		// allow merges of unrelated subgraphs, but the current rewrite checkers 
		// and rewriters do not allow this.
		// These cliques must exist because they may appear to be unrelated in
		// isolation but when several other graphs have been merged, they may
		// no longer be isolated.  This would occur in enumeration of disconnected
		// components.
		if (rewrite_graph(newG, *(*ft0), workingVertices, checks, 
							optimizations,rewrites,algos) != 1) {
			delete newG;
			//std::cout << "\tGroup is not profitable or cannot be merged\n";
			continue;
		}
#ifdef MAXMERGE
		if ((*ft0)->size() > maxMerge) {
			maxMerge = (*ft0)->size();
			if (s.size() > stackSize)
				s.pop();
		}
		else continue;
#endif
		// create history string
		std::set<subgraph_id>::iterator it = (*ft0)->begin();
		string history(old_wi.history + "-");
		for (; ; ) {
			history += boost::lexical_cast<string>(it->first) + boost::lexical_cast<string>(it->second);
			++it;
			if (it != (*ft0)->end())
				history += ".";
			else
				break;
		}
		
		s.push(work_item(newG, 0,0, history, old_wi.nesting+1));	

#ifndef MAXMERGE		
		// handle disconnected components.
		std::set<subgraph_id> beenMerged(*(*ft0));
		disconnected_comp(newG, ft0, beenMerged, mergable.end(), workingVertices, 
						  checks, optimizations,rewrites,algos,s, history, 
						  old_wi.nesting+1);
#endif
	}

#ifndef MAXMERGE
	//create unoptimized version
	graph *newG = new graph(g);
	string history(old_wi.history + "-");
	s.push(work_item(newG, 0,0, history, old_wi.nesting+1));
#endif

	// clean up cliques
	ft0 = mergable.begin();
	for (; ft0 != mergable.end(); ft0++) {
		delete(*ft0);
	}
}

/////////////////////////////////////////// End Apply Optimizations ///////////////////////////////
