#include "cost_estimate_common.hpp"
#include "translate_to_code.hpp"
#include "translate_utils.hpp"
#include <fstream>
#include "modelInfo.hpp"
#include <list>

using std::list;


extern "C" {
#include "memmodel_sym/tree_sym.h"
#include "memmodel_sym/memmodel_sym.h"
#include "memmodel_sym/cost.h"
#include "memmodel_sym/machines.h"
}

using namespace std;

void order_subgraphs(vector<subgraph*> &sgorder, subgraph *sg, graph &g) {
	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
  	
  	if (sg == 0) {
  		vector<vertex> verts;
	  	for (unsigned int i = 0; i != g.num_vertices(); ++i)
	    	if (g.find_parent(i) == 0 && (g.adj(i).size() > 0 || g.inv_adj(i).size() > 0))
	      		verts.push_back(i);
	  	
	  	order_subgraphs(order, new_sub, new_old, 0, verts, g.subgraphs, g);
  	}
  	else {
  		order_subgraphs(order, new_sub, new_old, sg, sg->vertices, sg->subs, g);
  	}
  	
  	map<vertex,subgraph*>::iterator itr = new_sub.begin();
  	for (; itr != new_sub.end(); itr++)
  		sgorder.push_back(itr->second);		  	
}

char *string_to_charp(string s) {
	const char *t = s.c_str();
	char *ret = (char*) malloc(sizeof(char) * (strlen(t)+1));
	strcpy(ret, t);
	return ret;
}

char **vector_to_charpp(std::vector<char> iters) {
	char **ret = (char**) malloc(sizeof(char*) * iters.size());
	for (int i = 0; i != iters.size(); i++) {
		ret[i] = (char*) malloc(sizeof(char) * 2);
		ret[i][0] = iters[i];
		ret[i][1] = NULL;
	}
	
	return ret;
}

long long glbl_its;
long long get_iterations(subgraph *sg) {
	//return sg->iterations;
	return glbl_its;
}

struct var* graph_to_tree_var(vertex v, int init_d, dir d, graph &g) {
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
				std::cout << "WARNING: cost_estimate.cpp: graph_to_tree_var: unexpected input 2\n"
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
	return create_var(string_to_charp(lbl), vector_to_charpp(iters), 
							iters.size());
}

struct node* build_tree_r(subgraph *sg, graph &g) {
	vector<subgraph*> order;
	order_subgraphs(order, sg, g);
	
	vector<struct node *> v_nodes;
	for (int i = 0; i != order.size(); i++) {
		v_nodes.push_back(build_tree_r(order[i], g));
	}
	
