#include <fstream>
#include "boost/lexical_cast.hpp"
#include "optimizers.hpp"
#include "build_graph.hpp"
#include <string>
#include <algorithm>
#include "type_analysis.hpp"
#include "translate_utils.hpp"
#include "analyze_graph.hpp"

//////////////////////////////////// OPTIMIZER HELPERS ////////////////////////////

template<class T, class I> bool in(const T& id, I begin, I end) {
	return (std::find(begin,end, id) != end);
}

void compute_reachable(vertex u, graph const& g, vector<bool>& mark)
{
	mark[u] = true;
	for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
		vertex v = g.adj(u)[i];
		if (! mark[v])
			compute_reachable(v, g, mark);
	}
}

void gather_descendant_verts(subgraph *sg, vector<vertex> &verts) {
	// gather all vertices from current subgraph as well as all
	// descendants
	
	unsigned int i;
	for (i = 0; i < sg->vertices.size(); ++i) {
		verts.push_back(sg->vertices[i]);
	}
	
	for (i = 0; i < sg->subs.size(); ++i) {
		gather_descendant_verts(sg->subs[i],verts);
	}
	
}

void gather_descendant_subs(subgraph *sg, set<subgraph*> &subs) {
	// gather all descendant subgraphs
	
	unsigned int i;
	for (i = 0; i < sg->subs.size(); ++i) {
		subs.insert(sg->subs[i]);
		gather_descendant_subs(sg->subs[i],subs);
	}
}

void propogate_dimension_change_store(graph &g, vertex u, string bc,
                                      string br, string ld) {
    // recurse up a store chain untill an operation is encountered
    // modifying the stores/temp/sumto on the way.
    
    if (g.info(u).op & (OP_ANY_ARITHMATIC | input | output))
        return;
    
    // update current type
    type *t = &g.info(u).t;
    while (t) {
        t->dim.base_cols = bc;
        t->dim.base_rows = br;
        t->dim.lead_dim	= ld;
        t = t->t;
    }
    
    // continue up store chain
    for (unsigned int i = 0; i < g.inv_adj(u).size(); ++i)
        propogate_dimension_change_store(g, g.inv_adj(u)[i],bc,br,ld);
}

void propogate_dimension_change_get(graph &g, vertex u, string bc,
                                      string br, string ld) {
    // recurse down a get chain untill an operation is encountered
    // modifying the gets on the way.
    
    if (g.info(u).op & (OP_ANY_ARITHMATIC | input | output))
        return;
    
    if (g.info(u).op & (sumto | temporary | input | output)) {
        std::cout << "WARNING: optimizers.cpp; propogate_dimension_change_store(); "
            << "unexpected graph\n";
        return;
    }
        
    // update current type
    type *t = &g.info(u).t;
    while (t) {
        t->dim.base_cols = bc;
        t->dim.base_rows = br;
        t->dim.lead_dim	= ld;
        t = t->t;
    }
    
    // continue up store chain
    for (unsigned int i = 0; i < g.adj(u).size(); ++i)
        propogate_dimension_change_get(g, g.adj(u)[i],bc,br,ld);
}

void propogate_dimension_change(graph &g, vertex u) {
    // when fusion causes a temporary value to move from the
    // top level of the graph (lose a container level) the new
    // container is smaller than the existing.  this new temporary
    // must be updated to reflect that as well as pushing those
    // changes up and down the get and store chains to ensure
    // correct pointer calculations.
    
    // this function takes u which has just changed dimensions (dropped
    // outer container), but has not been updated to reflect that.  
    // first update u then propogate.

    //std::cout << u << "\n" << prnt_detail(&g.info(u).t) << std::endl;
    
    // first determine the new overall dimension of u
    string bc = g.info(u).t.get_highest_column()->dim.dim;
    string br = g.info(u).t.get_highest_row()->dim.dim;
    string ld = "1";
    
    if (bc.compare("1") != 0 && br.compare("1") != 0) {
        // have both row and columns (matrix), so set ld to non 1.
        kind lns = g.info(u).t.get_lowest_ns()->k; 
        if (lns == row)
            ld = br;
        else if (lns == column)
            ld = bc;
    }
    
    // update the type of u
    type *t = &g.info(u).t;
    while (t) {
        t->dim.base_cols = bc;
        t->dim.base_rows = br;
        t->dim.lead_dim	= ld;
        t = t->t;
    }
    
    // propogate store chain
    for (unsigned int i = 0; i < g.inv_adj(u).size(); ++i)
        propogate_dimension_change_store(g, g.inv_adj(u)[i],bc,br,ld);
    
    // propogate down get chains
    for (unsigned int i = 0; i < g.adj(u).size(); ++i)
        propogate_dimension_change_get(g, g.adj(u)[i],bc,br,ld);
    
}

int check_reachable_new_r(vertex u, graph &g, 
                          vector<vertex> const&checkVerts,
						  set<subgraph*> &legalSubs, 
                          set<subgraph*> &reduceCheck, bool dep, 
						  vector<vertex> &beenChecked) {
	// reursively follow edges until either a dead end, or a connection
	// is found to a vertices indicating reachability.
	
	// u is the current vertex in the path
	// checkVerts is the list of vertices that will indicate we have 
	//		a connection
	// legalSubs is the list of subgraphs we must remains within.  If
	//		a path strays outside of these and a connection is found
	//		then there is a dependency.
	// dep indicates if a path has strayed outside of the legal subgraphs.
    //      it is also used to represent a reduction represents a dependancy.
	//		if a vertex matches to checkVerts and dep is true, there
	//		exists a dependency.
	// beenChecked is a list of vertices that is added to each time a
	//		vertex is checked.  If a vertex is gotten to more than once
	//		it indicates the path has already been taken and does not
	//		need to be re-checked.

	// return the vertex that matches if any path out of u can get to 
    // any vertex in check
	// -1 otherwise

	bool ldep;
	
	// determine if this path has already been taken
	// and continue if so
	if (in(u, beenChecked.begin(),beenChecked.end())) {
		return -1;
	}
	beenChecked.push_back(u);
	
	for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
		vertex v = g.adj(u)[i];
        ldep = dep;
        
		// see if the path has left allowed subgraphs
		if (dep || !in(g.find_parent(v),legalSubs.begin(),legalSubs.end())) {
            // if it has left to the top level, we only penalize if its
            // an operation.
            if (dep || g.find_parent(v) || 
                (g.info(v).op & (OP_ANY_ARITHMATIC))) {
                //std::cout << v << " is a dep\n";
                ldep = true;
            }
		}
		
        // if this path has a reduction then there exists a dependency
        // leaving the allowed subgraphs trumps a reduction, but imagine
        // sg0 to sg1 with two direct connections, one a temporary and the
        // other a sumto.  need to ensure that we cannot fuse sg0 and
        // sg1 using the temporary because the reduction exists.
        if (in(g.find_parent(v),reduceCheck.begin(),reduceCheck.end())
             && g.info(v).op == sumto) {
            ldep = true;
        }
        
		// determine if a connection has been found
		if (in(v, checkVerts.begin(), checkVerts.end())) {
			// if a connection has been found and
			// dep is true there is dependency, return true, else
			// no longer need to continue down this path
            
            //std::cout << v << " is in checkVerts; ldep: " << ldep << "\n";
			if (ldep) {
				return v;
			}
			
			continue;
		}
		
		int recurse = check_reachable_new_r(v, g, checkVerts, legalSubs, 
                                            reduceCheck, ldep, beenChecked);
        if (recurse > 0)
			return recurse;
	}
	
	return -1;
}

