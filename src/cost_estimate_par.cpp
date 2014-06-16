#include "cost_estimate_common.hpp"
#include "translate_to_code.hpp"
#include "translate_utils.hpp"
#include <fstream>
#include "modelInfo.hpp"
#include <list>

using std::list;

extern "C" {
#include "memmodel_par/tree_par.h"
#include "memmodel_par/memmodel_par.h"
#include "memmodel_par/cost_par.h"
#include "memmodel_par/parallel_machines.h"
}

#include "memmodel_par/build_machine.h"

//#define DEBUG

using namespace std;

char *string_to_charpAP(string s) {
	const char *t = s.c_str();
	char *ret = (char*) malloc(sizeof(char) * (strlen(t)+1));
	strcpy(ret, t);
	return ret;
}

char **vector_to_charppAP(std::vector<char> iters) {
	char **ret = (char**) malloc(sizeof(char*) * iters.size());
	for (int i = 0; i != iters.size(); i++) {
		ret[i] = (char*) malloc(sizeof(char) * 2);
		ret[i][0] = iters[i];
		ret[i][1] = NULL;
	}
	
	return ret;
}

long long glbl_itsAP;
long long get_iterationsAP(subgraph *sg, int nThreads) {
	if (sg->sg_iterator.conditions.begin()->right.find("$$") == 0)
		return glbl_itsAP/nThreads;

	return glbl_itsAP;
}

long long get_iterations_from_type(type *t) {
	if (t->dim.dim.find("__m") == 0 || t->dim.dim.find("__s") == 0) 
		return glbl_itsAP/2;
	
	return glbl_itsAP;
}

struct node *build_reduction_loops_r(graph &g, string &rTo, string &rFrom, type *t, int itrDepth) {
	
	
	if (t->k == scalar) {
		vector<char> iters;
		for (int i = 1; i < itrDepth; ++i)
			iters.push_back(iterators[i]);
		
		struct var **vars = (struct var**) malloc(sizeof(struct var*) * 2);
		// since these can both appear to in the tree as the same variable
		// set one of the summed flags
		// a[i] += a[thread*size+i]
		vars[0] = create_varP(string_to_charpAP(rTo), 
							  vector_to_charppAP(iters), itrDepth-1, 0);
		vars[1] = create_varP(string_to_charpAP(rFrom), 
							  vector_to_charppAP(iters), itrDepth-1,1);
		
		return create_stateP(vars, 2);
	}
	else {
		struct node **nodes = (struct node **) malloc(sizeof(struct node*));
		nodes[0] = build_reduction_loops_r(g, rTo, 
										   rFrom, t->t, itrDepth+1);
		long long iterations = get_iterations_from_type(t);
		char *iterate = (char*) malloc(sizeof(char) * 2);
		iterate[0] = iterators[itrDepth];
		iterate[1] = NULL;
		return create_loopP(iterations, nodes, 1, iterate);
	}
}

struct node *build_reduction_loops(graph &g, vertex rTo, vertex rFrom, int itrDepth, int nThreads) {	
	struct node **nodes = (struct node **) malloc(sizeof(struct node*));
	
	string to = g.info(rTo).label;
	if (to.compare("") == 0)
		to = "t" + boost::lexical_cast<string>(rTo);
	string from = g.info(rFrom).label;
	if (from.compare("") == 0)
		from = "t" + boost::lexical_cast<string>(rFrom);
	
	nodes[0] = build_reduction_loops_r(g, to, from, &g.info(rTo).t, itrDepth+1);
	long long iterations = nThreads;
	char *iterate = (char*) malloc(sizeof(char) * 2);
	iterate[0] = iterators[itrDepth];
	iterate[1] = NULL;
	return create_loopP(iterations, nodes, 1, iterate);
}