	if (true) { //(sg->subs.size() == 0) {
		// create variables
		vector<struct var*> v_vars;
		for (int j = 0; j != sg->vertices.size(); j++) {
			switch (g.info(sg->vertices[j]).op) {
			case trans: {
				std::cout << "?? cost_estimate.cpp: build_tree_r(): transpose?\n";
				break;
			}
			case add:
			case subtract:
			case multiply: {
				vertex u = sg->vertices[j];
				//left
				v_vars.push_back(graph_to_tree_var(g.inv_adj(u)[0],depth(g.find_parent(u)), up, g));
				//right
				v_vars.push_back(graph_to_tree_var(g.inv_adj(u)[1],depth(g.find_parent(u)), up, g));
				//result
				for (int i = 0; i != g.adj(u).size(); i++) {
					// if adj is an operation, then u needs to be the variable
					vertex a = g.adj(u)[i];
					if (g.info(a).op == add || g.info(a).op == subtract || g.info(a).op == multiply) {
						string lbl = g.info(u).label;
						if (lbl.compare("") == 0)
							lbl = "t" + boost::lexical_cast<string>(u);
						v_vars.push_back(create_var(string_to_charp(lbl), NULL, 0));
						
						continue;
					}
					v_vars.push_back(graph_to_tree_var(g.adj(u)[i],depth(g.find_parent(u)), down, g));
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

	long long iterations = get_iterations(sg);
	struct node *loop = create_loop(iterations, nodes, v_nodes.size(), iterate);

	//return current node
	return loop;
}

void print_var(struct var *v, int &id, std::ofstream &out) {
	out << id << "[label=\"" << v->name << "; " << v->iterates << "\\n";
	if (v->iterates != 0)
		out << v->iterate[0];
	
	for (int i = 1; i < v->iterates; i++) {
		out << "," << v->iterate[i];
	}
	out << "\"];\n";
}

void print_node(struct node* n, int &id, std::ofstream &out) {
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
		print_var(n->vars[i], id, out);
	}

	for (int i = 0; i != n->numchildren; i++) {
		id++;
		out << me << " -> " << id << ";\n";
		print_node(n->children[i], id, out);
	}
}

void print_tree(struct node *tree, int version) {
	std::ofstream out(("tree_" + boost::lexical_cast<string>(version) 
					+ ".dot").c_str());
	out << "digraph tree {\n";
	int id = 0;
	print_node(tree, id, out);
	out << "}\n";
}

struct node *build_tree(graph &g) {
		
	// order subgraphs
	vector<subgraph*> order;
	order_subgraphs(order, 0, g);
	
	vector<struct node*> v_trees;
	for (int i = 0; i != order.size(); i++) {
		v_trees.push_back(build_tree_r(order[i], g));
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

double get_cost(graph &g, int version, struct symbolic_store* new_cost, int* new_points, 
				int* num_new_points, int wordsize, double threshold) {
	//struct machine* arch = create_clovertown();
	struct machine* arch = create_quadfather();
	//struct machine* arch = create_opteron();
	
    // calculate cost
    struct node *tree = build_tree(g);
    print_tree(tree, version);	
	cost_est = run_model(tree, arch, wordsize, new_points, points, threshold);
	
	//cleanup
	delete_tree(tree);
	delete_machine(arch);
}

bool evalGraphSYM(graph &g, int vid, string root, modelMsg &mMsg, versionData *verData) {
	/*
	 //vars for symbolic model
	 vector <struct symbolic_store*> costs;
	 struct symbolic_store** new_costs;
	 vector <int> points;
	 int* new_points;
	 int num_new_points;
	 int num_points;
	 int version;
	 int versions[1000];
	 
	get_cost(g,vid, new_cost, new_points, &new_num_points, wordsize, threshold);
	if(vid == 1) {
		for(i = 0; i < new_points; i++) {
			versions.pushback(vid); 
			points.pushback(new_points[i]);
			costs.pushback(new_costs[i]);
		}
	}
	else
		compare_costs(costs, points, new_costs, new_points, num_new_points, versions, vid);
	*/
	return false;
}

bool insertOrderedSYM(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData) {
	
}


double compare_costs(struct symbolic_store* costs, int* points, 
		     struct symbolic_store* new_costs, int* new_points, 
                     int* num_new_points, int* versions, int version) {
  int j = 0;
  int end;
  for(int i = 0; i < num_new_points; i++) {
    end = j;
    //If at the last comparison zone
    if(j == points.size - 1) {
      for(int k = i; k < num_new_points; k++) {
        if(new_costs[k] < costs[j]) {
          if(k + 1 == num_new_points) { //if last of the new points
	    costs[j] = new_costs[k];
	    versions[j] = version;
          }
	  else {
            costs.insert(j, new_costs[k]);
	    versions.insert(j, version);
	    points.pushback(point[k+1]);
            j++;
          }
	}
      }
    }      
    else { //comparing all but the last point of the current best 
      while((points[end] < new_points[i]) && (end < (*points) - 1))
        end++;
      int first = 1;
      for(int k = j; k <= end; k++)
        if(new_costs[i] < costs[k])
        {
          if(first == 1) {
	    if(new_points[i+1] >= points[k+1]) {
	      costs[j] = new_costs[k];
	      versions[j] = version;
              points[j] = points[k];
	    }	
	    else{
              costs.insert(j, new_costs[k]);
	      versions.insert(j, version);
	      points.insert(j+1, point[k+1]);
              j++;
            }
 	    first = 0;
	  }
	  else {
	    if(new_points[i+1] >= points[k+1]) {
	      costs.remove(k);
	      versions.remove(k);
	      points.remove(k);
	      j--;
	      end--;
	    }
	    else
              points[k+1] = new_points[i+1];
          }
        }
      j = end;
    }
  
}

//old code
/*
  for(i = 0; i < num_new_points; i++) {
    if(new_points[i] == -1) { //if point has no cost
	  if(points[0] != -1) { //if there isn't a no cost point already
		if(points[1] < new_points[1]) { //if the no cost region extends beyond the first cost region
          j++;
          while(points[j+1] < new_points[1])
		    j++;
          if(j == 1) { //if no cost only extends into second region
		    for(k = 0; k < (*num_points); k++) {
              points[k+1] = points[k];
              costs[k+1] = costs[k];
              versions[k+1] = versions[k];
            }
            points[0] = -1;
            costs[0] = new_costs[0];
            versions[0] = version;
        	points[1] = new_points[1];
          }
          else { //more than one region affected
            for(k = 1; k < (*num_points) - j + 1; k++) {
              points[k] = points[k+j-1];
              costs[k] = costs[k+j-1];
              versions[k] = versions[k+j-1];
			}
            points[0] = -1;
            costs[0] = new_costs[0];
            versions[0] = version;
			points[1] = new_points[1];
			(*num_points) -= j-1;
            j = 1;
		  }
	    else { //need to add a new point because the no cost regioin is smaller than the previous region
	      points[0] = -1;
		  costs[0] = new_costs[0];
		  versions[0] = version;
		  for(k = 0; k < (*num_points); k++) {
            points[k+1] = points[k];
            costs[k+1] = costs[k];
            versions[k+1] = versions[k];
          }
          (*num_points)++;		
		}  
	  }
	  else { //add in no cost point
        if(points[1] > new_points[1]) { //new point needed
          for(k = 0; k < (*num_points); k++) {
            points[k+1] = points[k];
            costs[k+1] = costs[k];
            versions[k+1] = versions[k];
          }
		  (*num_points)++;
		}
        else {
          j++;
		  while(points[j+1] < new_points[1])
			j++;
		  if(j == 1)
            points[1] = new_points[1];
		  else {
            for(k = 1; k < (*num_points) - j + 1; k++) {
              points[k] = points[k+j-1];
              costs[k] = costs[k+j-1];
              versions[k] = versions[k+j-1];
            }
		    (*num_points) -= j-1;
		    j = 1;
            points[0] = -1;
            costs[0] = new_costs[0];
		    versions[0] = version;
            points[1] = new_points[1];
          }
		  costs[0] = new_costs[0];
          points[0] = -1;
		  versions[0] = version;
        }
      }
    }
    else if(j < (*num_points) - 1) {//we're in the middle
     if(new_points[i] =< new_points[j+1])
    }
    else { //at the end
      if(new_costs[i] < costs[j])
        if(new_point[i] != point[j]) { //if new point needed
          j++;
		  (*num_points)++;
		}
		  point[j] = new_point[i];
		  cost[j] = cost[i];
          version[j] = version;
      else 
        return;
  }*/
}
