//
//  optimize_analysis.cpp
//  btoblas
//
#include "optimize_analysis.hpp"
#include <stdint.h>

// global partition identifier specific to trees
int treePartID;

using namespace std;

type *partition_row(type *baseT, string partitionSize) {
    // given a <O*<R<O* where R is the outermost Row container
    // return a copy of baseT with new row partitioning
    // newR<O*<partitionedR<O*
    // return NULL if not possible.
    
    type *highestRow = baseT->get_highest_row();
    if (highestRow->k == scalar)
        return NULL;
    
    type *newT = new type(highestRow->k, highestRow->dim, 
                          highestRow->s, (baseT->height)+1,
                          highestRow->uplo, highestRow->diag);
    
    newT->t = new type(*baseT);
    
    newT->dim.step = partitionSize;
    newT->t->get_highest_row()->dim.dim = partitionSize;
    
    return newT;
}

type *partition_column(type *baseT, string partitionSize) {
    // given a <O*<C<O* where C is the outermost Column container
    // return a copy of baseT with new row partitioning
    // newC<O*<partitionedC<O*
    // return NULL if not possible.
    
    type *highestCol = baseT->get_highest_column();
    if (highestCol->k == scalar)
        return NULL;
    
    type *newT = new type(highestCol->k, highestCol->dim, 
                          highestCol->s, (baseT->height)+1,
                          highestCol->uplo, highestCol->diag);
    
    newT->t = new type(*baseT);
    
    newT->dim.step = partitionSize;
    newT->t->get_highest_column()->dim.dim = partitionSize;
    
    return newT;
}