struct var* graph_to_tree_varAP(vertex v, int init_d, dir d, graph &g,
								int threadDepth, map<string, bool> &summedMap) {
	std::vector<char> iters;
	
	// new //
	vertex u;
	bool test = true;
	
	if (g.info(v).op == add || g.info(v).op == subtract || g.info(v).op == multiply) {
		test = false;
		u = v;
	}
	
	int curr = depth(g.find_parent(v));
	int last = init_d;

	while (test) {
		if (curr > last || g.info(v).op == add || g.info(v).op == subtract || g.info(v).op == multiply)
			break;	
		u = v;
		if (d == up) {
			if (g.inv_adj(v).size() == 0)
				break;
			last = curr;
			if (g.inv_adj(v).size() != 1) {
				std::cout << v << "\n";
				std::cout << "WARNING: cost_estimate_par.cpp: graph_to_tree_varAP: unexpected input 2\n"
						  << v << "\n";
			}
			v = g.inv_adj(v)[0];
			curr = depth(g.find_parent(v));

		}
		else if (d == down) {
			if (g.adj(v).size() == 0)
				break;
			last = curr;
			v = g.adj(v)[0];
			curr = depth(g.find_parent(v));
		}
		if (depth(g.find_parent(u)) != 0)
			iters.push_back(iterators[depth(g.find_parent(u))]);		
	}
	
	end:
	string lbl = g.info(u).label;
	if (lbl.compare("") == 0)
		lbl = "t" + boost::lexical_cast<string>(u);
	
	int summedFlag = 0;
	if (threadDepth > 0) {
		if (g.info(u).op == sumto || g.info(u).op == temporary || summedMap[lbl]) {
			// there are reductions that are reduced into and then consumed
			// whole.  This is between two threaded loops at the top level.
			if (d == up && g.find_parent(u) == NULL && g.info(u).op == sumto
				&& !g.info(u).t.beenPartitioned())
				summedFlag = 0;
			else
				summedFlag = 1;
			
			summedMap[lbl] = true;
		}
	}
	return create_varP(string_to_charpAP(lbl), vector_to_charppAP(iters), 
							iters.size(), summedFlag);
}

