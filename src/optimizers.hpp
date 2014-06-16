#ifndef OPTIMIZERS_HPP_
#define OPTIMIZERS_HPP_

#include "syntax.hpp"
#include <set>

/////////////////// OPTIMIZATIONS /////////////////////////////////////////////


// checkers - need to return a set containing pairs of subgraph id's for which
// the optimization can merge the loops
// optimizers - need to return the id of the subgraph remaining after the merge.  
// This can be a new unique id, or the id of one of the old subgraphs that was merged.
subgraph_id fuse_loops(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge);
std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops(vertex u, graph& g, std::set<subgraph_id> &toMerge);

subgraph_id fuse_loops_n(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge);
std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops_n(vertex u, graph& g, std::set<subgraph_id> &toMerge);

subgraph_id fuse_loops_nested(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge);
std::set<std::pair<subgraph_id,subgraph_id> > check_fuse_loops_nested(vertex u, graph& g, std::set<subgraph_id> &toMerge);

subgraph_id merge_scalars(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge);
std::set<std::pair<subgraph_id,subgraph_id> > check_merge_scalars(vertex u, graph& g, std::set<subgraph_id> &toMerge);

subgraph_id pipeline(vertex u, graph& g, std::pair<subgraph_id,subgraph_id> &toMerge);
std::set<std::pair<subgraph_id,subgraph_id> > check_pipeline(vertex u, graph& g, std::set<subgraph_id> &toMerge);

// Vectorize inner loops could be an optimization...


int check_reachable_new_r(vertex u, graph &g, 
                           vector<vertex> const&checkVerts,
						   set<subgraph*> &legalSubs, 
                           set<subgraph*> &checkReduction,
                           bool dep, 
						   vector<vertex> &beenChecked);
void propogate_dimension_change(graph &g, vertex u);

#endif /*OPTIMIZERS_HPP_*/
