#include "cost_estimate_common.hpp"
#include "translate_to_code.hpp"
#include "translate_utils.hpp"
#include <fstream>
#include "modelInfo.hpp"
#include <list>

using std::list;


extern "C" {
#include "memmodel_an/tree.h"
#include "memmodel_an/memmodel_clean.h"
#include "memmodel_an/cost.h"
#include "memmodel_an/machines.h"
#include "memmodel_an/your_machine.h"
}

//#define DEBUG

using namespace std;

char *string_to_charpAS(string s) {
	const char *t = s.c_str();
	char *ret = (char*) malloc(sizeof(char) * (strlen(t)+1));
	strcpy(ret, t);
	return ret;
}

char **vector_to_charppAS(std::vector<char> iters) {
	char **ret = (char**) malloc(sizeof(char*) * iters.size());
	for (int i = 0; i != iters.size(); i++) {
		ret[i] = (char*) malloc(sizeof(char) * 2);
		ret[i][0] = iters[i];
		ret[i][1] = NULL;
	}
	
	return ret;
}

long long glbl_itsAS;
long long get_iterationsAS(subgraph *sg) {
	//return sg->iterations;
	return glbl_itsAS;
}

struct var* graph_to_tree_varAS(vertex v, int init_d, dir d, graph &g) {
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
				std::cout << "WARNING: cost_estimate.cpp: graph_to_tree_varAS: unexpected input 2\n"
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
	return create_var(string_to_charpAS(lbl), vector_to_charppAS(iters), 
							iters.size());
}

struct node* build_treeAS_rAS(subgraph *sg, graph &g) {
	vector<subgraph*> order;
	order_subgraphs_wrapper(order, sg, g);
	
	vector<struct node *> v_nodes;
	for (int i = 0; i != order.size(); i++) {
		v_nodes.push_back(build_treeAS_rAS(order[i], g));
	}
	