struct node* build_tree_rAP(subgraph *sg, graph &g, int currDepth, int threadDepth, 
							map<string, bool> &summedMap, int nThreads) {

	vector<subgraph*> order;
	order_subgraphs_wrapper(order, sg, g);
	
	vector<struct node *> v_nodes;
	for (int i = 0; i != order.size(); i++) {
		if (order[i]->sg_iterator.updates.begin()->right.compare("1") != 0 && currDepth <= threadDepth) {
			// thread level subgraph that the tree does not need.
			struct node *tmp = build_tree_rAP(order[i], g, currDepth+1, threadDepth, 
											  summedMap, nThreads);
			
			// actually want the children.
			for (int j = 0; j < tmp->numchildren; ++j) {
				v_nodes.push_back(tmp->children[j]);
			}
			
			delete_nodeP(tmp);
			
			// create reduction loops.
			for (vertex x = 0; x < g.num_vertices(); ++x) {
				if (g.find_parent(x) != NULL)
					continue;
				if (g.info(x).op != sumto)
					continue;
				
				// only interested in results from the current
				// subgraph so that the order of loops is correct
				subgraph *p = g.find_parent(g.inv_adj(x)[0]);
				bool relatedToCurr = false;
				while (p) {
					if (p == order[i]) {
						relatedToCurr = true;
						break;
					}
					p = p->parent;
				}
				if (!relatedToCurr)
					continue;
				
				vertex rTo, rFrom = x;
				if (g.find_parent(g.adj(x)[0]) == NULL && g.info(g.adj(x)[0]).op == output)
					rTo = g.adj(x)[0];
				else
					rTo = rFrom;
				
				v_nodes.push_back(build_reduction_loops(g,rTo,rFrom,1,nThreads));
			}
		}
		else {
			v_nodes.push_back(build_tree_rAP(order[i], g, currDepth+1, threadDepth, 
											 summedMap, nThreads));
		}
	}
	
	// create variables
	vector<struct var*> v_vars;
	for (int j = 0; j != sg->vertices.size(); j++) {
		switch (g.info(sg->vertices[j]).op) {
			case trans: {
				std::cout << "?? cost_estimate_par.cpp: build_tree_rAP(): transpose?\n";
				break;
			}
			case add:
			case subtract:
			case multiply: {
				vertex u = sg->vertices[j];
				//left
				v_vars.push_back(graph_to_tree_varAP(g.inv_adj(u)[0],depth(g.find_parent(u)), up, g,
													 threadDepth, summedMap));
				//right
				v_vars.push_back(graph_to_tree_varAP(g.inv_adj(u)[1],depth(g.find_parent(u)), up, g,
													 threadDepth, summedMap));
				//result
				for (int i = 0; i != g.adj(u).size(); i++) {
					// if adj is an operation, then u needs to be the variable
					vertex a = g.adj(u)[i];
					if (g.info(a).op == add || g.info(a).op == subtract || g.info(a).op == multiply) {
						string lbl = g.info(u).label;
						if (lbl.compare("") == 0)
							lbl = "t" + boost::lexical_cast<string>(u);

						int summedFlag = 0;
						if (threadDepth > 0) {
							if (g.info(u).op == sumto || g.info(u).op == temporary || summedMap[lbl]) {
								summedFlag = 1;
								summedMap[lbl] = true;
							}
						}
						
						v_vars.push_back(create_varP(string_to_charpAP(lbl), NULL, 0, summedFlag));
						
						continue;
					}
					v_vars.push_back(graph_to_tree_varAP(g.adj(u)[i],depth(g.find_parent(u)), down, g,
														 threadDepth, summedMap));
				}
				
				break;
			}
			default: {
				break;
			}
		}
		
	}
	
	// create statement if needed
	if (v_vars.size() != 0) {
		struct var **vars = (struct var**) malloc(sizeof(struct var*) * v_vars.size());
		for (int i = 0; i != v_vars.size(); i++) {
			vars[i] = v_vars[i];
		}
		struct node *stmt = create_stateP(vars, v_vars.size());
		v_nodes.push_back(stmt);
	}
	
	// create loop
	char *iterate = (char*) malloc(sizeof(char) * 2);
	iterate[0] = iterators[depth(sg)];
	iterate[1] = NULL;
	
	struct node **nodes = (struct node **) malloc(sizeof(struct node*) * v_nodes.size());
	for (int i = 0; i != v_nodes.size(); i++) {
		nodes[i] = v_nodes[i];
	}

	long long iterations = get_iterationsAP(sg, nThreads);
	struct node *loop = create_loopP(iterations, nodes, v_nodes.size(), iterate);

	//return current node
	return loop;
}

struct node *build_treeAP(graph &g, int threadDepth, int nThreads) {
	// order subgraphs
	vector<subgraph*> order;
	order_subgraphs_wrapper(order, 0, g);

	// map names to summed flag
	map<string, bool> summedMap;
	for (vertex i = 0; i < g.num_vertices(); ++i) {
		if (g.find_parent(i) == NULL) {
			string lbl = g.info(i).label;
			if (lbl.compare("") == 0)
				lbl = "t"+boost::lexical_cast<string>(i);
			
			if (g.info(i).op == sumto) {
				summedMap[lbl] = true;
				vertex ad = g.adj(i)[0];
				if (g.info(ad).op == output)
					summedMap[g.info(ad).label] = true;
				continue;
			}
			
			if (g.info(i).t.beenPartitioned())
				summedMap[lbl] = true;
		}
	}
	
