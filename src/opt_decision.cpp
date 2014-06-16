#include "opt_decision.hpp"
#include "optimize_analysis.hpp"
#include <iostream>
#include <fstream>

boost::mt19937 rng;

/*
// 2 threads (no search), tile size 1024 doubles  (no search)
int MAX_PARTS = 2;
int partition_bounds[2][3] = { {2,2,1}, {1024,1024,128}};
*/

/* OptPoint methods {{{ */

OptPoint::OptPoint(unsigned int n) : fsgr(n), num_nodes(n), num_parts(n), partitions(n) {
	// Things we need:
	// First, num_parts is initialized to 0 by default.
	// Second, partitions needs a vector for each n.
	// That vector should have MAX_PARTS elements, each with the 
	// minimal number for way and blocksize.
	for (unsigned int opnum=0; opnum<n; ++opnum) {
		vector<PartitionChoice> v(MAX_PARTS);
		for (int partnum=0; partnum < MAX_PARTS; ++partnum) {
			v[partnum].way = m;
			v[partnum].blocksize = partition_bounds[partnum][0]; 
		}
		partitions[opnum] = v;
	}

	// Finally, fusegraph needs edges w/ 0 for each pt
	for (unsigned int i=0; i < n; ++i) {
		for (unsigned int j=i+1; j < n; ++j) {
			boost::add_edge(i,j,0,fsgr);
		}
	}
}

unsigned int OptPoint::size() {
	return num_nodes + 2*MAX_PARTS*num_nodes + num_nodes * (num_nodes-1) / 2;
}

/* 
 *	There's actually a constant-time way to find these index numbers
 *	But as best I can tell it requires some hairy math with square roots
 *	i think this is less error prone and possibly faster for small i or n
 */
pair<unsigned int,unsigned int> numToMat(unsigned int n, unsigned int i) {
	//int old_n = n;
	//int old_i = i;
	--n; // 1st row has n-1
	unsigned int r = 0;
	while (i >= n) {
		i -= n;
		--n;
		r++;
	}
	// cout << "numToMat(" << old_n << ", " << old_i << ") = (" << r << ", " << i+r+1 << ")" << endl;
	return pair<unsigned int,unsigned int>(r,i+r+1);
}

void OptPoint::set(unsigned int i, unsigned int val) {
	if (i < num_nodes*(num_nodes-1)/2) {
		pair<unsigned int,unsigned int> indx = numToMat(num_nodes, i);
		pair<FuseEdge,bool> e = edge(indx.first, indx.second, fsgr);
		if (e.second) {
			fsgr[e.first] = val;
		} // else error?
	} else {
        if (MAX_PARTS == 0)
            return;
        
		i = i-num_nodes*(num_nodes-1)/2;
		if (i < 2*MAX_PARTS*num_nodes) {
			int opnum = i / (2*MAX_PARTS);
			i = i % (2*MAX_PARTS);
			int plevel = i / 2;
			i = i % 2;
			if (i == 0) {
				partitions[opnum][plevel].way = (Way)val;
			} else {
				partitions[opnum][plevel].blocksize = val;
			}
		} else {
			i -= 2*MAX_PARTS*num_nodes;
			num_parts[i] = val;
		}
	}
}
void OptPoint::orthoSet(unsigned int i,unsigned int val) {
	if (i < num_nodes*(num_nodes-1)/2) {
		pair<unsigned int,unsigned int> indx = numToMat(num_nodes, i);
		pair<FuseEdge,bool> e = edge(indx.first, indx.second, fsgr);
		if (e.second) {
			fsgr[e.first] = val;
		} // else error?
	} else {
		i = i-num_nodes*(num_nodes-1)/2;
		if (i < 2*MAX_PARTS*num_nodes) {
			int opnum = i / (2*MAX_PARTS);
			i = i % (2*MAX_PARTS);
			int plevel = i / 2;
			i = i % 2;
			if (i == 0) {
				partitions[opnum][plevel].way = (Way)val;
			} else {
				partitions[opnum][plevel].blocksize = val;
			}
		} else {
			i -= 2*MAX_PARTS*num_nodes;
			if (val != num_parts[i]) {
				pair<out_edge_iter, out_edge_iter> egs = boost::out_edges(i, fsgr);
				for (; egs.first != egs.second; ++egs.first) {
					int k = fsgr[*egs.first];
					int s = source(*egs.first, fsgr); // assert s = i
					int t = target(*egs.first, fsgr);
					int oldmax = max(num_parts[s],num_parts[t]);
					int newmax;
					newmax = max(num_parts[t],val);
					// ex 1: was at 2,2, becomes 1,2, maxes stay the same, no change
					// ex 2: was at 0,0 becomes 2,0,  +2
					if (k > 0) {
						fsgr[*egs.first] = k+ newmax - oldmax;
					}
				}
			}
			num_parts[i] = val;
		}
	}
}

unsigned int OptPoint::get(unsigned int i) {
	if (i < num_nodes*(num_nodes-1)/2) {
		pair<unsigned int,unsigned int> indx = numToMat(num_nodes, i);
		pair<FuseEdge,bool> e = edge(indx.first, indx.second, fsgr);
		if (e.second) {
			return fsgr[e.first];
		} // else error?
	} else {
        if (MAX_PARTS == 0)
            return 0;
        
		i = i-num_nodes*(num_nodes-1)/2;
		if (i < 2*MAX_PARTS*num_nodes) {
			int opnum = i / (2*MAX_PARTS);
			i = i % (2*MAX_PARTS);
			int plevel = i / 2;
			i = i % 2;
			if (i == 0) {
				return partitions[opnum][plevel].way;
			} else {
				return partitions[opnum][plevel].blocksize;
			}
		} else {
			i -= 2*MAX_PARTS*num_nodes;
			return num_parts[i];
		}
	}
	return -1;
}

unsigned int get_max_fuse(FuseGraph &fsgr) {
	unsigned int maxFuse = 0;
	
	pair<edge_iter, edge_iter> e = boost::edges(fsgr);
	for (;e.first != e.second; ++(e.first)) {
		if (fsgr[*e.first] > maxFuse)
			maxFuse = fsgr[*e.first];
	}
	return maxFuse;
}