partitionBranch_t *makeBranch(int level, graph &g, int maxDepth, 
		partitionOp_t *my_type, op_code op, vertex leftv, vertex rightv, 
		vertex resultv) {
	if (level > maxDepth) {
		return NULL;
	}
	partitionBranch_t *t = new partitionBranch_t(level);
    
    string partitionSize = "$$" + boost::lexical_cast<string>(treePartID);
    
    // must special case scales of matrices and vectors
    bool scale = false;
    if (op == multiply) {
        if (my_type->left->k == scalar) {
            if (my_type->right->k != scalar && 
                my_type->result->k != scalar)
                scale = true;
        }
        else if (my_type->right->k == scalar) {
            if (my_type->left->k != scalar && 
                my_type->result->k != scalar)
                scale = true;
        }
    }
    
	if (op == add || op == subtract) {
		// case m
        // add columns all around
        type *left = partition_column(my_type->left,partitionSize); 
		type *right = partition_column(my_type->right,partitionSize); 
		type *result = partition_column(my_type->result,partitionSize);
        
        if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
            // if anything comes back NULL, this case is not an option
            // clean up
            if (left)
                delete left;
            if (right)
                delete right;
            if (result)
                delete result;
            
            t->m = NULL;
            t->nextM = NULL;
        }
        else {
            // build rest of structure and recurse
            treePartID++;
            t->m = new partitionOp_t(left,right,result);
            t->nextM = makeBranch(level+1, g, maxDepth, t->m, op,
                                  leftv, rightv, resultv);
        }
        
        
		// case n
        // add rows all around
        left = partition_row(my_type->left,partitionSize); 
		right = partition_row(my_type->right,partitionSize); 
		result = partition_row(my_type->result,partitionSize);
        
        if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
            // if anything comes back NULL, this case is not an option
            // clean up
            if (left)
                delete left;
            if (right)
                delete right;
            if (result)
                delete result;
            
            t->n = NULL;
            t->nextN = NULL;
        }
        else {
            // build rest of structure and recurse
            treePartID++;
            t->n = new partitionOp_t(left,right,result);
            t->nextN = makeBranch(level+1, g, maxDepth, t->n, op,
                                  leftv, rightv, resultv);
        }
        
        
		// case k
        // not possible for add
		t->k = NULL; 
        t->nextK = NULL;
        
	} else if (scale) {
        // handle vector or matrix scale.
        if (my_type->left->k == scalar) {
            // case m
            // add column to left and result, right remains same
            type *left = new type(*my_type->left); 
            type *right = partition_column(my_type->right,partitionSize);
            type *result = partition_column(my_type->result,partitionSize);
            
            if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
                // if anything comes back NULL, this case is not an option
                // clean up
                if (left)
                    delete left;
                if (right)
                    delete right;
                if (result)
                    delete result;
                
                t->m = NULL;
                t->nextM = NULL;
            }
            else {
                // build rest of structure and recurse
                treePartID++;
                t->m = new partitionOp_t(left,right,result);
                t->nextM = makeBranch(level+1, g, maxDepth, t->m, op,
                                      leftv, rightv, resultv);
            }
            
            
            // case n
            // add row to right and result, left stays the same
            left = new type(*my_type->left); 
            right = partition_row(my_type->right,partitionSize); 
            result = partition_row(my_type->result,partitionSize);
            
            if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
                // if anything comes back NULL, this case is not an option
                // clean up
                if (left)
                    delete left;
                if (right)
                    delete right;
                if (result)
                    delete result;
                
                t->n = NULL;
                t->nextN = NULL;
            }
            else {
                // build rest of structure and recurse
                treePartID++;
                t->n = new partitionOp_t(left,right,result);
                t->nextN = makeBranch(level+1, g, maxDepth, t->n, op,
                                      leftv, rightv, resultv);
            }
        }
        else if (my_type->right->k == scalar) {
            // case m
            // add column to left and result, right remains same
            type *left = partition_column(my_type->left,partitionSize); 
            type *right = new type(*my_type->right); 
            type *result = partition_column(my_type->result,partitionSize);
            
            if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
                // if anything comes back NULL, this case is not an option
                // clean up
                if (left)
                    delete left;
                if (right)
                    delete right;
                if (result)
                    delete result;
                
                t->m = NULL;
                t->nextM = NULL;
            }
            else {
                // build rest of structure and recurse
                treePartID++;
                t->m = new partitionOp_t(left,right,result);
                t->nextM = makeBranch(level+1, g, maxDepth, t->m, op,
                                      leftv, rightv, resultv);
            }
            
            
            // case n
            // add row to right and result, left stays the same
            left =  partition_row(my_type->left,partitionSize);
            right = new type(*my_type->right); 
            result = partition_row(my_type->result,partitionSize);
            
            if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
                // if anything comes back NULL, this case is not an option
                // clean up
                if (left)
                    delete left;
                if (right)
                    delete right;
                if (result)
                    delete result;
                
                t->n = NULL;
                t->nextN = NULL;
            }
            else {
                // build rest of structure and recurse
                treePartID++;
                t->n = new partitionOp_t(left,right,result);
                t->nextN = makeBranch(level+1, g, maxDepth, t->n, op,
                                      leftv, rightv, resultv);
            }
        }
        
        // case k
        // not possible for scale
		t->k = NULL; 
        t->nextK = NULL;
        
    } else  { // multiply, non scale
		// case m
        // add column to left and result, right remains same
        type *left = partition_column(my_type->left,partitionSize); 
		type *right = new type(*my_type->right); 
		type *result = partition_column(my_type->result,partitionSize);
        
        if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
            // if anything comes back NULL, this case is not an option
            // clean up
            if (left)
                delete left;
            if (right)
                delete right;
            if (result)
                delete result;
            
            t->m = NULL;
            t->nextM = NULL;
        }
        else {
            // build rest of structure and recurse
            treePartID++;
            t->m = new partitionOp_t(left,right,result);
            t->nextM = makeBranch(level+1, g, maxDepth, t->m, op,
                                  leftv, rightv, resultv);
        }
        
        
		// case n
        // add row to right and result, left stays the same
        left = new type(*my_type->left); 
		right = partition_row(my_type->right,partitionSize); 
		result = partition_row(my_type->result,partitionSize);
        
        if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
            // if anything comes back NULL, this case is not an option
            // clean up
            if (left)
                delete left;
            if (right)
                delete right;
            if (result)
                delete result;
            
            t->n = NULL;
            t->nextN = NULL;
        }
        else {
            // build rest of structure and recurse
            treePartID++;
            t->n = new partitionOp_t(left,right,result);
            t->nextN = makeBranch(level+1, g, maxDepth, t->n, op,
                                  leftv, rightv, resultv);
        }
        
        
		// case k
        // add row to left, column to right, result stays the same
		left = partition_row(my_type->left,partitionSize); 
		right = partition_column(my_type->right,partitionSize); 
		result = new type(*my_type->result);
        
        if (!((uintptr_t)left & (uintptr_t)right & (uintptr_t)result)) {
            // if anything comes back NULL, this case is not an option
            // clean up
            if (left)
                delete left;
            if (right)
                delete right;
            if (result)
                delete result;
            
            t->k = NULL;
            t->nextK = NULL;
        }
        else {
            // build rest of structure and recurse
            treePartID++;
            t->k = new partitionOp_t(left,right,result);
            t->nextK = makeBranch(level+1, g, maxDepth, t->k, op,
                                  leftv, rightv, resultv);
        }
	}

	return t;
}