bool check_reachable_new(subgraph *sg0, subgraph *sg1, graph &g) {
	// Determine if one subgraph is reachable from another subgraph.
	// right now this only checks if sg0 -> sg1 exists.
	
	// Find all vertices in both subgraphs, including all of their 
	// descendants.  Create the set of subgraphs that contains all descendants
	// and ancestors of the two graphs in question.  If any path found from one 
	// of the graphs in question to the other has left the allowed set of 
	// subgraphs, then the path must have passed through another subgraph and 
	// indicates a dependency.  
	
	// collect set of sg0 vertices
	vector<vertex> sg0Verts;
	gather_descendant_verts(sg0, sg0Verts);
	
	// collect set of sg1 vertices
	vector<vertex> sg1Verts;
	gather_descendant_verts(sg1, sg1Verts);
	
	// build set of legal subgraphs
	set<subgraph*> legalSubs;
	legalSubs.insert(NULL);
	subgraph *p = sg0;
	while (p) {
		legalSubs.insert(p);
		p = p->parent;
	}
	p = sg1;
	while (p) {
		legalSubs.insert(p);
		p = p->parent;
	}
    // reductions that happen above sg0 and sg1 are a dependency
    // so keep a separate  list of the parents and look for
    // reductions here
    set<subgraph*> reduceCheck(legalSubs);
    reduceCheck.erase(reduceCheck.find(sg0));
    if (sg0 != sg1)
        reduceCheck.erase(reduceCheck.find(sg1));
    
	gather_descendant_subs(sg0, legalSubs);
	gather_descendant_subs(sg1, legalSubs);
	
	// vector used to determine what has been checked and what has not
	vector<vertex> beenChecked;
	
	// check sg0 -> sg1
	for (unsigned int i = 0; i < sg0Verts.size(); ++i) {
		if (check_reachable_new_r(sg0Verts[i], g, sg1Verts, legalSubs, 
								  reduceCheck, false, beenChecked) > 0) {
			return true;
		}
	}
	
	// does sg1 -> sg0 need to happen???
	
	return false;
}

// sg1 depends on sg2 if sg1 uses stuff computed by sg2
bool depends_on(subgraph* sg1, subgraph* sg2, graph const& g)
{
	vector<bool> mark(g.num_vertices(), false);
	for (unsigned int i = 0; i != sg2->vertices.size(); ++i) {
		vertex u = sg2->vertices[i];
		compute_reachable(u, g, mark);
	}
	for (unsigned int i = 0; i != sg1->vertices.size(); ++i) {
		vertex u = sg1->vertices[i];
		if (mark[u])
			return true;
	}
	return false;
}

int useless_temporary(vertex u, graph& g) {
	if ((g.info(u).op == temporary
			|| g.info(u).op == output
			|| g.info(u).op == store_column
		    || g.info(u).op == store_add_column
			|| g.info(u).op == store_row
			|| g.info(u).op == store_add_row
			|| g.info(u).op == store_element
			|| g.info(u).op == store_add_element)
			//&& g.info(u).eval == defer // if flagged as evaluate, could be an output variable!
			&& g.adj(u).size() >= 1 && g.inv_adj(u).size() == 1) {
		
		vector<type> prevCasts;
		vector<type> succCasts;
		
		int prev = g.inv_adj(u)[0];
		while (g.info(prev).op == partition_cast) {
			prevCasts.push_back(g.info(prev).t);
			prev = g.inv_adj(prev)[0];
		}
		
        //std::ofstream out4("lower7.dot");
        //print_graph(out4, g);
        //out4.close();
		for (unsigned int i = 0; i != g.adj(u).size(); i++) {
			int succ = g.adj(u)[i];
			succCasts.clear();
            bool singleCast = true;
			while (g.info(succ).op == partition_cast) {
				succCasts.push_back(g.info(succ).t);
                if (g.adj(succ).size() > 1) {
                    singleCast = false;
                    break;
                }
				succ = g.adj(succ)[0];
			}
			
            if (!singleCast) {
                // a cast has two out edges and therefore two
                // different casts somewhere in the chaing
                // we are trying to fuse.
                continue;
            }

            if (g.find_parent(prev) && g.find_parent(succ) &&
                check_reachable_new(g.find_parent(prev), 
                                    g.find_parent(succ), g))
                continue;
            
			if (succCasts.size() == 0 && prevCasts.size() > 1) {
				std::cout << "optimizers.cpp; useless_temporary(); complete me\n";
			}
			else if (succCasts.size() > 1 && prevCasts.size() == 0) {
				// tmp -> cast -> cast ... get
				// tmp and last cast must be of the same type
				if (total_type_match(g.info(u).t,succCasts[succCasts.size()-1]))
					return i;
			}
			else if (succCasts.size() > 0 && prevCasts.size() > 0) {

				bool castMatch = true;
				for (unsigned int c = 0; c < succCasts.size(); ++c) {
					if (!total_type_match(succCasts[c], prevCasts[c])) {
						castMatch = false;
						break;
					}
				}
		
				if (castMatch)
					return i;
			}
			else {
				if (g.info(u).t.t) {
					if ((g.info(u).t.t->k == column && g.info(prev).t.k == column
						 && (g.info(succ).op == get_column 
							 || g.info(succ).op == get_column_from_row))
						|| (g.info(u).t.t->k == row && g.info(prev).t.k == row
							&& (g.info(succ).op == get_row
								|| g.info(succ).op == get_row_from_column))
						|| ((g.info(u).t.k == row || g.info(u).t.k == column)
							&& g.info(prev).t.k == scalar
							&& g.info(succ).op == get_element)) {
							return i;
						}
				}
				else {
					if ((g.info(u).t.k == row || g.info(u).t.k == column)
						&& g.info(prev).t.k == scalar
						&& g.info(succ).op == get_element) {
						return i;
					}
				}
			}
		}
		return -1;
	} 
	else
		return -1;
}