	if (true) { //(sg->subs.size() == 0) {
		// create variables
		vector<struct var*> v_vars;
		for (int j = 0; j != sg->vertices.size(); j++) {
			switch (g.info(sg->vertices[j]).op) {
			case trans: {
				std::cout << "?? cost_estimate.cpp: build_treeAS_rAS(): transpose?\n";
				break;
			}
			case add:
			case subtract:
			case multiply: {
				vertex u = sg->vertices[j];
				//left
				v_vars.push_back(graph_to_tree_varAS(g.inv_adj(u)[0],depth(g.find_parent(u)), up, g));
				//right
				v_vars.push_back(graph_to_tree_varAS(g.inv_adj(u)[1],depth(g.find_parent(u)), up, g));
				//result
				for (int i = 0; i != g.adj(u).size(); i++) {
					// if adj is an operation, then u needs to be the variable
					vertex a = g.adj(u)[i];
					if (g.info(a).op == add || g.info(a).op == subtract || g.info(a).op == multiply) {
						string lbl = g.info(u).label;
						if (lbl.compare("") == 0)
							lbl = "t" + boost::lexical_cast<string>(u);
						v_vars.push_back(create_var(string_to_charpAS(lbl), NULL, 0));
						
						continue;
					}
					v_vars.push_back(graph_to_tree_varAS(g.adj(u)[i],depth(g.find_parent(u)), down, g));
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
			struct node *stmt = create_state(vars, v_vars.size());
			v_nodes.push_back(stmt);
		}
	}
	
	// create loop
	char *iterate = (char*) malloc(sizeof(char) * 2);
	iterate[0] = iterators[depth(sg)];
	iterate[1] = NULL;
	
	struct node **nodes = (struct node **) malloc(sizeof(struct node*) * v_nodes.size());
	for (int i = 0; i != v_nodes.size(); i++) {
		nodes[i] = v_nodes[i];
	}

	long long iterations = get_iterationsAS(sg);
	struct node *loop = create_loop(iterations, nodes, v_nodes.size(), iterate);

	//return current node
	return loop;
}

void print_varAS(struct var *v, int &id, std::ofstream &out) {
	out << id << "[label=\"" << v->name << "; " << v->iterates << "\\n";
	if (v->iterates != 0)
		out << v->iterate[0];
	
	for (int i = 1; i < v->iterates; i++) {
		out << "," << v->iterate[i];
	}
	out << "\"];\n";
}

void print_nodeAS(struct node* n, int &id, std::ofstream &out) {
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
		print_varAS(n->vars[i], id, out);
	}

	for (int i = 0; i != n->numchildren; i++) {
		id++;
		out << me << " -> " << id << ";\n";
		print_nodeAS(n->children[i], id, out);
	}
}

void print_treeAS(struct node *tree, int version) {
#ifdef DEBUG
	std::ofstream out(("tree_" + boost::lexical_cast<string>(version) 
					+ ".dot").c_str());
	out << "digraph tree {\n";
	int id = 0;
	print_nodeAS(tree, id, out);
	out << "}\n";
	out.close();
#endif
}

struct node *build_treeAS(graph &g) {
		
	// order subgraphs
	vector<subgraph*> order;
	order_subgraphs_wrapper(order, 0, g);
	
	vector<struct node*> v_trees;
	for (int i = 0; i != order.size(); i++) {
		v_trees.push_back(build_treeAS_rAS(order[i], g));
	}
	
	struct node **trees = (struct node **) malloc(sizeof(struct node*) * v_trees.size());
	for (int i = 0; i != v_trees.size(); i++) {
		trees[i] = v_trees[i];
	}
	
	char *iterate = (char*) malloc(sizeof(char) * 2);
	iterate[0] = 'a';
	iterate[1] = NULL;
	
	struct node *forest = create_loop(1, trees, v_trees.size(), iterate);
		
	return forest;	
}

bool get_cost(graph &g, int version, int min, int max, int step, string root,
				modelData *data) {
//#define DEBUG
#ifdef DEBUG
	std::ofstream cost_out((root + ".csv").c_str()); 
#endif
	
	struct machine* arch = your_machine();
	long long* result;
	double cost_est;
	
	for (long long i = min; i < max+1; i += step) {
		glbl_itsAS = i;
		
		// calculate cost

		struct node *tree = build_treeAS(g);
		//print_treeAS(tree, version);
		
		result = all_misses(arch, tree);
		cost_est = new_cost(arch, tree);
		
		data->add_data(i,cost_est);
		
#ifdef DEBUG
		cost_out << "cost," << root << version << "," << i << "," << i << ",1,1," << cost_est << "\n";
		
		for(int j = 0; j < arch->numcaches; j++) {
			cost_out << arch->caches[j]->name << "," << root << version << "," << i << "," << i
					  << ",1,1," << result[j] << cost_est <<"\n";
		}
#endif
		free(result);
		delete_tree(tree);
	}
	delete_machine(arch);

	return true;
}

bool evalGraphAS(graph &g, int vid, string root, modelMsg &mMsg, versionData *verData) {
	// create the data structure
	modelData *newData = new modelData();
	verData->add_model(analyticSerial,newData);
	
	// get the cost
	return get_cost(g,verData->vid,mMsg.start,mMsg.stop,mMsg.step, root, newData);
}

bool insertOrderedAS(list<versionData*> &verList, modelMsg &mMsg,
				   double threshold, versionData *verData) {

	modelData *newData = verData->get_model_by_name(analyticSerial);
	if (newData == NULL) {
		std::cout << "Analytic modeling error, maintaining unorderded list\n";
		verList.push_back(verData);
		return false;
	}
	
	// There is only one data point in newData so getting any should
	// all be the same
	double cost_back = newData->get_front();
	
	// insert this version into the ordered version list
	if (verList.size() == 0) {
		verList.push_back(verData);
	}
	else {
		// find the ratio of performance between the best performing and the
		// new version.
		modelData *bestData = verList.front()->get_model_by_name(analyticSerial);
		if (bestData == NULL) {
			// model not found
			std::cout << "ERROR: Unable to find the data for the analytic model\n";
			return false;
		}
		double currBest = bestData->get_back();
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
					double currCost = (*itr)->get_model_by_name(analyticSerial)->get_back();
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
					double currCost = (*itr)->get_model_by_name(analyticSerial)->get_back();
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