partitionTree_t *initializeTree(graph &g, vertex v, int maxDepth) {
	//cout << "entering initializeTree v = " << v << endl;
    
    // recursively build the tree for current vertex if this vertex
    // is an operation.
    
	op_code op = g.info(v).op;
	if (op == add || op == subtract || op == multiply) {
        // new tree structure
		struct partitionTree_t *t = new partitionTree_t(g.inv_adj(v)[0],
                                        g.inv_adj(v)[1],v,maxDepth, op);

        // store base types
		t->baseTypes = new partitionOp_t(new type(g.info(t->left).t),
                                         new type(g.info(t->right).t),
                                         new type(g.info(v).t));
        // begin building partitions
		t->partitions = makeBranch(1, g, maxDepth, t->baseTypes, op,
                                   g.inv_adj(v)[0],g.inv_adj(v)[1],
                                   v);
        
        // perform tranpose when necessary??????
        // bool transLeft = false;
        // bool transRight = false;
        if (g.info(g.inv_adj(v)[0]).op == trans) {
            t->left = g.inv_adj(g.inv_adj(v)[0])[0];
            // transLeft = true;
        }
        if (g.info(g.inv_adj(v)[1]).op == trans) {
            t->right = g.inv_adj(g.inv_adj(v)[1])[0];
            // transRight = true;
        }

		return t;

	} else {
		return nullptr;
	}
}


void buildForest(vector<partitionTree_t*> &forest, graph &g, int maxDepth) {
    // build a tree for each operation vertex in the graph.  Forest
    // is the size of num_vertices and non-operation vertices will
    // have NULL
    treePartID = 0;
    for (unsigned int i = 0; i < g.num_vertices(); ++i) {
		partitionTree_t *t = initializeTree(g, i, maxDepth);
		forest.push_back(t);
	}
}

void branchesAtDepthWork(vector< pair< vector<Way>, partitionOp_t *> > &vec,
		partitionBranch_t *t, int d, vector<Way> curpath) {
	if (t == NULL) {
		return;
	}

	vector<Way> mp = curpath; mp.push_back(m);
	vector<Way> np = curpath; np.push_back(n);
	vector<Way> kp = curpath; kp.push_back(k);

	if (t->partitionLevel < (d-1)) {
		branchesAtDepthWork(vec, t->nextM, d, mp);
		branchesAtDepthWork(vec, t->nextN, d, np);
		branchesAtDepthWork(vec, t->nextK, d, kp);
	} else {
		vec.push_back(pair<vector<Way>, partitionOp_t*>(mp, t->m));
		vec.push_back(pair<vector<Way>, partitionOp_t*>(np, t->n));
		vec.push_back(pair<vector<Way>, partitionOp_t*>(kp, t->k));
	}
}

void branchesAtDepth(vector< pair< vector<Way>, partitionOp_t *> > &vec,
		partitionBranch_t *t, int d) {
	vector<Way> path;
	//cout << "branchesAtDepth: started" << endl;
	branchesAtDepthWork(vec, t, d, path);
	//cout << "branchesAtDepth: finished" << endl;
}