	vector<struct node*> v_trees;
	for (int i = 0; i != order.size(); i++) {
		if (order[i]->sg_iterator.updates.begin()->right.compare("1") != 0 && threadDepth > 0) {
			// thread level subgraph that the tree does not need.
			struct node *tmp = build_tree_rAP(order[i], g, 1, threadDepth, 
											  summedMap, nThreads);
			
			// actually want the children.
			for (int j = 0; j < tmp->numchildren; ++j) {
				v_trees.push_back(tmp->children[j]);
			}
			
			delete_nodeP(tmp);
			
			// create reduction loops.
			for (vertex x = 0; x < g.num_vertices(); ++x) {
				if (g.find_parent(x) != NULL)
					continue;
				if (g.info(x).op != sumto)
					continue;

				// only interested in results from the current
				// subgraph so that the order of loops is correct
				subgraph *p = g.find_parent(g.inv_adj(x)[0]);
				bool relatedToCurr = false;
				while (p) {
					if (p == order[i]) {
						relatedToCurr = true;
						break;
					}
					p = p->parent;
				}
				if (!relatedToCurr)
					continue;
				
				vertex rTo, rFrom = x;
				if (g.find_parent(g.adj(x)[0]) == NULL && g.info(g.adj(x)[0]).op == output)
					rTo = g.adj(x)[0];
				else
					rTo = rFrom;
				
				v_trees.push_back(build_reduction_loops(g,rTo,rFrom,1,nThreads));
				
			}
		}
		else {
			v_trees.push_back(build_tree_rAP(order[i], g, 1, threadDepth, 
											 summedMap, nThreads));
		}
	}
	
	struct node **trees = (struct node **) malloc(sizeof(struct node*) * v_trees.size());
	for (int i = 0; i != v_trees.size(); i++) {
		trees[i] = v_trees[i];
	}
	
	char *iterate = (char*) malloc(sizeof(char) * 2);
	iterate[0] = 'a';
	iterate[1] = NULL;
	
	struct node *forest = create_loopP(1, trees, v_trees.size(), iterate);
		
	return forest;	
}

void print_varAP(struct var *v, int &id, std::ofstream &out) {
	out << id << "[label=\"" << v->name << "; " << v->iterates << "\\n";
	if (v->iterates != 0)
		out << v->iterate[0];
	
	for (int i = 1; i < v->iterates; i++) {
		out << "," << v->iterate[i];
	}
	out << "\\n";
	out << v->summed;
	out << "\"];\n";
}

void print_nodeAP(struct node* n, int &id, std::ofstream &out) {
	out << id << "[label=\"";
	
	if (n->iterate == NULL) {
		out << "stmt" << "\\nvars: " << n->variables;
	}
	else {	
		out << "itr:" << n->iterate << "; iters:" << n->its << "\\n num_chld:"
		<< n->numchildren << "; vars:" << n->variables; 
	}
	out << "\"];\n";
	
	int me = id;
	for (int i = 0; i != n->variables; i++) {
		id++;
		out << me << " -> " << id << ";\n";
		print_varAP(n->vars[i], id, out);
	}
	
	for (int i = 0; i != n->numchildren; i++) {
		id++;
		out << me << " -> " << id << ";\n";
		print_nodeAP(n->children[i], id, out);
	}
}

void print_treeAP(struct node *tree, int version) {
//#define DEBUG
#ifdef DEBUG
	std::ofstream outTree(("tree_" + boost::lexical_cast<string>(version) 
						   + ".dot").c_str());
	outTree << "digraph tree {\n";
	int id = 0;
	print_nodeAP(tree, id, outTree);
	outTree << "}\n";
	outTree.close();
#endif
}