void OptPoint::print(vector<OperationInfo> &op_nodes) {
	
	unsigned int maxFuse = get_max_fuse(fsgr);	
	bool none = true;
	
	cout << "Fusions:" << endl;
	vector<FuseVertex> found;
	for (unsigned int depth = 1; depth <= maxFuse; ++depth) {
		std::cout << "depth " << depth << ":\n";
		found.clear();
		pair<vertex_iter, vertex_iter> verts = vertices(fsgr);
		for (; verts.first != verts.second; ++(verts.first)) {
			if (find(found.begin(),found.end(),*verts.first) != found.end())
				continue;
			
			none = false;
			std::cout << "\t{" << op_nodes[*verts.first].id;
			pair<adj_edge_iter, adj_edge_iter> adjEdges = out_edges(*verts.first, fsgr);
			for (; adjEdges.first != adjEdges.second; ++(adjEdges.first)) {
				if (!fsgr[*adjEdges.first] || fsgr[*adjEdges.first] < depth)
					continue;
				
				std::cout << ", " << op_nodes[target(*adjEdges.first,fsgr)].id;
				found.push_back(target(*adjEdges.first,fsgr));
			}
			std::cout << "}\n";
			
			found.push_back(*verts.first);
		}
	}
	if (none)
		std::cout << "\tnone\n";
	std::cout << "\n";
	

	
	std::cout << "Partitions: (way, block size)\n";
	
	for (unsigned int i = 0; i < num_nodes; ++i) {
		std::cout << "\t" << op_nodes[i].id << ": ";
		for (unsigned int j = 0; j < num_parts[i]; ++j) {
			std::cout << "(" << partitions[i][j].way << ","
			<< partitions[i][j].blocksize << ")\t";
		}
		std::cout << "\n";
	}

	std::cout << "\n\n";
}

/* }}} */

/* OptSpace methods {{{ */

ParamBound OptSpace::bounds(OptPoint point, unsigned int i) {
	ParamBound p;
	unsigned int num_nodes = op_nodes.size();
	if (i < num_nodes*(num_nodes-1)/2) {
		pair<unsigned int,unsigned int> indx = numToMat(num_nodes,i);
		int max_depth = min(op_nodes[indx.first].maxFuseDepth,
							op_nodes[indx.second].maxFuseDepth) +
						min(point.num_parts[indx.first],point.num_parts[indx.second]);
		p.low = 0;
		p.high = max_depth;
		p.stride = 1;
	} else {
		i -= num_nodes*(num_nodes-1)/2;
		if (i < 2*MAX_PARTS*num_nodes) {
			int opnum = i / (2*MAX_PARTS);
			i = i % (2*MAX_PARTS);
			unsigned int plevel = i / 2;
			if (i%2 == 0) {
				if (point.num_parts[opnum] >= plevel+1) {
					// legitimate partition
					p.low = 0;
					p.high = 2;
					p.stride = 1; 
				} else {
					// force no search over non-used partitions
					p.low = 0;
					p.high = 0;
					p.stride = 1; 
				}
			} else {
				if (point.num_parts[opnum] >= plevel+1) {
					// legitimate partition
					p.low = partition_bounds[plevel][0];
					p.high = partition_bounds[plevel][1];
					p.stride = partition_bounds[plevel][2];
				} else {
					// force no search over non-used partitions
					p.low = partition_bounds[plevel][0];
					p.high = partition_bounds[plevel][0];
					p.stride = 1;
				}
			}
		} else {
			// Number of partitions
			p.low = 0;
			p.high = MAX_PARTS;
			p.stride = 1;
		}
	}
	return p;
}

map<vertex, vector<PartitionChoice> > OptSpace::mapPartition(OptPoint *p) {
	map<vertex, vector<PartitionChoice> > m;
	for (size_t i=0; i < op_nodes.size(); ++i) {
		// resize truncates the vector to only include the first num_parts[i] partitions
		m[op_nodes[i].id] = p->partitions[i]; // make copy
		//cout << "numparts[i]" << p->num_parts[i] << endl;
		m[op_nodes[i].id].resize(p->num_parts[i]); //truncate it
	}
	return m;
}


// This constructor should take a reference to 
// a lowered unfused graph (the result of initial_lower)
// and builds the search space
OptSpace::OptSpace(graph &g) {
	vector<subgraph*>::iterator i;
	for (i = g.subgraphs.begin(); i != g.subgraphs.end(); ++i) {
		OperationInfo op;
		op.id = (*i)->getMajorID();
		op.maxFuseDepth = (*i)->getMinorID() ;
		vector<subgraph*>::iterator b = (*i)->subs.begin();
		vector<subgraph*>::iterator e = (*i)->subs.end();
		while (b != e) {
			op.maxFuseDepth += 1; // subgraph has subs, so depth+1
			// We only need to look at the first element of subs
			// because at these stage each subgraph should only have 1
			// child. more are added only after fusing.
			e = (*b)->subs.end();
			b = (*b)->subs.begin();
		}
		op_nodes.push_back(op);
	}
	int s = op_nodes.size();
	search_dimension = s + 2*MAX_PARTS*s + s*(s-1)/2;
	max_fuse = 0;
	for (int n=0; n < s; ++n) {
		if (op_nodes[n].maxFuseDepth > max_fuse) {
			max_fuse = op_nodes[n].maxFuseDepth;
		}
	}
}

bool OptSpace::checkPoint(OptPoint *p) {
	for (size_t i=0; i != op_nodes.size(); ++i) {
		vector<PartitionChoice> pc = p->partitions[i];
		for (size_t j=p->num_parts[i]; j < pc.size() ; ++j) {
			if (pc[j].way != m || // default minimum
				pc[j].blocksize != partition_bounds[j][0]) {
				return false;
			}
		}
	}
	return true;
}

/* }}} */

/* OptPoint utility functions {{{ */
void printSet(set<vertex> s) {
	cout << "{";
	for (set<vertex>::iterator i = s.begin(); i != s.end(); ++i) {
		cout << *i << ", ";
	}
	cout << "}";
}

void printPartition(map<vertex, vector<PartitionChoice> > &parts) {
	map<vertex, vector<PartitionChoice> >::iterator pmap;
	cout << "Paritions by vertex:" << endl;
	for (pmap = parts.begin(); pmap != parts.end(); ++pmap) {
		cout << pmap->first << ": ";
		vector<PartitionChoice>::iterator choice;
		for (choice = (pmap->second).begin(); choice != (pmap->second).end(); ++choice) {
			cout << "(" << choice->way << ", " << choice->blocksize << ") ";
		}
		cout << endl;
	}
}