sg_iterator_t* get_iterator(type* left, type *right, type *result,
                  op_code op, bool &reduction) {
	reduction = false;
    // determine which lowering rule will fire for this operation and
    // therefore what the iteration of the loop will look like.
    //
    // outputs
    // reduction with true for reduction and false for none.
    // a newed sg_iterator_t with iteration details, or null for no loop
    
    vector<pair<type*, subgraph*> > parentType;
    
    if (op == add || op == subtract) {
        // the loop in this case can come from any of the types.
        // scalar's will have no loop
        if (left->k == scalar)
            return NULL;
        return new sg_iterator_t(*left, 0, NULL, NULL);
    }
    else if (op == multiply) {
        if (left->k == row) {
            if (right->k == row) {
                // row * row
                return new sg_iterator_t(*result, 0, NULL, NULL);
            }
            else if (right->k == column) {
                // row * column (dot)
				reduction = true;
                if (left->height > right->height)
                    return new sg_iterator_t(*left, 0, NULL, NULL);
                else
                    return new sg_iterator_t(*right, 0, NULL, NULL);
            }
            else {
                // row * scalar
                return new sg_iterator_t(*left, 0, NULL, NULL);
            }
        }
        else if (left->k == column) {
            if (right->k == row) {
                // column * row (outer)
                return new sg_iterator_t(*result, 0, NULL, NULL);
            }
            else if (right->k == column) {
                // column * column
                return new sg_iterator_t(*result, 0, NULL, NULL);
            }
            else {
                // col * scalar
                return new sg_iterator_t(*left, 0, NULL, NULL);
            }
        }
        else {
            if (right->k == scalar) {
                // scalar * scalar
                return NULL;
            }
            else {
                // scalar * container
                return new sg_iterator_t(*right, 0, NULL, NULL);
            }
        }
    }
    else {
        std::cout << "Error: optimize_analysis.cpp: get_iterator(); unhandled operation\n";
    }
    return NULL;
}

vector<vector<Way> > queryPair(partitionTree_t *t1, partitionTree_t *t2,
		int depth, int numVertices) {
    // determine the way that two trees need to be partitioned to
    // remain fusable to a given depth.
    // return is vector possible combinations of partitionings
    // for example if t1 and t2 represent vertices 3 and 4 and the
    // graph has 5 vertices the return could be
    // {  {-1,-1,-1,0,2},
    //    {-1,-1,-1,2,1}  }
    // meaning there are two partitionings that work, one where
    // 3=m, 4=k; and the other where 2=k, 4=m.
    // 
    // numVertices needs to be the number of vertices in the graph
    
	vector<pair<vector<Way>, partitionOp_t *> > t1_options;
	vector<pair<vector<Way>, partitionOp_t *> > t2_options;

	branchesAtDepth(t1_options, t1->partitions, depth);
	branchesAtDepth(t2_options, t2->partitions, depth);

	vector<vector<Way> > legalPartitions;

	vector<pair<vector<Way>, partitionOp_t *> >::iterator i;
	for (i = t1_options.begin(); i != t1_options.end(); ++i) {
        
        partitionOp_t *type1 = i->second;
        if (type1 == NULL) continue;
        
        // get iterator and reduction information
        bool t1Reduce = false;
        sg_iterator_t *t1Itr = get_iterator(type1->left, type1->right, 
                                            type1->result,t1->op,t1Reduce);
        
		vector<pair<vector<Way>, partitionOp_t *> >::iterator j;
		for (j = t2_options.begin(); j != t2_options.end(); ++j) {
			//cout << "checking " << i->first.back() << ", " << j->first.back() << endl;
			
			partitionOp_t *type2 = j->second;
			if (type2 == NULL) continue;
            
            // for each combination, determine the iterators and compare
            // to see if they are fusable.  also check pipeline dependencies
            // which may prevent the fusion.
            
            // get t2 iterator and reduction information
            bool t2Reduce = false;            
            sg_iterator_t *t2Itr = get_iterator(type2->left,type2->right, 
                                                type2->result,t2->op,
                                                t2Reduce);
            
            // can the two iterators fuse
            bool canFuse = t1Itr->fusable(*t2Itr, true);
            
            // clean up t2
            delete t2Itr;
            
            // if cannot fuse, continue
            if (!canFuse)
                continue;
            
            // if the loops can be fused, there still may be dependencies
            // which prevent fusion.  This is checking for a pipleline
            // dependency with a reduction.
            if (t1->left == t2->result || t1->right == t2->result) {
                // pipeline from t2 -> t1
                if (t2Reduce)
                    canFuse = false;
                    
            }
            else if (t1->result == t2->left || t1->result == t2->right) {
                // pipeline from t1 -> t2
                if (t1Reduce)
                    canFuse = false;
            }

			if (canFuse) {
				//cout << "can fuse" << endl;
				vector<Way> vw(numVertices);
				vw[t1->result] = i->first.back();
				vw[t2->result] = j->first.back();
				legalPartitions.push_back(vw);
			}
		}
        
        delete t1Itr;
	}

	return legalPartitions;
}