/////////////////////////////////END OPTIMIZER HELPERS ////////////////////////////


//////////////////////////////////// OPTIMIZERS ///////////////////////////////

/*
given
     A.0
  /       \
A.1(i)   A.2(i)
look from A.0 at A.1 and A.2 to see if they get the same data
are unmerged, and their graphs are possitioned for legal fusion
*/

std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops_n(vertex u, graph& g, std::set<subgraph_id> &toMerge)
{	
    std::set<std::pair<subgraph_id,subgraph_id> > compat;
	
    //std::cout << u << ":\t" << toMerge.begin()->second << "\n";
    //std::ofstream out4("lower7.dot");
    //print_graph(out4, g);
    //out4.close();
    
	if (g.adj(u).size() < 2)
		return compat;

    vector<vertex> check0, check1;
	    
    subgraph_id id, id1;

    for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
        bool deadEnd = true;

        // crawl down until we find a subgraph we are looking to merge, or a dead end
		while (g.adj(iv).size() == 1 && (g.info(iv).op == partition_cast || g.info(iv).op & OP_GET)) {
            id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
            if (id == pair<int,int>(0,0) || (! in(id, toMerge.begin(), toMerge.end()))) {
                check0.push_back(iv);
                iv = g.adj(iv)[0];
            }
            else {
                deadEnd = false;
                break;
            }
		}
        
        // if we dead end, or end on something that is not a get, continue.
        if (deadEnd || !(g.info(iv).op & OP_GET))
            continue;
        
        // now iv is a get operation in a subgraph that we care to merge.
        
        for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
            check1.clear();           
            vertex jv = g.adj(u)[j];
            
            deadEnd = true;
            
            // crawl down until we find a subgraph we are looking to merge, or a dead end
            while (g.adj(jv).size() == 1 && (g.info(jv).op == partition_cast || g.info(jv).op & OP_GET)) {
                id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
                if (id1 == pair<int,int>(0,0) || (! in(id1, toMerge.begin(), toMerge.end()))) {
                    check1.push_back(jv);
                    jv = g.adj(jv)[0];
                }
                else {
                    deadEnd = false;
                    break;
                }
            }
            
            // if we dead end, or end on something that is not a get, continue.
            if (deadEnd || !(g.info(jv).op & OP_GET))
                continue;
            
            // now iv and jv are get operations in subgraphs that we care to merge.
            //std::cout << iv << "\t" << jv << "\n";
            //std::cout << toMerge.begin()->second << "\n";
            
            // ops match?
            if (g.info(jv).op != g.info(iv).op)
                continue;
            
            // already merged?
            if (g.find_parent(jv) == g.find_parent(iv))
                continue;
            
            // one subgraph up must be the same, or these cannot merge.
            if (g.find_parent(jv)->parent != g.find_parent(iv)->parent)
                continue;
            
            // are these two iterators compatible
            if (!g.find_parent(jv)->sg_iterator.fusable(
                     g.find_parent(iv)->sg_iterator,true)) {
                continue;
            }
            
            // what about the path each took to get here?  are these the same?
            if (check0.size() != check1.size())
                continue;
            
            // each of the vertices in these two checks should have the same type
            // where same type means
            // 1) container orientation matches all the way down.
            // 2) iteration... a little tricky, for now just look at if the 
            //      two vertices have the same parent and that means that
            //      the types have same iteration up to current fusion
            //      and should just have to ensure current fusion has 
            //      common iteration.
            vector<vertex>::iterator itr0, itr1;
            itr0 = check0.begin();
            itr1 = check1.begin();
            bool allMatch = true;
            
            for (; itr0 != check0.end(); ++itr0, ++itr1) {
                // 1
                if (!total_type_match(g.info(*itr0).t, g.info(*itr1).t)) {
                    allMatch = false;
                    break;
                }
                // 2
                if (g.find_parent(*itr0) != g.find_parent(*itr1)) {
                    allMatch = false;
                    break;
                }
            }
            
            if (!allMatch)
                continue;
            
            // all parent subgraphs are fused up to here, and the types on the way
            // down look to be compatibable (likely require a union of partition sizes)
            
            // is there a dependence?
            if (depends_on(g.find_parent(iv), g.find_parent(jv), g)
                || depends_on(g.find_parent(jv), g.find_parent(iv), g)) 
                continue;
            
            // finally, check the iterations of the two current subgraphs.
            if (g.mergeable(jv,iv)) 
                compat.insert(std::pair<subgraph_id,subgraph_id> (id,id1));
        }

    }
    
    return compat;
}