void printFuse(FuseGraph &fsgr) {
	cout << "Fusions:" << endl;
	pair<edge_iter, edge_iter> e = boost::edges(fsgr);
	for (;e.first != e.second; ++(e.first)) {
		FuseVertex v1 = boost::source(*e.first, fsgr);
		FuseVertex v2 = boost::target(*e.first, fsgr);
		cout << "(" << v1 << ", " << v2 << ") = " << fsgr[*e.first] << endl;
	}
}

/* 
 * Input: depth, graph of parent depth
 * Output: graph of lower depth
 * The purpose of this is to build a graph w/ info only at the spec. depth
 */
FuseGraph build_fuse_graph(unsigned int depth, unsigned int numNodes, FuseGraph parentlevel) {
	//cout << "building graph for depth = " << depth << endl;
	//cout << "numNodes = " << numNodes << endl;
	if (numNodes <= 1) {
		return parentlevel;  //nothing to do
	}
	FuseGraph fg(numNodes); //do I need to worry about empty sets?
	pair<edge_iter, edge_iter> e = boost::edges(parentlevel);
	for (;e.first != e.second; ++(e.first)) {
		if (parentlevel[*e.first] >= depth) {
			FuseVertex v1 = boost::source(*e.first, parentlevel);
			FuseVertex v2 = boost::target(*e.first, parentlevel);
			boost::add_edge(v1, v2, parentlevel[*e.first], fg);
			//cout << "adding edge " << v1 << " + " << v2 << " at depth " << depth << endl;
			//cout << "requested depth: " << parentlevel[*e.first] << endl;
		}
	}
	//at this point fg should have all the edges of the parent at current depth
	return fg;
}


/* fusesets should be empty to start */
void makeFuseSetsFromGraph(FuseGraph fg, int maxfuse, vector<OperationInfo> op_nodes,
		 vector<vector<set<int> > > &fusesets) {
	//cout << "input graph" << endl;
	//printFuse(fg);
	int depth;
	int maxdepth = MAX_PARTS + maxfuse;
	//cout << "max depth = " << maxdepth << endl; 
	for (depth = 1; depth <= maxdepth; ++depth) {
		fg = build_fuse_graph(depth, op_nodes.size(), fg);
		vector<int> component(num_vertices(fg));
		int num = boost::connected_components(fg, &component[0]);
		//cout << "num components:" << num << endl;
		vector<int>::size_type i;
		vector<set<int> > fuse_ids(num);
		for (i = 0; i != component.size(); ++i) {
			//cout << i << " in component " << component[i] << endl;
			fuse_ids[component[i]].insert(i);
		}
		fusesets.push_back(fuse_ids);
	}
}

/* }}} */

/* getPerformance and friends {{{ */

/* remap takes applies the fusions specified in fg to workGraph
 * using rewriteGraph 
 */
bool remap(graph *workGraph, FuseGraph fg, vector<unsigned int> num_parts,
           vector<OperationInfo> op_nodes,
           bool strict,
           build_details_t &bDet) {
	
	int depth;
	int MAXDEPTH = get_max_fuse(fg);
	if (op_nodes.size() <= 1) {
		return true;
	}

	for (depth = 1; depth <= MAXDEPTH; ++depth) {
		// find all working vertices
		std::vector<vertex> workingVertices;
		for (size_t i = 0; i != workGraph->num_vertices(); ++i) {
			if (workGraph->info(i).op == deleted) {
				continue;
			}
			subgraph *p = workGraph->find_parent(i);
			int v_depth;
			if (p == NULL)
				v_depth = 0;
			else
				v_depth = p->getMinorID();
			
			if (workGraph->adj(i).size() > 0 && (depth-1) == v_depth)
				workingVertices.push_back(i);
		}

		fg = build_fuse_graph(depth, op_nodes.size(), fg); //ok to overwrite?
		vector<unsigned int> component(num_vertices(fg));
		unsigned int num = boost::connected_components(fg, &component[0]);
		vector<unsigned int>::size_type i;
		vector<set<unsigned int>> fuse_ids(num);
		bool modified = false;
		for (i = 0; i != component.size(); ++i) {
			for (auto c : fuse_ids[component[i]]) {
				unsigned int source, target;
				if (c < i) {
					source = i;
					target = c;
				} else {
					source = c;
					target = i;
				}
				pair<FuseEdge, bool> e = boost::edge(source, target, fg);
				if (!e.second && strict) {
					std::cout << "Error in finding edge!!" << endl;
					return false;
				}
				if (fg[e.first] == 0 && strict) {
					//trying to build, but didn't specify for every edge
					std::cout << "Not obeying transitivity" << endl;
					return false;
				}
				if (fg[e.first] > min(op_nodes[source].maxFuseDepth,
						op_nodes[target].maxFuseDepth)+
						min(num_parts[source],num_parts[target])) {
					cout << "trying to fuse more than the number of loops" << endl;
					return false;
				}
						
			}
			fuse_ids[component[i]].insert(i); 
		}
		
		for (set<unsigned int> mergeSet : fuse_ids) {
			if (mergeSet.size() > 1) {
				set<subgraph_id> pair_set;
				//cout << "merge set: ";
				for (auto m : mergeSet) {
					//cout << "(" << op_nodes[*m].id << "," << depth << ")";
					pair_set.insert(pair<int,int>(op_nodes[m].id, depth));
				}
				//cout << endl;
				int a = rewrite_graph(workGraph, pair_set, workingVertices,
                                      *bDet.checks, *bDet.optimizations, 
                                      *bDet.rewrites, *bDet.algos);
				if (a != 1) {
					// cout << "can't rewrite" << endl;
					return false;
				}
				modified = true;
			}
		}
		if (!modified) {
			break;
		}
	}
	return true;
}