// choice_info has 3 vectors:
// the iterators, the branch paths, and the reductions
void print_choices(partitionChoices_t choice_info) {
	cout << "iterators: ";
	for (auto i : choice_info.iterators) {
		cout << i << " ";
	}
	cout << endl;
	cout << "branch paths: ";
	for (auto j : choice_info.branch_paths) {
		cout << j << " ";
	}
	cout << endl;
	cout << "reductions: ";
	for (auto k : choice_info.reductions) {
		cout << k << " ";
	}
	cout << endl;
}

vector<partitionChoices_t> tryAddFuseNode(graph g, 
                                          const partitionChoices_t &choice_info, 
                                          const partitionTree_t *tree, int depth) {
    
	// cout << "==== adding in " << tree->result << endl;
	// print_choices(choice_info);
	vector<partitionChoices_t> all_choices;
	if (tree == NULL) {
		cout << "ERROR! entering tryAddFuseNode, tree pointer = " << tree << endl;
		return all_choices;
	}
	vector<pair<vector<Way>, partitionOp_t *> > t_options;
	branchesAtDepth(t_options, tree->partitions, depth);

	// For each partition choice of the added node, we see if it will fuse into the given set
	vector<pair<vector<Way>, partitionOp_t *> >::iterator i;
	for (i = t_options.begin(); i != t_options.end(); ++i) {
        partitionOp_t *type1 = i->second; //type1 is a possible partition option for the added node
        if (type1 == NULL) continue;
		
        // get iterator and reduction information
        bool t1Reduce = false;
        sg_iterator_t *t1Itr = get_iterator(type1->left, type1->right, 
                                            type1->result,tree->op,t1Reduce);
		if (t1Itr == NULL) continue;
		//cout << "successfully found iterator" << endl;
        
		// if fusable() is transitive, we only need to check 
		// one of the iterators in the list
		unsigned int oi;
		bool canFuse;
		for (oi=0; oi != choice_info.iterators.size(); ++oi) {
			if (choice_info.iterators[oi] != NULL) {
				break;
			}
		}
		if (oi == choice_info.iterators.size()) {
			// no iterators yet
			canFuse = true;
			//cout << "first iteration" << endl;
		} else {
			//cout << "comparing to " << oi << endl;
			canFuse = t1Itr->fusable(*choice_info.iterators[oi],true);
			//cout << "success fusable: canfuse = " << canFuse << endl;
		}
		//
		// one of these should check the vector in choice_info
		// the other should check t1Reduce
		// if the introduced op is a pipeline, 
		// we need to get the out-edges of it from the graph
		// otherwise, we check each in edge (left & right) 
		// and make sure each one isn't a pipeline
		
		// First check if either argument to this operation is a pipeline
		if (choice_info.reductions[tree->left] || 
			choice_info.reductions[tree->right]) {
			//cout << "can't fuse, pipeline reduction" << endl;
			canFuse = false;
		}

		deque<vertex> order;
		vector<bool> visited(choice_info.iterators.size());
		if (t1Reduce) {
			topo_sort_r(tree->result, g, order, visited);
			//cout << "t1 reduce" << endl;
			deque<vertex>::iterator vrt_itr;
			for (vrt_itr = order.begin(); vrt_itr != order.end(); ++vrt_itr) {
				if (choice_info.iterators[*vrt_itr] != NULL) {
					//cout << "can't fuse, reduction blocking previous" << endl;
					canFuse = false;
				}
			}
		}

		if (canFuse) {
			//cout << "legal fuse in tryAddFuse" << endl;
			partitionChoices_t addedChoice = choice_info; // copy
			addedChoice.iterators[tree->result] = t1Itr;
			addedChoice.branch_paths[tree->result] = i->first.back();
			if (t1Reduce) {
				deque<vertex>::iterator vrt_itr;
				for (vrt_itr = order.begin(); vrt_itr != order.end(); ++vrt_itr) {
					addedChoice.reductions[*vrt_itr] = true;
				}
			}
			all_choices.push_back(addedChoice);
		}
	}
	return all_choices;
}