subgraph_id fuse_loops_n(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge) {
    if (g.adj(u).size() < 2)
		return std::pair<int,int>(-1,-1);
    
    vector<vertex> check0, check1;
    
    subgraph_id id, id1;
    
    for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
        bool deadEnd = true;
        
        // crawl down until we find a subgraph we are looking to merge, or a dead end
		while (g.adj(iv).size() == 1 && (g.info(iv).op == partition_cast || g.info(iv).op & OP_GET)) {
            id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
            if (id == pair<int,int>(0,0) || ((id != toMerge.first) && (id != toMerge.second))) {
                check0.push_back(iv);
                iv = g.adj(iv)[0];
            }
            else {
                deadEnd = false;
                break;
            }
		}
        
        // if we dead end, or end on something that is not a get, continue.
        if (deadEnd || !(g.info(iv).op & OP_GET))
            continue;
        
        // now iv is a get operation in a subgraph that we care to merge.
        
        for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
            check1.clear();           
            vertex jv = g.adj(u)[j];
            
            deadEnd = true;
            
            // crawl down until we find a subgraph we are looking to merge, or a dead end
            while (g.adj(jv).size() == 1 && (g.info(jv).op == partition_cast || g.info(jv).op & OP_GET)) {
                id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
                if (id1 == pair<int,int>(0,0) || ((id1 != toMerge.first) && (id1 != toMerge.second))) {
                    check1.push_back(jv);
                    jv = g.adj(jv)[0];
                }
                else {
                    deadEnd = false;
                    break;
                }
            }
            
            // if we dead end, or end on something that is not a get, continue.
            if (deadEnd || !(g.info(jv).op & OP_GET))
                continue;
            
            // now iv and jv are get operations in subgraphs that we care to merge.
            
            // ops match?
            if (g.info(jv).op != g.info(iv).op)
                continue;
            
            // already merged?
            if (g.find_parent(jv) == g.find_parent(iv))
                continue;
            
            // one subgraph up must be the same, or these cannot merge.
            if (g.find_parent(jv)->parent != g.find_parent(iv)->parent)
                continue;
            
            // are these two iterators compatible
            if (!g.find_parent(jv)->sg_iterator.fusable(
                   g.find_parent(iv)->sg_iterator,true)) {
                continue;
            }
            
            // what about the path each took to get here?  are these the same?
            if (check0.size() != check1.size())
                continue;
            
            // each of the vertices in these two checks should have the same type
            // where same type means
            // 1) container orientation matches all the way down.
            // 2) iteration... a little tricky, for now just look at if the 
            //      two vertices have the same parent and that means that
            //      the types have same iteration up to current fusion
            //      and should just have to ensure current fusion has 
            //      common iteration.
            vector<vertex>::iterator itr0, itr1;
            itr0 = check0.begin();
            itr1 = check1.begin();
            bool allMatch = true;
            
            for (; itr0 != check0.end(); ++itr0, ++itr1) {
                // 1
                if (!total_type_match(g.info(*itr0).t, g.info(*itr1).t)) {
                    allMatch = false;
                    break;
                }
                // 2
                if (g.find_parent(*itr0) != g.find_parent(*itr1)) {
                    allMatch = false;
                    break;
                }
            }
            
            if (!allMatch)
                continue;
            
            // all parent subgraphs are fused up to here, and the types on the way
            // down look to be compatibable (likely require a union of partition sizes)
            
            // is there a dependence?
            if (depends_on(g.find_parent(iv), g.find_parent(jv), g)
                || depends_on(g.find_parent(jv), g.find_parent(iv), g)) 
                continue;
            
            // finally, check the iterations of the two current subgraphs.
            int merge = g.mergeable(jv,iv);
         
            if (merge) {
                //std::cerr << "merge gets " << u << std::endl;
                
                subgraph *sg1 = g.find_parent(jv);
                subgraph *sg2 = g.find_parent(iv);
                
                if (merge & 0x2) {
                    // must merge iteration variables from
                    // two graphs.  this happens when iteration
                    // variables are both partitions. i.e. $$x and $$y
                    g.merge_iterOps(sg1->sg_iterator.conditions,
                                  sg2->sg_iterator.conditions);
                    update_iters(g);
                }
                if (merge & 0x4) {
                    // must merge step variables from
                    // two graphs.  this happens when step
                    // variables are both partitions. i.e. $$x and $$y
                    g.merge_iterOps(sg1->sg_iterator.updates,
                                  sg2->sg_iterator.updates);
                    update_iters(g);
                }
                
                return g.merge(g.find_parent(jv), g.find_parent(iv));
            }
        }
        
    }
    
    return std::pair<int,int>(-1,-1);;
}

/*
given
     A.0
  /       \
A.1(i)   A.2(i)
look from A.0 at A.1 and A.2 to see if they get the same data
are unmerged, and their graphs are possitioned for legal fusion
*/

std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops(vertex u, graph& g, 
		std::set<subgraph_id> &toMerge) {	
	std::set<std::pair<subgraph_id,subgraph_id> > compat;
	
	if (g.adj(u).size() < 2)
		return compat;
		
	vector<vertex> check0, check1;
	vector<vertex>::iterator tci;
	
	for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
		while (g.info(iv).op == partition_cast) {
			check0.push_back(iv);
			iv = g.adj(iv)[0];
		}
		
		// we only care to merge if its in our toMerge list.
		subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
		if (id == pair<int,int>(0,0) || (! in(id, toMerge.begin(), toMerge.end())))
			continue;
		
		if (g.info(iv).op == get_row || g.info(iv).op == get_column 
			|| g.info(iv).op == get_row_from_column 
			|| g.info(iv).op == get_column_from_row
			|| g.info(iv).op == get_element) {
			for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
				check1.clear();
				tci = check0.begin();
				
				vertex jv = g.adj(u)[j];
                
                //std::cout << iv << "\t" << jv << "\n";
                //std::ofstream out4("lower10.dot");
                //print_graph(out4, g);
                //out4.close();
                
				bool allMatch = true;
				while (g.info(jv).op == partition_cast) {
					if (tci != check0.end() && total_type_match(g.info(*tci).t,g.info(jv).t)) {
						check1.push_back(jv);
						jv = g.adj(jv)[0];
                        tci++;
					}
					else {
						allMatch = false;
						break;
					}
				}
                
				// collection of type casts do match
				if (!allMatch)
					continue;

				// Already fused
				if (g.find_parent(jv) == g.find_parent(iv)) 
                    continue;
                
				// we only care to merge if its in our toMerge list.
				subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
				if (id1 == pair<int,int>(0,0) || (! in(id1, toMerge.begin(), toMerge.end())))
					continue;
				
                // are these two iterators compatible
                if (!g.find_parent(jv)->sg_iterator.fusable(
                          g.find_parent(iv)->sg_iterator,true)) {
                    continue;
                }

				if (g.info(jv).op == g.info(iv).op
					&& (! (depends_on(g.find_parent(iv), g.find_parent(jv), g)
						   || depends_on(g.find_parent(jv), g.find_parent(iv), g)))) {
					
					if (g.mergeable(jv,iv)) {
						compat.insert(std::pair<subgraph_id,subgraph_id> (id,id1));
					}
				}//if
			}//for
		}//if
	}//for
	
	return compat;
}