int ssuid = 0;
bool buildPartition(graph *g, 
                    map<vertex, vector<PartitionChoice> > &partitions,
                    build_details_t &bDet) {
    
    
    vector<rewrite_fun> &check = *bDet.part_checks;
    vector<partition_fun> &partitioners = *bDet.partitioners;
    vector<algo> &algos = *bDet.algos;
    vector<rewrite_fun> &rewrites = *bDet.rewrites;
    
	// 0: part_mult_left_result
	// 1: part_mult_right_result
	// 2: part_mult_left_right 	
	// 3: part_mult_scl_row
	// 4: part_mult_scl_col
	// 5: partition_add_col
	// 6: partition_add_row
	// 7: part_mult_scl
	// 8: partition_add

	map<vertex, vector<PartitionChoice> >::iterator it = partitions.begin();
	
	for (; it != partitions.end(); ++it) {
		vertex u = it->first;
		
		vector<PartitionChoice> &pc = it->second;
		
		vector<PartitionChoice>::iterator jt = pc.begin();

		
		for (; jt != pc.end(); ++jt) {
			// assuming following
			// C = A op B
			// mult
			//	m : shared dimension of A,C
			//	n : shared dimension of B,C
			//	k : shared dimension of A,B
			// add/subtract
			//	m : rows
			//	n : columns
			//	k : invalid
			
			switch (g->info(u).op) {
				case add:
				case subtract: {
					
					int partitioner;
					if (jt->way == m)
						partitioner = 5;
					else if (jt->way == n)
						partitioner = 6;
					else {
						return false;
					}
						
					if (!check[partitioner](u,*g)) {
						return false;
					}

					vertex left = g->inv_adj(u)[0];
					vertex right = g->inv_adj(u)[1];
					vertex_info castLeft(g->info(left).t,partition_cast,g->info(left).label);
					vertex_info castRight(g->info(right).t,partition_cast,g->info(right).label);
					vertex_info castResult(g->info(u).t,partition_cast,g->info(u).label);
					
					vertex cleft = g->add_vertex(castLeft);
					vertex cright = g->add_vertex(castRight);
					vertex cu = g->add_vertex(castResult);
					
					g->remove_edges(left,u);
					g->remove_edges(right,u);
					
					g->add_edge(left,cleft);
					g->add_edge(right,cright);
					
					g->add_edge(cleft,u);
					g->add_edge(cright,u);
					
					g->move_out_edges_to_new_vertex(u,cu);
					g->add_edge(u,cu);
					
					partitioners[partitioner](u,*g,false);
                    
					// look for cast removal
					apply_rewrites(rewrites, *g);
					// must update algorithms
					assign_algorithms(algos, *g);
					
					
					break;
				}
				case multiply: {
					
					// no need to introduce a cast if there is no
					// change.  
					bool left = false;
					bool right = false;
					bool result = false;
					
					int partitioner;
					if (jt->way == m) {
						if (check[0](u,*g)) {
							partitioner = 0;
							left = true;
							result = true;
						}
						else if (check[4](u,*g)) {
							partitioner = 4;
							result = true;
							if (g->info(g->inv_adj(u)[0]).t.k == scalar) {
								right = true;
							}
							else {
								left = true;
							}
						}
						else {
							return false;
						}
					}
					else if (jt->way == n) {
						if (check[1](u,*g))  {
							partitioner = 1;
							right = true;
							result = true;
						}
						else if (check[3](u,*g)) {
							partitioner = 3;
							if (g->info(g->inv_adj(u)[0]).t.k == scalar) {
								right = true;
							}
							else {
								left = true;
							}
						}
						else {
							return false;
						}
					}
					else if (jt->way == k) {
						if (check[2](u,*g)) {
							partitioner = 2;
							left = true;
							right = true;
						}
						else  {
							return false;
						}
					}
					else {
						return false;
					}
							
					if (left) {
						vertex left = g->inv_adj(u)[0];
						vertex_info castLeft(g->info(left).t,partition_cast,g->info(left).label);
						vertex cleft = g->add_vertex(castLeft);
						
						// left must be carefull to preserve ordering
						g->remove_edges(left,u);
						g->add_edge(left,cleft);
						g->add_left_operand(cleft,u);
					}
					if (right) {
						vertex right = g->inv_adj(u)[1];
						vertex_info castRight(g->info(right).t,partition_cast,g->info(right).label);
						vertex cright = g->add_vertex(castRight);
						
						// right should be ok as far as ordering goes.
						g->remove_edges(right,u);
						g->add_edge(right,cright);
						g->add_edge(cright,u);
					}
					if (result) {
						vertex_info castResult(g->info(u).t,partition_cast,g->info(u).label);
						vertex cu = g->add_vertex(castResult);
						
						// preserve ordering
						g->move_out_edges_to_new_vertex(u,cu);
						g->add_edge(u,cu);
					}
                    
					partitioners[partitioner](u,*g,false);
					
                    //std::ofstream out("lower97.dot");
                    //print_graph(out, *g);
                    //out.close();
                    
					// look for cast removal
					apply_rewrites(rewrites, *g);
					// must update algorithms
					assign_algorithms(algos, *g);
					
					break;
				}
				default:
					std::cout << "opt_decision.cpp; buildPartition(); unexpected operation type\n";
					return false;
			}
		}
	}
	
	// if we made it this far we have sucessfully partitioned the graph
	// this graph needs to be checked.
	
	return true;
}


int buildVersion(graph *g, 
				 map<vertex, vector<PartitionChoice> > &partitions,
				 FuseGraph fg, 
				 vector<unsigned int> num_parts,
				 vector<OperationInfo> &op_nodes,
                 int vid, bool strict,
                 build_details_t &bDet) {
	// perform specified partitioning and fusion.
	// 
	// on success return true, else false
		
	bool result;
	//cout << "buildVersion" << endl;
	
	result = buildPartition(g, partitions, bDet);

	if (!result) {
		// std::cout << "Unable to introduce partitions\n";
		return -1;
	}

	// attach partition information to the graph.
	get_partition_information(*g);
	
	// getting this to return a bool would be nice.
	initial_lower(*g, *bDet.algos, *bDet.rewrites);
    
    //std::ofstream out4("lower10.dot");
    //print_graph(out4, *g);
    //out4.close();
	//std::ofstream out2(string("lower" + boost::lexical_cast<string>(200+vid) 
	//						 + ".dot").c_str());
	//print_graph(out2, *g);
	//out2.close();
        
	if (result) {
		result = remap(g,fg,num_parts, op_nodes, strict,
                       bDet);
	}

    //std::ofstream out5("lower11.dot");
    //print_graph(out5, *g);
    //out5.close();
    
	//std::ofstream out3(string("lower" + boost::lexical_cast<string>(300+vid) 
	//						  + ".dot").c_str());
	//print_graph(out3, *g);
	//out3.close();
	
	if (!result) {
		// std::cout << "Unable to introduce fusions\n";
		return -2;
	}
		
	// update partition information (fusion can change the number of
	// unique and therfore the deps)
	update_partition_information(*g);
	
	// run some basic checks to ensure the partitions match those that
	// were requested and if true, map the symbolic names to the requested
	// sizes
	if (!map_partitions_to_sizes(*g, partitions)) {
		std::cout << "Partitions in graph do not match those requested\n";
		return -3;
	}
		
	return 1;
}