bool get_costAP(graph &g, int version, string root, modelData *data, 
				modelMsg &mMsg) {
		
	int min = mMsg.start;
	int max = mMsg.stop;
	int step = mMsg.step;
	int threadDepth = mMsg.threadDepth;
	string pathToTop = mMsg.pathToTop;
	struct machine* arch;
	
	if (threadDepth > 0)
		arch = mMsg.parallelArch;
	else
		arch = mMsg.serialArch;
	
	long long* result;
	double *cost_est;
	
	for (long long i = min; i < max+1; i += step) {
		glbl_itsAP = i;
		
		// calculate cost
		int nThreads = arch->threads;
		struct node *tree = build_treeAP(g, threadDepth, nThreads);
		
		result = all_missesP(arch, tree);
		cost_est = parallel_costs(arch, tree);

		vector<double> tmp;
		tmp.clear();
		// NOTE: for parallel both data points are valid, for serial
		// we only want the second of the two.
		if (threadDepth > 0) {
			if (cost_est[0] == 0.0) {
				//std::cout << "WARNING: parallel model, cost of 0\n";
			}
			tmp.push_back(cost_est[0]);
			if (cost_est[1] == 0.0) {
				//std::cout << "WARNING: parallel model, cost of 0\n";
			}
			tmp.push_back(cost_est[1]);
		}
		else {
			if (cost_est[1] == 0.0) {
				//std::cout << "WARNING: parallel model, cost of 0\n";
			}
			tmp.push_back(cost_est[1]);
			tmp.push_back(cost_est[1]);
		}
		
		//std::cout << cost_est[0] << " " << cost_est[1] << "\n";
		data->add_dataSet(i,tmp);
		
		free(result);
		free(cost_est);
		
		print_treeAP(tree, version);
		delete_treeP(tree);
	}
	

	return true;
}

bool evalGraphAP(graph &g, int vid, string root, modelMsg &mMsg, versionData *verData) {
	// create the data structure
	modelData *newData = new modelData();
	verData->add_model(analyticParallel,newData);

	// get the cost
	bool status = get_costAP(g,verData->vid, root, newData, mMsg);
	
	return status;
}

bool insertOrderedAP(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData) {
	
	// NOTE: this is copied in the from the serial analytic model so this
	// never considers more than a single data point.  This needs to be
	// updated to handle the parallel models cost bounds.
	
	modelData *newData = verData->get_model_by_name(analyticParallel);
	if (newData == NULL) {
		std::cout << "Parallel analytic modeling error, maintaining unorderded list\n";
		verList.push_back(verData);
		return false;
	}
	
	// There are two data points in the parallel model
	// using average for true parallel and second value for
	// serial.  In the serial case only the second value is
	// stored twice in get_cost so average can be used in both
	// cases and still return desired results.
	double cost_back = newData->get_back_avg();
	
	// insert this version into the ordered version list
	if (verList.size() == 0) {
		verList.push_back(verData);
	}
	else {
		// find the ratio of performance between the best performing and the
		// new version.
		modelData *bestData = verList.front()->get_model_by_name(analyticParallel);
		if (bestData == NULL) {
			// model not found
			std::cout << "ERROR: Unable to find the data for the parallel analytic model\n";
			return false;
		}
		double currBest = bestData->get_back_avg();
		double ratio = currBest / cost_back;
		
		if (1-threshold < ratio) {
			list<versionData*>::iterator itr;
			
			// this version is within the top 'threshold'
			// if this version is not withing the threshold, do 
			// not keep the version around
			
			// we want to keep this versionList ordered so insert
			// the new routine in the correct location, and cut
			// off values that are no longer within the threshold
			if (cost_back < currBest) {
				// if the new value is the best, push front and remove
				// end values that are no longer within the new threshold
				for (itr = verList.begin(); itr != verList.end(); ++itr) {
					double currCost = (*itr)->get_model_by_name(analyticParallel)->get_back_avg();
					if (1-threshold >= cost_back / currCost) {
						verList.erase(itr,verList.end());
						break;
					}
				}
				verList.push_front(verData);
			}
			else {
				// if the new value is within the threshold insert in correct location
				bool inserted = false;
				for (itr = verList.begin(); itr != verList.end(); ++itr) {
					double currCost = (*itr)->get_model_by_name(analyticParallel)->get_back_avg();
					if (cost_back < currCost) {
						verList.insert(itr,verData);
						inserted = true;
						break;
					}
				}
				if (! inserted)
					verList.push_back(verData);
			}
		}
		
	}
	
	return true;
}