/*
 * each item in the vector represents a legal set of partition Choices
 * under the constraint that the nodes are all fused at depth
 */
vector<partitionChoices_t> queryFuseSet(graph &g, vector<partitionTree_t*> forest,  set<vertex> nodes, int depth) {
	//cout << "== Entering queryFuseSet ==" << endl;
	set<vertex>::iterator i;
	vector<partitionChoices_t> choices;
	partitionChoices_t stub(g.num_vertices());
	choices.push_back(stub);
	for (i = nodes.begin(); i != nodes.end(); ++i) {
		//cout << "tree " << *i << endl;
		vector<partitionChoices_t> new_choices;
		vector<partitionChoices_t>::iterator choice;
		for (choice = choices.begin(); choice != choices.end(); ++choice) {
			vector<partitionChoices_t> v = tryAddFuseNode(g, *choice, forest[*i], depth);
			//cout << "finished tryAddFuseNode" << endl;
			vector<partitionChoices_t>::iterator j;
			for (j = v.begin(); j != v.end(); ++j) {
				new_choices.push_back(*j);
			}
		}
		if (new_choices.empty()) {
			return new_choices;
		}
		choices = new_choices;
	}
	return choices;
}

void deleteForest(vector<partitionTree_t*> &forest) {
    for (unsigned int i = 0; i < forest.size(); ++i) {
        if (forest[i]) delete (forest[i]);
    }
}

int printBranch(ostream &out, partitionBranch_t *branch, int n) {
	if (branch == NULL) {
		return n;
	}
	out << n << "[label=\""; 
	if (branch->m != NULL) {
		out << "m: " << type_to_string(*branch->m->left) << "    "
					<< type_to_string(*branch->m->right) << "    "
                    //<< branch->m->result->dim.dim << "\t" << 
                    //    branch->m->result->dim.step << "   "
					<< type_to_string(*branch->m->result) << "\\n";
	}
	if (branch->n != NULL) {
		out << "n: " << type_to_string(*branch->n->left) << "    "
					<< type_to_string(*branch->n->right) << "    "
        //<< branch->n->result->dim.dim << "\t" << 
        //branch->n->result->dim.step << "   "
					<< type_to_string(*branch->n->result) << "\\n";
	}
	if (branch->k != NULL) {
		out << "k: " << type_to_string(*branch->k->left) << "    "
					<< type_to_string(*branch->k->right) << "    "
        //<< branch->k->result->dim.dim << "\t" << 
        //branch->k->result->dim.step << "   "
					<< type_to_string(*branch->k->result) << "\\n";
	}
	out << " \"];" << endl;
	int n2 = n+1;
	if (branch->nextM != NULL) {
		out << n << " -> " << n2 << " [label = \"m\"];" << endl;
		n2 = printBranch(out, branch->nextM, n2);
	}
	if (branch->nextN != NULL) {
		out << n << " -> " << n2 << " [label = \"n\"];" << endl;
		n2 = printBranch(out, branch->nextN, n2);
	}
	if (branch->nextK != NULL) {
		out << n << " -> " << n2 << " [label = \"k\"];" << endl;
		n2 = printBranch(out, branch->nextK, n2);
	}
	return n2;
}

void printTree(std::ofstream &out, partitionTree_t *tree) {
	out << "digraph G {" << endl;
	printBranch(out, tree->partitions, 0);
	out << "}" << endl;
}