subgraph_id fuse_loops(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge) 
{
    
	if (g.adj(u).size() < 2)
		return std::pair<int,int>(-1,-1);
	
	vector<vertex> check0, check1;
	vector<vertex>::iterator tci;
	
	for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
		while (g.info(iv).op == partition_cast) {
			check0.push_back(iv);
			iv = g.adj(iv)[0];
		}
		
		// we only care to merge if its in our toMerge list.
		subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
				
		if (id == pair<int,int>(0,0) || ((id != toMerge.first) && (id != toMerge.second)))
			continue;
		
		if (g.info(iv).op == get_row || g.info(iv).op == get_column 
			|| g.info(iv).op == get_row_from_column 
			|| g.info(iv).op == get_column_from_row
			|| g.info(iv).op == get_element) {
			for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
				check1.clear();
				tci = check0.begin();
				
				vertex jv = g.adj(u)[j];
				
				bool allMatch = true;
				while (g.info(jv).op == partition_cast) {
					if (tci != check0.end() && total_type_match(g.info(*tci).t,g.info(jv).t)) {
						check1.push_back(jv);
						jv = g.adj(jv)[0];
                        tci ++;
					}
					else {
						allMatch = false;
						break;
					}
				}
				
				// collection of type casts do match
				if (!allMatch)
					continue;
						
				// Already fused
				if (g.find_parent(jv) == g.find_parent(iv))
					continue;
                
				// we only care to merge if its in our toMerge list.
				subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
				if (id1 == pair<int,int>(0,0) || ((id1 != toMerge.first) && (id1 != toMerge.second)))
					continue;
				
                // are these two iterators compatible
                if (!g.find_parent(jv)->sg_iterator.fusable(
                          g.find_parent(iv)->sg_iterator,true)) {
                    continue;
                }
                
				if (g.info(jv).op == g.info(iv).op
					&& (! (depends_on(g.find_parent(iv), g.find_parent(jv), g)
						   || depends_on(g.find_parent(jv), g.find_parent(iv), g)))) {
					
					int merge = g.mergeable(jv,iv);
					if (merge) {
						//std::cerr << "merge gets " << u << std::endl;
						
						subgraph *sg1 = g.find_parent(jv);
						subgraph *sg2 = g.find_parent(iv);
						
						if (merge & 0x2) {
							// must merge iteration variables from
							// two graphs.  this happens when iteration
							// variables are both partitions. i.e. $$x and $$y
							g.merge_iterOps(sg1->sg_iterator.conditions,
                                          sg2->sg_iterator.conditions);
							update_iters(g);
						}
						if (merge & 0x4) {
							// must merge step variables from
							// two graphs.  this happens when step
							// variables are both partitions. i.e. $$x and $$y
							g.merge_iterOps(sg1->sg_iterator.updates,
                                          sg2->sg_iterator.updates);
							update_iters(g);
						}
						
						return g.merge(g.find_parent(jv), g.find_parent(iv));
					} //if
				}//if
			}//for
		}//if
	}//for

	return std::pair<int,int>(-1,-1);
}

std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops_nested(vertex u, graph& g, std::set<subgraph_id> &toMerge)
{	
	/*
	imagine
	         d
	        / \
	sg0(nop)   sg1(nop)
	      /	   \
	sg2(get)		sg3(get)
	
	we would like to be able to merge sg0 and sg1
	*/
	
	// NOTE: this check assumes that the toMerge set id's all have the SAME minor id.
	int minorId = toMerge.begin()->second;

	std::set<std::pair<subgraph_id,subgraph_id> > compat;
	
	if (g.adj(u).size() < 2)
		return compat;
		
	vector<vertex> check0, check1;
	vector<vertex>::iterator tci;
	
	for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
		while (g.info(iv).op == partition_cast) {
			check0.push_back(iv);
			iv = g.adj(iv)[0];
		}
		
		// we only care to merge if its in our toMerge list.
		subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
		
		if (id == pair<int,int>(0,0))
			continue;
		
		// do we have matching major id's and minor ids which is <= something in our list
		if (minorId > id.second)
			continue;
		bool haveMatch = false;
		std::set<subgraph_id>::iterator itr = toMerge.begin();
		for (; itr != toMerge.end(); ++itr) {
			if (itr->first == id.first) {
				haveMatch = true;
				break;
			}
		}
		
		if (!haveMatch)
			continue;
			
		//std::cout << "check" << id << "\n";
		
		if (g.info(iv).op == get_row || g.info(iv).op == get_column 
			|| g.info(iv).op == get_row_from_column 
			|| g.info(iv).op == get_column_from_row
			|| g.info(iv).op == get_element) {
			for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
				check1.clear();
				tci = check0.begin();
				
				vertex jv = g.adj(u)[j];
				
				bool allMatch = true;
				while (g.info(jv).op == partition_cast) {
					if (tci != check0.end() && total_type_match(g.info(*tci).t,g.info(jv).t)) {
						check1.push_back(jv);
						jv = g.adj(jv)[0];
					}
					else {
						allMatch = false;
						break;
					}
				}
				
				// collection of type casts do match
				if (!allMatch)
					continue;
				
				// we only care to merge if its in our toMerge list.
				subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
				if (id1 == pair<int,int>(0,0))
					continue;
                
				// do we have matching major id's and minor ids which is <= something in our list
				if (minorId > id1.second)
					continue;
				haveMatch = false;
				std::set<subgraph_id>::iterator itr = toMerge.begin();
				for (; itr != toMerge.end(); ++itr) {
					if (itr->first == id1.first) {
						haveMatch = true;
						break;
					}
				}
				
				if (!haveMatch)
					continue;
				
				haveMatch = true;
				subgraph *sg1 = g.find_parent(jv);
				subgraph *sg2 = g.find_parent(iv);

                // are these two iterators compatible
                if (!sg1->sg_iterator.fusable(sg2->sg_iterator,true)) {
                    continue;
                }
                
				while (sg1 && sg1->uid.second > minorId && sg2 && sg2->uid.second > minorId) {
					// Already fused
					if (sg1 == sg2) {
						haveMatch = false;
						break;
					}
					
					sg1 = sg1->parent;
					sg2 = sg2->parent;
				}
				
				if (!haveMatch || sg1 == NULL || sg2 == NULL)
					continue;
				
                if (sg1->parent != sg2->parent)
                    continue;
                
				id1 = sg1->uid;
				id = sg2->uid;
				
				if (g.info(jv).op == g.info(iv).op
					&& (!(depends_on(g.find_parent(iv), g.find_parent(jv), g)
						   || depends_on(g.find_parent(jv), g.find_parent(iv), g)))) {
					if (g.mergeable_majorID_at_minorID(sg1,sg2,minorId)) {
                        //std::ofstream out4("lower7.dot");
                        //print_graph(out4, g);
                        //out4.close();
                        
						compat.insert(std::pair<subgraph_id,subgraph_id> (id,id1));
					}
				}//if
			}//for
		}//if
	}//for
	
	return compat;
}