#include "test_generator.hpp"
double evaluateVersion(graph &g, int vid, int threadDepth,
                       compile_details_t &cDet,
                       build_details_t &bDet) {
	
	// return cost or -1.0 on failure
	// if both empirical and model are specified??? 
    // for now just return empirical if we have it
	
	versionData *verData = new versionData(vid);
	
	// if (cDet.useModel) {
		// model is on
		// bDet.modelMessage->threadDepth = threadDepth;
		// modelVersionSingle(g, *bDet.models, vid, cDet.routine_name, 
                           // *bDet.modelMessage, verData);
	// }
	
	if (cDet.useEmpirical) {
		std::list<versionData*> orderedVersions;
		orderedVersions.push_back(verData);
		
		runEmpiricalTest(cDet.tmpPath, cDet.fileName, orderedVersions, 
						 __INT_MAX__, *bDet.modelMessage);
	}
	
    if (cDet.runCorrectness) {
        std::list<versionData*> orderedVersions;
		orderedVersions.push_back(verData);
        //std::cout << "Running correctness\n";
        runCorrectnessTest(cDet.tmpPath, cDet.fileName, orderedVersions);
    }
    
	double cost = -1.0;
	
	
	if (cDet.useEmpirical) {
		if (verData->get_model_by_name(empirical))
			cost = (verData->get_model_by_name(empirical))->get_back();
	}
	else if (cDet.useModel && cost < 0.0) {
		if (verData->get_model_by_name(analyticParallel))
			cost = (verData->get_model_by_name(analyticParallel))->get_back();
	}	
	
	verData->del();
	delete verData;
	
	return cost;
}

double getPerformance(int vid, graph &g, graph &baseGraph, 
                      map<vertex, vector<PartitionChoice> > partitions,
                      FuseGraph fg, vector<unsigned int> num_parts, 
                      vector<OperationInfo> &op_nodes,
                      bool strict,
                      compile_details_t &cDet,
                      build_details_t &bDet) {
	
		graph *toOptimize = new graph(baseGraph);
		int baseDepth = 0;
		check_depth(1,baseDepth, g.subgraphs);
		int r = buildVersion(toOptimize, partitions, fg, num_parts, 
                             op_nodes,vid, strict, bDet);
		if (r < 0) {
			//std::cout << "Build failed\n";
			delete toOptimize;
			return r;
		}
		
		int threadDepth = 0;
		check_depth(1,threadDepth, toOptimize->subgraphs);
		// assume all are for threads until something more sophisticated happens
		// eventually want to use pt.threadDepth, but that can wait
		threadDepth -= baseDepth;
		// if threadDepth is 2, only allow 1 thread and 1 cache block
		threadDepth = threadDepth >= 2 ? 1 : threadDepth;
	
		generate_code(cDet.tmpPath+cDet.fileName, vid, *toOptimize, "",
                      baseDepth, threadDepth, cDet.routine_name, 
                      cDet.noptr, *cDet.inputs, *cDet.outputs, 
                      *bDet.modelMessage, cDet.mpi, cDet.codetype);
	
		double cost = evaluateVersion(*toOptimize, vid, threadDepth,
                                      cDet, bDet);
		
		if (cost < 0.0) {
			std::cout << "Failed to get cost\n";
			cost = 1.0e99;
		}
	
		delete toOptimize;
		return cost;
}

/* }}} */

/* Search Strategies {{{ */

int random_search(int N, graph &g, graph &baseGraph, 
                  compile_details_t &cDet,
                  build_details_t &bDet) {
    
	OptSpace space(g);
	OptPoint pt(space.op_nodes.size());
	rng.seed(time(NULL));
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());

	int bestid = 1;
	double bestCost = 1e80;
	map<vertex, vector<PartitionChoice> > partitions;
	for (int vid = 1; vid <= N; ++vid) {
		for (int i = space.search_dimension-1; i >= 0; --i) {
			ParamBound pb = space.bounds(pt,i);
			int num_pts = (pb.high - pb.low)/pb.stride; // number of points in rand
			boost::uniform_smallint<> randrange(pb.low, pb.low + num_pts);
			boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > generator(rng, randrange);
			pt.set(i,generator());
		}
		partitions = space.mapPartition(&pt);
		double cost = getPerformance(vid, g, baseGraph, partitions, 
                                     pt.fsgr,  pt.num_parts, 
                                     space.op_nodes, false, cDet, bDet);
		if (cost > 0) {
			datalog << vid << "," << time(0) << ",";
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				datalog << pt.get(z) << ",";
			}
			datalog << cost << endl;
		}
		if (cost < 0 || cost > 1e10) {
			--vid;
		}
		if (cost > 0 && cost < bestCost) {
			bestid = vid;
			bestCost = cost;
		}
	}
	datalog.close();
	return bestid;
}

int orthogonal_search(graph &g, graph &baseGraph, 
                      compile_details_t &cDet,
                      build_details_t &bDet) {
	OptSpace space(g);
	OptPoint pt(space.op_nodes.size());
	OptPoint bestpoint = pt;
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());

	int bestid = 1;
	int vid = 1;
	map<vertex, vector<PartitionChoice> > partitions;

	//int arr[12] = {1,1,1,0,2,0,2,0,2,0,0,0};
	//for (int s=0; s < 12; ++s) {
	//	pt.set(s,arr[s]);
	//}


	partitions = space.mapPartition(&pt);
	double bestCost = getPerformance(vid, g, baseGraph, partitions, 
                                     pt.fsgr,  pt.num_parts, 
                                     space.op_nodes, false, cDet, bDet);
	datalog << vid << "," << time(0) << ",";
	for (unsigned int z=0; z < space.search_dimension; ++z) {
		datalog << pt.get(z) << ",";
	}
	datalog << bestCost << endl;


	//exit(0);
	++vid;
	for (unsigned int i = 0; i < space.search_dimension; ++i) {
		pt = bestpoint;
		ParamBound pb = space.bounds(pt, i);
		for (unsigned int k = pb.low; k <= pb.high; k += pb.stride) {
			if (pt.get(i) == k) {
				continue;
			}
			//cout << vid << ": setting pt[" << i << "] = " << k << endl;
			pt.set(i,k);
			partitions = space.mapPartition(&pt);
			double cost = getPerformance(vid, g, baseGraph, partitions, 
                                         pt.fsgr,  pt.num_parts,
                                         space.op_nodes, false, cDet, bDet);
			datalog << vid << "," << time(0) << ",";
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				datalog << pt.get(z) << ",";
			}
			datalog << cost << endl;
			if (cost > 0 && cost < bestCost) {
				bestid = vid;
				bestCost = cost;
				bestpoint = pt;
				cout << "new best = " << vid << " with cost " << cost << endl;
			}
			++vid;
		}
	}
	datalog.close();
	return bestid;
}