subgraph_id fuse_loops_nested(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge) 
{
	/*
	imagine
	         d
	        / \
	sg0(nop)   sg1(nop)
	      /	   \
	sg2(get)		sg3(get)
	
	we would like to be able to merge sg0 and sg1
	*/
	
	if (g.adj(u).size() < 2)
		return std::pair<int,int>(-1,-1);
	
	if (toMerge.first.second != toMerge.second.second)
		return std::pair<int,int>(-1,-1);
	
	vector<vertex> check0, check1;
	vector<vertex>::iterator tci;
	
	for (unsigned int i = 0; i < g.adj(u).size(); ++i) {
		check0.clear();
		vertex iv = g.adj(u)[i];
		
		while (g.info(iv).op == partition_cast) {
			check0.push_back(iv);
			iv = g.adj(iv)[0];
		}
		
		// we only care to merge if its in our toMerge list.
		subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
		
		// only check major id's here
		if (id == pair<int,int>(0,0)) 
			continue;
		if (((id.first != toMerge.first.first) || (toMerge.first.second > id.second))
			&& ((id.first != toMerge.second.first) || (toMerge.second.second > id.second)))
			continue;
		
		if (g.info(iv).op == get_row || g.info(iv).op == get_column 
			|| g.info(iv).op == get_row_from_column 
			|| g.info(iv).op == get_column_from_row
			|| g.info(iv).op == get_element) {
			for (unsigned int j = i + 1; j < g.adj(u).size(); ++j) {
				check1.clear();
				tci = check0.begin();
				
				vertex jv = g.adj(u)[j];
				
				bool allMatch = true;
				while (g.info(jv).op == partition_cast) {
					if (tci != check0.end() && total_type_match(g.info(*tci).t,g.info(jv).t)) {
						check1.push_back(jv);
						jv = g.adj(jv)[0];
					}
					else {
						allMatch = false;
						break;
					}
				}
				
				// collection of type casts do match
				if (!allMatch)
					continue;
				
				// Already fused
				if (g.find_parent(jv) == g.find_parent(iv))
					continue;
				
				// we only care to merge if its in our toMerge list.
				// only check major id's here
				subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
				if (id1 == pair<int,int>(0,0))
					continue;
				if (((id1.first != toMerge.first.first) || (toMerge.first.second > id1.second))
					&& ((id1.first != toMerge.second.first) || (toMerge.second.second > id1.second)))
					continue;				
				
				subgraph *sg1 = g.find_parent(jv);
				subgraph *sg2 = g.find_parent(iv);
				while (sg1 && sg1->uid.second > toMerge.first.second)
					sg1 = sg1->parent;
				while (sg2 && sg2->uid.second > toMerge.first.second)
					sg2 = sg2->parent;
				
                // are these two iterators compatible
                if (!sg1->sg_iterator.fusable(sg2->sg_iterator,true)) {
                    continue;
                }
                
				if (sg1 == NULL || sg2 == NULL)
					return std::pair<int,int>(-1,-1);
				
				if (g.info(jv).op == g.info(iv).op
					&& (! (depends_on(sg1, sg2, g)
						   || depends_on(sg2, sg1, g)))) {
					
					int merge = g.mergeable_majorID_at_minorID(sg1,sg2,toMerge.first.second);
					if (merge) {
						//std::cerr << "merge gets " << u << std::endl;
												
						if (merge & 0x2) {
							// must merge iteration variables from
							// two graphs.  this happens when iteration
							// variables are both partitions. i.e. $$x and $$y
							g.merge_iterOps(sg1->sg_iterator.conditions,
                                          sg2->sg_iterator.conditions);
							update_iters(g);
						}
						if (merge & 0x4) {
							// must merge step variables from
							// two graphs.  this happens when step
							// variables are both partitions. i.e. $$x and $$y
							g.merge_iterOps(sg1->sg_iterator.updates,
                                          sg2->sg_iterator.updates);
							update_iters(g);
						}
						
						return g.merge(sg1, sg2);
					} //if
				}//if
			}//for
		}//if
	}//for
	
	return std::pair<int,int>(-1,-1);
}


std::set<std::pair<subgraph_id,subgraph_id> > check_merge_scalars(vertex u, graph& g, std::set<subgraph_id> &toMerge)
{
	std::set<pair<subgraph_id,subgraph_id> > result;
	if (g.info(u).t.k == scalar) {
		for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
			vertex iv = g.adj(u)[i];
			
			// we only care to merge if its in our toMerge list.
			subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
			
			if (id == pair<int,int>(0,0) || (! in(id, toMerge.begin(), toMerge.end())))
				continue;
			
			if (g.info(iv).t.k == scalar) {
				for (unsigned int j = i + 1; j != g.adj(u).size(); ++j) {
					vertex jv = g.adj(u)[j];
					
					// we only care to merge if its in our toMerge list.
					subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
					if (id1 == pair<int,int>(0,0) || (! in(id1, toMerge.begin(), toMerge.end())))
						continue;
					
					if (g.info(jv).t.k == scalar
						&& (! (depends_on(g.find_parent(iv), g.find_parent(jv), g)
							   || depends_on(g.find_parent(jv), g.find_parent(iv), g)))) {
						if (g.mergeable(iv,jv)) {
							subgraph_id id0 = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
							subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
							result.insert(pair<subgraph_id,subgraph_id>(id0,id1));
						}
					}//if
				}//for
			}//if
		}//for
	} 

	return result;
}

subgraph_id merge_scalars(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge)
{
	if (g.info(u).t.k == scalar) {
		for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
			vertex iv = g.adj(u)[i];
			// we only care to merge if its in our toMerge list.
			subgraph_id id = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
			
			if (id == pair<int,int>(0,0) || ((id != toMerge.first) && (id != toMerge.second)))
				continue;
			
			if (g.info(iv).t.k == scalar) {
				for (unsigned int j = i + 1; j != g.adj(u).size(); ++j) {
					vertex jv = g.adj(u)[j];
					
					// we only care to merge if its in our toMerge list.
					subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
					if (id1 == pair<int,int>(0,0) || ((id1 != toMerge.first) && (id1 != toMerge.second)))
						continue;
					
					if (g.info(jv).t.k == scalar
						&& (! (depends_on(g.find_parent(iv), g.find_parent(jv), g)
							   || depends_on(g.find_parent(jv), g.find_parent(iv), g)))) {
						
						//std::cerr << "merge scalars " << u << std::endl;
												
						int merge = g.mergeable(iv,jv);
						if (merge) {
							//std::cerr << "merge gets " << u << std::endl;
							
							subgraph* sg1 = 0, *sg2 = 0;
							sg1 = g.find_parent(iv);
							sg2 = g.find_parent(jv);
							assert(0 != sg1 && 0 != sg2);
							
							if (merge & 0x2) {
								// must merge iteration variables from
								// two graphs.  this happens when iteration
								// variables are both partitions. i.e. $$x and $$y
								g.merge_iterOps(sg1->sg_iterator.conditions,
                                              sg2->sg_iterator.conditions);
								update_iters(g);
							}
							if (merge & 0x4) {
								// must merge step variables from
								// two graphs.  this happens when step
								// variables are both partitions. i.e. $$x and $$y
								g.merge_iterOps(sg1->sg_iterator.conditions,
                                              sg2->sg_iterator.conditions);
								update_iters(g);
							}
							return g.merge(sg1, sg2);
						}
					}//if
				}//for
			}//if
		}//for
	}
	return std::pair<int,int>(-1,-1);
}

std::set<std::pair<subgraph_id,subgraph_id> > check_pipeline(vertex u, graph& g, std::set<subgraph_id> &toMerge)
{
	// if we have a temporary that is consumed by a get_column
	// and that is produced by a column vector, then 
	// remove the temporary and merge the two subgraphs
	//std::cout << u << "\n";
	
	// NOTE: PROBLEM: useless_temporary can have several adj vertices
	// but it always finds, the same one.  
    // Data depency analysis has been pulled into useless_temporary,
    // i think this will take care of this problem.
	
	std::set<std::pair<subgraph_id,subgraph_id> > result;
	
	int ut = useless_temporary(u,g);
	if (ut < 0) {
		return result;
	}
	
	//std::cout << u << "\t" << ut << "\t" << g.adj(u)[ut] << "\n";
	
	vector<type> predCasts;
	vector<type> succCasts;
	
	vertex iv = g.inv_adj(u)[0];
	while (g.info(iv).op == partition_cast) {
		predCasts.push_back(g.info(iv).t);
		iv = g.inv_adj(iv)[0];
	}
	vertex jv = g.adj(u)[ut];
	while (g.info(jv).op == partition_cast) {
		succCasts.push_back(g.info(jv).t);
		jv = g.adj(jv)[0];
	}
	
	subgraph_id id0 = g.find_parent(iv) == 0 ? pair<int,int>(0,0) : g.find_parent(iv)->uid;
	if (id0 == pair<int,int>(0,0) || (! in(id0,toMerge.begin(), toMerge.end()))) {
		return result;
	}
	subgraph_id id1 = g.find_parent(jv) == 0 ? pair<int,int>(0,0) : g.find_parent(jv)->uid;
	if (id1 == pair<int,int>(0,0) || (! in(id1,toMerge.begin(), toMerge.end()))) {
		return result;
	}
	
    // ensure type series of type casts lead to compatible merge
	if (succCasts.size() == 0 && predCasts.size() >= 1) {
		std::cout << "optimizers.cpp; check_pipleline(); complete me\n";
	}
	else if (succCasts.size() >= 1 && predCasts.size() == 0) {
		// tmp -> cast -> cast ... get
		// tmp and last cast must be of the same type
		if (!total_type_match(g.info(u).t,succCasts[succCasts.size()-1]))
			return result;
	}
	else if (succCasts.size() > 0 && predCasts.size() > 0) {
		
		bool castMatch = true;
		for (unsigned int c = 0; c < succCasts.size(); ++c) {
			if (!total_type_match(succCasts[c], predCasts[c])) {
				castMatch = false;
				break;
			}
		}
		
		if (!castMatch)
			return result;
	}
    
	if (g.mergeable(iv, jv)) {
		/*
		must check for other dependencies, this can be seen in gemver
		sg0
		|	\
		sg1  |
		|	|
		sg2	|
		|	|
		\   /
		 sg3
		sg0 and sg3 appear to be pipable, but sg3 depends on sg0 in two
		places
		*/
		
		// Find this case by finding all vertices that sg0 can get to.  If
		// it can get to a vertice in sg3 that is not the connection between
		// sg0 -> sg3, then there must exists another dependency.

		if (check_reachable_new(g.find_parent(iv), g.find_parent(jv), g)) {
			return result;
		}
		result.insert(pair<subgraph_id,subgraph_id>(id0,id1));
	}
		
	return result;
}