void exhaustive_work(int &vid, ofstream &datalog, double &bestCost, 
                     int &bestid, OptPoint &bestPoint, OptSpace &space,
                     int minBound, int maxBound,
                     graph &g, graph &baseGraph, 
                     compile_details_t &cDet,
                     build_details_t &bDet) {
    
	cout << "lo " << minBound << ", hi " << maxBound << endl;
	int i = minBound;
	OptPoint pt = bestPoint;
	while (i < maxBound) {
		ParamBound bnd = space.bounds(pt, i);
		if (pt.get(i) < bnd.high) {
			pt.orthoSet(i,min(bnd.high, pt.get(i)+bnd.stride));
			++vid;
			map<vertex, vector<PartitionChoice> > partitions;
			partitions = space.mapPartition(&pt);
			i = minBound;
			double cost = getPerformance(vid, g, baseGraph, partitions, 
                                         pt.fsgr,  pt.num_parts,
                                         space.op_nodes, false, cDet, bDet);
			if (cost > 1e5 || cost < 0) {
				--vid;
			} else {
				datalog << vid << "," << time(0) << ",";
				for (unsigned int z=0; z < space.search_dimension; ++z) {
					datalog << pt.get(z) << ",";
				}
				datalog << cost << endl;
				if (cost < bestCost) {
					cout << "new best = " << vid << " with cost " << cost << endl;
					bestid = vid;
					bestCost = cost;
					bestPoint = pt;
				}
			}
		} else {
			pt.orthoSet(i,bnd.low);
			++i;
		}
	}
}

int exhaustive_search(graph &g, graph &baseGraph, 
                      compile_details_t &cDet,
                      build_details_t &bDet) {
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());
	OptSpace space(g);
	//cout << "op_nodes:" << space.op_nodes.size() << endl;
	//vector<OperationInfo>::iterator o;
	//for (o = space.op_nodes.begin(); o != space.op_nodes.end(); ++o) {
		//cout << o->id << ",";
	//}
	//cout << endl;
	OptPoint pt(space.op_nodes.size());

	unsigned int i=0;
	int vid = 1;
	int bestid = 1;
	double bestCost = 1e80;

	/* first n*(n-1)/2 versions are fusions
	 * next 2*MAX_FUSE*num_nodes are partition info
	 * final gives num_parts 
	 * Starting from a particular point
	 */
	//int arr[63] = {3,3,3,3,2,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
				   //1,2,1,2,1,2,1,2,0,2,0,2,0,2,2,2,0,2,
				   //1,  1,  1,  1,  1,  1,  1,  1,  0};
				//// 4   5   9  10  14  15  17  19   20
	//for (int s=0; s < 63; ++s) {
		//pt.set(s,arr[s]);
	//}

	
	map<vertex, vector<PartitionChoice> > partitions;
	partitions = space.mapPartition(&pt);

	// I don't think this check should be necessary
	//if (space.checkPoint(&pt)) {
		//cout << "pt checks" << endl;
	bestCost = getPerformance(vid, g, baseGraph, partitions, pt.fsgr, 
                              pt.num_parts, space.op_nodes, false, 
                              cDet, bDet);
	if (bestCost < 1e5) {
		//pt.print(space.op_nodes);
		datalog << vid << "," << time(0) << ",";
		for (unsigned int z=0; z < space.search_dimension; ++z) {
			datalog << pt.get(z) << ",";
		}
		datalog << bestCost << endl;
	}
	//}

	//exit(0);
	int fuseP = pt.num_nodes * (pt.num_nodes - 1) / 2;
	
	while (i < space.search_dimension) {
		ParamBound bnd = space.bounds(pt, i);
		if (pt.get(i) < bnd.high) {
			pt.set(i,pt.get(i)+bnd.stride);
			++vid;
			cout << vid << ",";
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				cout << pt.get(z) << ",";
			}
			
			map<vertex, vector<PartitionChoice> > partitions;
			partitions = space.mapPartition(&pt);
			i = 0;
			//if (!space.checkPoint(&pt)) {
				////cout << "failed checkPoint" << endl;
				//--vid;
				//continue; //this throws out illegal versions
			//}
			// change
			double cost = getPerformance(vid, g, baseGraph, partitions, 
                                         pt.fsgr,  pt.num_parts,
                                         space.op_nodes, true, cDet, bDet);
			cout << cost << endl;
			if (cost > 1e5 || cost < 0) {
				if (cost == -1) {
					// don't mess w/ fusions on a partition fail
					i = fuseP;
				}
				--vid;
			} else {
				//pt.print(space.op_nodes);
				datalog << vid << "," << time(0) << ",";
				for (unsigned int z=0; z < space.search_dimension; ++z) {
					datalog << pt.get(z) << ",";
				}
				datalog << cost << endl;
				if (cost < bestCost) {
					cout << "new best = " << vid << " with cost " << cost << endl;
					bestid = vid;
					bestCost = cost;
				}
			}
		} else {
			pt.set(i,bnd.low);
			++i;
		}
		//cout << partFail << " " << fuseFail << " " << otherFail << endl;
	}
	datalog.close();
	return bestid;
}


int smart_hybrid_search(graph &g, graph &baseGraph, 
                        compile_details_t &cDet,
                        build_details_t &bDet) {

	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());
	OptSpace space(g);
	OptPoint pt(space.op_nodes.size());
	OptPoint bestpt = pt;
	int fuseP = pt.num_nodes * (pt.num_nodes - 1) / 2;
	
	int vid = 0;
	int bestid = 0;
	double bestCost = 1e80;

	vector<vector<int> > opts;

	get_fusion_options(g, opts, bDet);

	 cout << "num fusions back:" << opts.size() << endl;

	// phase 1: find best fusion

	for (unsigned int i = 0; i < opts.size(); ++i) {
		++vid;
		for (unsigned int z = 0; z < opts[i].size(); ++z) {
			pt.set(z, opts[i][z]);
		}
		map<vertex, vector<PartitionChoice> > partitions;
		partitions = space.mapPartition(&pt);
		double cost = getPerformance(vid, g, baseGraph, partitions, 
                                     pt.fsgr,  pt.num_parts,
                                     space.op_nodes, false, cDet, bDet);
		cout << cost << endl;
		if (cost > 1e5 || cost < 0) {
			--vid;
		} else {
			//pt.print(space.op_nodes);
			datalog << vid << "," << time(0) << ",";
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				datalog << pt.get(z) << ",";
			}
			datalog << cost << endl;
			if (cost < bestCost) {
				cout << "new best = " << vid << " with cost " << cost << endl;
				bestid = vid;
				bestCost = cost;
				bestpt = pt;
			}
		}
	}
	
	
	/*
	int arr[63] = {2,0,2,2,0,0,0,0,2,2,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,0,0,0,0,0,0,0,0};
	int arr[63] = {2,2,2,2,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,
		0,  0,  0,  0,  0,  0,  0,  0,  0};
	// 4   5   9  10  14  15  17  19   20
	for (int s=0; s < 63; ++s) {
		bestpt.set(s,arr[s]);
	}*/
        
	// phase 2: same as before
	exhaustive_work(vid, datalog, bestCost, bestid, bestpt, space,
                    fuseP, space.search_dimension, g, baseGraph,
                    cDet, bDet);
	
	datalog.close();
	return bestid;
}


int smart_exhaustive(graph &g, graph &baseGraph, 
					 compile_details_t &cDet,
                     build_details_t &bDet) {
	// Basic outline:
	// turn all parts on, and see what ways we can turn on partitions 1 @ a time.
	// for each permutation, find all legal fusions & test them.

	// it's starting to bug me that we have so many functions w/ the exact same code.
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());
	OptSpace space(g);
	OptPoint pt(space.op_nodes.size());
	OptPoint bestpt = pt;
	int fuseP = pt.num_nodes * (pt.num_nodes - 1) / 2;
	
	int vid = 0;
	int bestid = 1;
	double bestCost = 1e80;

	vector< vector<bool> > legalparts;
	map<vertex, vector<PartitionChoice> > partitions;
	partitions = space.mapPartition(&pt);

	OptPoint bestPoint = pt;
	if (MAX_PARTS > 0) {
		// First we build a table of which Ways are legal
		for (unsigned int i = 0; i < pt.num_nodes ; ++i) {
			pt.num_parts[i] = 1; // should only need 1?
			vector<bool> legals;
			for (int j = 0 ; j <= 2; ++j) { // m,n,k
				pt.partitions[i][0].way = (Way)j; //only setting 1st partition
				partitions = space.mapPartition(&pt);
				graph *toOptimize = new graph(baseGraph);
				bool b = buildPartition(toOptimize, partitions, bDet);
				legals.push_back(b);
				delete toOptimize;
				//cout << "node " << i << " way " << j << ": " << b << endl;
			}
			pt.num_parts[i] = 0;
			pt.partitions[i][0].way = m;
			legalparts.push_back(legals);
		}
	}

	unsigned int i = fuseP;
	//now we start our exhaustive
	while (true) { 
		bool legal_part = true;
		for (unsigned int p = 0; p < pt.num_nodes; ++p) {
			for (unsigned int q = 0; q < static_cast<unsigned int>(MAX_PARTS); ++q) {
				if (pt.num_parts[p] > q && !legalparts[p][pt.partitions[p][q].way]) {
					legal_part = false;
					break;
				}
			}
		}
		if (legal_part) {
			map<vertex, vector<PartitionChoice> > partitions;
			partitions = space.mapPartition(&pt);
			graph g2 = baseGraph; // making a copy
            
			buildPartition(&g2, partitions, bDet);
            //std::ofstream out5("lower9.dot");
            //print_graph(out5, g2);
            //out5.close();
			vector<vector<int> > opts;
			initial_lower(g2, *bDet.algos, *bDet.rewrites);
			get_fusion_options(g2, opts, bDet);

			// find best fusion
			for (unsigned int j = 0; j < opts.size(); ++j) {
				++vid;
				for (unsigned int z = 0; z < opts[j].size(); ++z) {
					pt.set(z, opts[j][z]);
				}
				vector<vector<set<int> > > fuselevels;
				makeFuseSetsFromGraph(pt.fsgr, space.max_fuse, space.op_nodes, fuselevels);
				//cout << "fuse sets for " << vid << endl;
				for (unsigned int v = 0; v != fuselevels.size(); ++v) {
					for (vector<set<int> >::iterator s = fuselevels[v].begin(); 
							s != fuselevels[v].end(); ++s) {
						//printSet(*s);
						for (set<int>::iterator i = s->begin(); i != s->end(); ++i) {
							for (set<int>::iterator j = s->begin(); j != s->end(); ++j) {
								if (*i < *j) {
									pair<FuseEdge,bool> e = edge(*i, *j, pt.fsgr);
									pt.fsgr[e.first] = max(v+1,pt.fsgr[e.first]);
									//cout << " value for " << *i << "," << *j << "set to " << pt.fsgr[e.first] << endl;
								}
							}
						}
					}
					//cout << endl;
				}
				map<vertex, vector<PartitionChoice> > partitions;
				partitions = space.mapPartition(&pt);
                
                //datalog << vid << "," << time(0) << ",";
				cout << vid << "," << time(0) << ",";
				for (unsigned int z=0; z < space.search_dimension; ++z) {
					//datalog << pt.get(z) << ",";
					cout << pt.get(z) << ",";
				}
                cout << "\n";
				double cost = getPerformance(vid, g, baseGraph, partitions, 
                                             pt.fsgr,  pt.num_parts, 
                                             space.op_nodes, false, 
                                             cDet, bDet);
				datalog << vid << "," << time(0) << ",";
				//cout << vid << "," << time(0) << ",";
				for (unsigned int z=0; z < space.search_dimension; ++z) {
					datalog << pt.get(z) << ",";
					//cout << pt.get(z) << ",";
				}
				datalog << cost << endl;
				//cout << cost << endl;
				if (cost > 1e5 || cost < 0) {
					//--vid;
				} else {
					if (cost < bestCost) {
						//cout << "new best = " << vid << " with cost " << cost << endl;
						bestid = vid;
						bestCost = cost;
						bestpt = pt;
					}
				}
			}
		}
		ParamBound bnd = space.bounds(pt, i);
		while (pt.get(i) >= bnd.high) {
			pt.set(i,bnd.low);
			++i;
			bnd = space.bounds(pt, i);
			if (i >= pt.size()) {
				return bestid;
			}
		}
		pt.set(i,min(bnd.high, pt.get(i)+bnd.stride));
		i = fuseP;
	}
	return bestid;
}