subgraph_id pipeline(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge)
{
	//std::cout << "inside pipeline " << u << std::endl;
	
	// if we have a temporary that is consumed by a get_column
	// and that is produced by a column vector, then 
	// remove the temporary and merge the two subgraphs
	int ut = useless_temporary(u,g);

	if (ut < 0)
		return std::pair<int,int>(-1,-1);
	
	vector<vertex> toClear;
	vector<type> predCasts;
	vector<type> succCasts;
	
	vertex pred = g.inv_adj(u)[0];
	while (g.info(pred).op == partition_cast) {
		toClear.push_back(pred);
		predCasts.push_back(g.info(pred).t);
		pred = g.inv_adj(pred)[0];
	}
	vertex succ = g.adj(u)[ut];
	while (g.info(succ).op == partition_cast) {
		toClear.push_back(succ);
		succCasts.push_back(g.info(succ).t);
		succ = g.adj(succ)[0];
	}

	//std::cout << pred << "\t" << succ << "\n";
	
	subgraph_id id0 = g.find_parent(pred) == 0 ? pair<int,int>(0,0) : g.find_parent(pred)->uid;
	if (id0 == pair<int,int>(0,0) || ((id0 != toMerge.first) && (id0 != toMerge.second)))
		return std::pair<int,int>(-1,-1);
	subgraph_id id1 = g.find_parent(succ) == 0 ? pair<int,int>(0,0) : g.find_parent(succ)->uid;
	if (id1 == pair<int,int>(0,0) || ((id1 != toMerge.first) && (id1 != toMerge.second)))
		return std::pair<int,int>(-1,-1);

	//std::cout << u << "\n";
	

	
	// ensure type series of type casts lead to compatible merge
	if (succCasts.size() == 0 && predCasts.size() >= 1) {
		std::cout << "optimizers.cpp; pipleline(); complete me\n";
	}
	else if (succCasts.size() >= 1 && predCasts.size() == 0) {
		// tmp -> cast -> cast ... get
		// tmp and last cast must be of the same type
		if (!total_type_match(g.info(u).t,succCasts[succCasts.size()-1]))
			return std::pair<int,int>(-1,-1);
	}
	else if (succCasts.size() > 0 && predCasts.size() > 0) {
		
		bool castMatch = true;
		for (unsigned int c = 0; c < succCasts.size(); ++c) {
			if (!total_type_match(succCasts[c], predCasts[c])) {
				castMatch = false;
				break;
			}
		}
		
		if (!castMatch)
			return std::pair<int,int>(-1,-1);
	}
	//if (!total_type_match(g.info(u).t,g.info(g.inv_adj(succ)[0]).t)) {
	//	return std::pair<int,int>(-1,-1);
	//}
	
	int merg = g.mergeable(pred, succ);
	//std::cout << "merg: " << merg << "\n";
	if (merg) {

		subgraph* sg1 = 0, *sg2 = 0;
		sg1 = g.find_parent(pred);
		sg2 = g.find_parent(succ);
		assert(0 != sg1 && 0 != sg2);
		
		/*
		must check for other dependencies, this can be seen in gemver
		sg0
		|	\
		sg1  |
		|	|
		sg2	|
		|	|
		\   /
		 sg3
		sg0 and sg3 appear to be pipable, but sg3 depends on sg0 in two
		places
		*/
		
		// Find this case by finding all vertices that sg0 can get to.  If
		// it can get to a vertice in sg3 that is not the connection between
		// sg0 -> sg3, then there must exists another dependency.
		
		if (check_reachable_new(sg1, sg2, g)) {
			return std::pair<int,int>(-1,-1);
		}
		
		//std::cout << "Merging " << sg1->uid.first << "," << sg1->uid.second << "\t";
		//std::cout << "with\t" << sg2->uid.first << "," << sg2->uid.second << "\t";
        //std::cout << merg << "\n";
        
		if (merg & 0x2) {
			// must merge iteration variables from
			// two graphs.  this happens when iteration
			// variables are both partitions. i.e. $$x and $$y
			g.merge_iterOps(sg1->sg_iterator.conditions,
                          sg2->sg_iterator.conditions);
			update_iters(g);
		}
		if (merg & 0x4) {
			// must merge step variables from
			// two graphs.  this happens when step
			// variables are both partitions. i.e. $$x and $$y
			g.merge_iterOps(sg1->sg_iterator.updates,
                          sg2->sg_iterator.updates);
			update_iters(g);
		}
		
		//std::cerr << "about to pipeline " << u << std::endl;
		//std::cout << prnt_detail(&g.info(u).t) << "\n";
		
		/*
            pred
             |
             u               <-- (temporary)
          /  |  \
         0   1  ut(succ)     <-- (ut returned by useless_temporary)
               / | \
              succ_adj
        */ 
		
        // pred and succ are get and store of the same type with succ_adj
        // getting the same element type of succ and pred.  so we can
        // just add edges from pred to all of succ_adj and remove succ.
        // pred and succ are merged into the same subgraph.
        // then there are two cases to deal with for u and pred
        // 1) succ is the only adjacent edge to u:
        //      if u is not an output, then u can be deleted and 
        //      pred made into a temporary
        // 2) u has multiple adj edges:
        //      u must remain in tack for the other adjacent edges.
        //      pred must remain a store to u.
        
        // fuse the graphs
        subgraph_id newID = g.merge(sg1, sg2);
        //std::cout << newID.first << "\t" << newID.second << "\n";
		//std::cout << "merged " << u << std::endl;
        
        // add the edges from pred -> succ_adj vertices
        g.move_out_edges_to_new_vertex(succ, pred);
        
        if (g.info(u).op != output && g.adj(u).size() == 1) {
            // if the old temporary can go away remove it
            // and modify the size of the container to 
            // reflect the smaller overall size.
            
			if (g.info(pred).op != sumto)
				g.info(pred).op = temporary;
			g.clear_vertex(u);
            
            // any cast chains around u are now useless, so remove
            // them all
            for (unsigned int i = 0; i < toClear.size(); ++i) {
				//std::cout << toClear[i] << "\n";
				g.clear_vertex(toClear[i]);
			}
            
            // remove succ
            g.clear_vertex(succ);
            
            // creating this temporary changes the base container of the 
            // new temporary. if this new temporary is moved completely 
            // into the the new fused loops then container dimension
            // changes.  those changes need to be pushed up and down
            // so pointer updates are computed correctly.
            propogate_dimension_change(g, pred);
		}
        else {
            // if u must remain in tack for other output edges, then
            // only need to remove succ and clean up any cast chain
            // from ut -> succ.
            
            // cast chain first
            vertex c = g.adj(u)[ut];
            while (g.info(c).op == partition_cast) {
                vertex next = g.adj(c)[0];
                //std::cout << "clearing " << c << "\n";
                g.clear_vertex(c);
                c = next;					
            }
            
            // remove succ
            g.clear_vertex(succ);
        }	

		return newID;
	}
	else
		return std::pair<int,int>(-1,-1);
}


/////////////////////////////////////////// END OPTIMIZERS NEW ////////////////////////////////