int debug_search(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
                 compile_details_t &cDet,
                 build_details_t &bDet) {

	OptSpace space(g);
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());
	OptPoint pt(space.op_nodes.size());
	int bestid = 1;
	double bestCost = 1e80;
	OptPoint bestPoint = pt;

	/*set<vertex> nodes;
	//nodes.insert(4);
	nodes.insert(5);
	nodes.insert(7);
	//nodes.insert(9);
	cout << "querying fuse set" << endl;
	vector<partitionChoices_t> choices = queryFuseSet(g, part_forest, nodes, 1);
	cout << "query succeeded" << endl;
	vector<partitionChoices_t>::iterator ii;
	for (ii = choices.begin(); ii != choices.end(); ++ii) {
		for (int jj = 0; jj != ii->branch_paths.size(); ++jj) {
			if (ii->iterators[jj] != NULL) {
				cout << "restriction: way of node " << jj << ": " 
					<< ii->branch_paths[jj] << endl;
			}
		}
	}*/

	vector<vector<int> > versions;
	ifstream inputfile("debug.txt");
	string line;
	while (getline(inputfile,line)) {
		stringstream s(line);
		vector<int> v;
		string k;
		while (getline(s,k,',')) {
			v.push_back(atoi(k.c_str()));
		}
		versions.push_back(v);
	}

	for (unsigned int vid = 0; vid < versions.size(); ++vid) {
		for (unsigned int i = 0; i < versions[vid].size(); ++i) {
			pt.set(i,versions[vid][i]);
		}
        //pt.print(space.op_nodes);
        //cout << space.op_nodes.size() << "<<<<<<<\n";
		map<vertex, vector<PartitionChoice> > partitions;
		partitions = space.mapPartition(&pt);
		double cost = getPerformance(vid, g, baseGraph, partitions, 
                                     pt.fsgr,  pt.num_parts, 
                                     space.op_nodes, false, cDet, bDet);
		datalog << vid << "," << time(0) << ",";
		//cout << vid << "," << time(0) << ",";
		for (unsigned int z=0; z < space.search_dimension; ++z) {
			datalog << pt.get(z) << ",";
			//cout << pt.get(z) << ",";
		}
		datalog << cost << endl;
		//cout << cost << endl;
		if (cost < bestCost) {
			//cout << "new best = " << vid << " with cost " << cost << endl;
			bestid = vid;
			bestCost = cost;
			bestPoint = pt;
		}
	}
	return bestid;
}


void thread_ortho(vector<int> &v, OptPoint &pt, OptSpace &space, 
                       graph &baseGraph, graph &g, compile_details_t &cDet,
                       build_details_t &bDet, ofstream &datalog,
                       int &vid) {
    
    // vector containing indexes of thread count
    // NOTE: finding these assuming they are the only value in v of 12
    vector<int> locations;
    for (unsigned int i = 0; i < v.size(); ++i)
        if (v[i] == 12)
            locations.push_back(i);
    
    // set up point initially
    for (unsigned int i = 0; i < v.size(); ++i) {
        pt.set(i,v[i]);
    }

    // initially set all thread values to 12
    for (unsigned int j = 0; j < locations.size(); ++j) {
        pt.set(locations[j],12);
    }
    
    for (unsigned int j = 0; j < locations.size(); ++j) {
        int bestI = 2;
        double bestCost = __DBL_MAX__;
        for (int i = 2; i <= 24; i += 2) {
                
            pt.set(locations[j],i);
            
            map<vertex, vector<PartitionChoice> > partitions;
            partitions = space.mapPartition(&pt);
            double cost = getPerformance(vid, g, baseGraph, partitions, 
                                         pt.fsgr,  pt.num_parts, 
                                         space.op_nodes, false, cDet, bDet);
            datalog << vid << "," << time(0) << ",";
            
            for (unsigned int z=0; z < space.search_dimension; ++z) {
                datalog << pt.get(z) << ",";
            }
            datalog << cost << endl;
            ++vid;
            if (cost > 0 && cost < bestCost) {
                bestCost = cost;
                bestI = i;
            }
        }
        
        pt.set(locations[j],bestI);
    }
}

void exhaustive_global(vector<int> &v, OptPoint &pt, OptSpace &space, 
                  graph &baseGraph, graph &g, compile_details_t &cDet,
                  build_details_t &bDet, ofstream &datalog,
                  int &vid) {
    
    // vector containing indexes of thread count
    // NOTE: finding these assuming they are the only value in v of 12
    vector<int> locations;
    for (unsigned int i = 0; i < v.size(); ++i)
        if (v[i] == 12)
            locations.push_back(i);
    
    // set up point initially
    for (unsigned int i = 0; i < v.size(); ++i) {
        pt.set(i,v[i]);
    }
    
    for (int i = 2; i <= 24; i += 2) {
        
        // set all thread values to i (2..24 @ 2)
        for (unsigned int j = 0; j < locations.size(); ++j) {
            pt.set(locations[j],i);
        }
        
        map<vertex, vector<PartitionChoice> > partitions;
        partitions = space.mapPartition(&pt);
        double cost = getPerformance(vid, g, baseGraph, partitions, 
                                     pt.fsgr,  pt.num_parts, 
                                     space.op_nodes, false, cDet, bDet);
        datalog << vid << "," << time(0) << ",";
        
        for (unsigned int z=0; z < space.search_dimension; ++z) {
            datalog << pt.get(z) << ",";
        }
        datalog << cost << endl;
        ++vid;
    }
}

int thread_search(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
                 compile_details_t &cDet,
                 build_details_t &bDet) {
    
	OptSpace space(g);
	ofstream datalog((cDet.tmpPath+"datalog.csv").c_str());
	OptPoint pt(space.op_nodes.size());
    
	vector<vector<int> > versions;
	ifstream inputfile("debug.txt");
	string line;
	while (getline(inputfile,line)) {
		stringstream s(line);
		vector<int> v;
		string k;
		while (getline(s,k,',')) {
			v.push_back(atoi(k.c_str()));
		}
		versions.push_back(v);
	}
    int vid = 1;
	for (auto j : versions) {
        thread_ortho(j, pt, space, baseGraph, g, cDet, bDet, datalog, vid);
	}
    
	return 0;
    
}

/* }}} */
