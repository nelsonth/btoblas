#include <map>
#include <deque>
#include <iostream>
#include <fstream>
#include "boost/next_prior.hpp"
#include "build_graph.hpp"
#include "translate_utils.hpp"
#include "iterator.hpp"
#include "optimizers.hpp"

using namespace std;

extern std::string precision_type;

// ordered list of operations.  first operations should execute first
// last should be last.
extern vector<unsigned int> operationOrder;

int depth(subgraph* sg)
{
        if (sg == 0)
            return 0;
        else
            return 1 + depth(sg->parent);
}

bool ancestor(subgraph* a, subgraph* p)
{
    if (p == 0)
        return false;
    else if (p->parent == a)
        return true;
    else 
        return ancestor(a, p->parent);
}

subgraph* get_child_ancestor(subgraph* a, subgraph* c)
{
    if (c && c->parent == a)
        return c;
    else
        return get_child_ancestor(a, c->parent);
}



void topo_sort_r(vertex u, graph& g, deque<vertex>& order, vector<bool>& visited) 
{
    visited[u] = true;
    for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
        vertex v = g.adj(u)[i];
        if (!visited[v])
            topo_sort_r(v, g, order, visited);
    }
    order.push_front(u);
}

void topo_sort(graph& g, deque<vertex>& order) 
{
    vector<bool> visited(g.num_vertices(), false);
    for (vertex i = 0; i != g.num_vertices(); ++i)
        if (! visited[i])
            topo_sort_r(i, g, order, visited);
}

void get_child_verts(subgraph* current, vector<vertex> &verts) {
    // gather all of the vertices in current and from all of the child
    // subgraphs
    if (current) {
        for (unsigned int i = 0; i < current->vertices.size(); ++i) {
            verts.push_back(current->vertices[i]);
        }
        for (unsigned int i = 0; i < current->subs.size(); ++i) {
            get_child_verts(current->subs[i],verts);
        }
    }
}

int toponum = 0;

void order_subgraphs(deque<vertex>& order, 
        map<vertex,subgraph*>& new_sub, 
        map<vertex,vertex>& new_old,
        subgraph* current,
        vector<vertex> const& vertices,
        vector<subgraph*> const& subgraphs,
        graph& g)
{
    // create the graph to sort
    graph sg;
    map<vertex,vertex> old_new;
    //std::cerr << "starting graph creation " << vertices.size() << " " << subgraphs.size() << std::endl;
    {
        map<subgraph*,vertex> sub_new;
        // create the vertices
        for (unsigned int i = 0; i != vertices.size(); ++i) {
            vertex old = vertices[i];
            vertex n = sg.add_vertex(g.info(old));
            old_new[old] = n;
            new_old[n] = old;
        }
        for (unsigned int i = 0; i != subgraphs.size(); ++i) {
            vertex_info vi;
            vertex n = sg.add_vertex(vi);
            new_sub[n] = subgraphs[i];
            sub_new[subgraphs[i]] = n;
        }  
        
        // get all current level and verts of all children subgraphs
        // this is only needed when not at the top level.
        vector<vertex> childVerts;
        if (current)
            get_child_verts(current,childVerts);

        //std::cerr << sg.num_vertices() << " vertices added" << std::endl;
        // create the edges
        for (vertex u = 0; u != g.num_vertices(); ++u) {
            for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
                vertex v = g.adj(u)[i];
                // normal edges in this subgraph
                if (g.find_parent(u) == current
                    && g.find_parent(v) == current) {
                    if (old_new[u] != old_new[v])
                        sg.add_edge_no_dup(old_new[u],old_new[v]);
                }

                // edges from nested subgraph to this subgraph
                if (ancestor(current, g.find_parent(u))
                    && g.find_parent(v) == current) {
                    if (sub_new[get_child_ancestor(current, 
                            g.find_parent(u))] != old_new[v])
                        sg.add_edge_no_dup(sub_new[get_child_ancestor(current, g.find_parent(u))], old_new[v]);
                }
                // edges from this subgraph to nested subgraph
                if (g.find_parent(u) == current
                    && ancestor(current, g.find_parent(v))) {
                    if (old_new[u] != sub_new[get_child_ancestor(current, 
                                g.find_parent(v))])
                        sg.add_edge_no_dup(old_new[u], 
                            sub_new[get_child_ancestor(current, 
                            g.find_parent(v))]);
                }

                // edges from one nested subgrpah to another
                if (g.find_parent(u) != g.find_parent(v)
                        && ancestor(current, g.find_parent(u))
                    && ancestor(current, g.find_parent(v))) {
                    if (sub_new[get_child_ancestor(current, g.find_parent(u))] != 
                        sub_new[get_child_ancestor(current, g.find_parent(v))])
                        sg.add_edge_no_dup(sub_new[get_child_ancestor(current, g.find_parent(u))], 
                                sub_new[get_child_ancestor(current, g.find_parent(v))]);
                }
                
                // there can be dependencies above this level of subgraph
                // that come back into this subgraph
                // so any vertex in a subgraph that is a parent level 
                // is important
                // 1) from vertex inside this level -> outside
                if (g.find_parent(u) == current && !(g.find_parent(v) == current || ancestor(current, g.find_parent(v)))) {
                    set<subgraph*> sgs;
                    vector<vertex> verts;

                    vector<vertex> lchild(childVerts);
                    set<subgraph*> empty;
                    while (1) {
                        verts.clear();
                        int reachable = 
                        check_reachable_new_r(v,g,lchild,sgs,empty,true,
                                              verts);
                        
                        if (reachable < 0) 
                            break;
                        
                        lchild.erase(find(lchild.begin(),lchild.end(),
                                          reachable));
                        
                        //cout << u << "->" << reachable << "\n";
                        if (find(vertices.begin(),vertices.end(),reachable) 
                            != vertices.end()) {
                            // vertex to vertex
                            vertex l = old_new[u];
                            vertex r = old_new[reachable];
                            if (l != r)
                                sg.add_edge_no_dup(l,r);
                        }
                        else {
                            // vertex to subgraph
                            vertex l = old_new[u];
                            vertex r = sub_new[get_child_ancestor(
                                    current, g.find_parent(reachable))];
                            if (l != r) 
                                sg.add_edge_no_dup(l,r);
                        }
                    }
                }
                // 2) from nested subgraph inside this level -> outside
                if (ancestor(current, g.find_parent(u)) && !(g.find_parent(v) == current || ancestor(current, g.find_parent(v)))) {
                    set<subgraph*> sgs;
                    vector<vertex> verts;

                    vector<vertex> lchild(childVerts);
                    set<subgraph*> empty;
                    while (1) {
                        verts.clear();
                        int reachable = check_reachable_new_r(v,g,
                                    lchild,sgs,empty,true,verts);
                        
                        if (reachable < 0) 
                            break;
                        
                        lchild.erase(find(lchild.begin(),lchild.end(),
                                          reachable));
                        
                        //cout << u << "->" << reachable << "\n";
                        if (find(vertices.begin(),vertices.end(),reachable) 
                            != vertices.end()) {
                            // subgraph to vertex
                            vertex l = sub_new[get_child_ancestor(current, 
                                        g.find_parent(u))];
                            vertex r = old_new[reachable];
                            if (l != r)
                                sg.add_edge_no_dup(l,r);
                        }
                        else {
                            // subgraph to subgraph
                            vertex l = sub_new[get_child_ancestor(current, 
                                        g.find_parent(u))];
                            vertex r = sub_new[get_child_ancestor(current, 
                                        g.find_parent(reachable))];
                            if (l != r)
                                sg.add_edge_no_dup(l,r);
                        }
                    }
                }
                

            }//for
        }//for
    }//create graph
    //std::cerr << "finished graph creation" << std::endl;  

#if 0
    { 
    	std::cout << toponum << "\n";
        std::ofstream fout(string("topo" + boost::lexical_cast<string>(toponum++) + ".dot").c_str());
        print_graph(fout, sg);
    }
#endif

    // topologically sort the graph
    topo_sort(sg, order);
    //std::cerr << "finished topo sort" << std::endl;
}

void order_subgraphs_wrapper(vector<subgraph*> &sgorder, subgraph *sg, 
                             graph &g) {
	// simplify the call to the above order subgraphs
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
  	for (unsigned int i = 0; i < order.size(); i++) {
        map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter != new_sub.end()) {
            sgorder.push_back(iter->second);
        }
    }
}

string expr_of(vertex u, graph& g, subgraph* cur)
{

	switch (g.info(u).op) {
		case trans:
			assert(g.inv_adj(u).size() == 1);
			return expr_of(g.inv_adj(u)[0], g, cur);
		case negate_op:
			assert(g.inv_adj(u).size() == 1);
			return "(-" + expr_of(g.inv_adj(u)[0], g, cur) + ")";
		case squareroot:
			assert(g.inv_adj(u).size() == 1);
			return "(" + expr_of(g.inv_adj(u)[0], g, cur) + ")";	 
		case add:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of(g.inv_adj(u)[0], g, cur) + "+" + expr_of(g.inv_adj(u)[1], g, cur) + ")";
		case subtract:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of(g.inv_adj(u)[0], g, cur) + "-" + expr_of(g.inv_adj(u)[1], g, cur) + ")";
		case multiply:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of(g.inv_adj(u)[0], g, cur) + "*" + expr_of(g.inv_adj(u)[1], g, cur) + ")";
		case divide:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of(g.inv_adj(u)[0], g, cur) + "/" + expr_of(g.inv_adj(u)[1], g, cur) + ")";
		case get_element: {
			string indx;
			indx.assign(var_name,depth(g.find_parent(u)),1);
			
			if (g.info(g.inv_adj(u)[0]).op == get_row_from_column 
				|| g.info(g.inv_adj(u)[0]).op == get_column_from_row)
				indx += "*"+g.info(g.inv_adj(u)[0]).t.dim.step;
			
			
			if (g.info(g.inv_adj(u)[0]).op == output || g.info(g.inv_adj(u)[0]).op == input)
				return g.info(g.inv_adj(u)[0]).label + "[" + indx +"]";
			
			vertex pred = g.inv_adj(u)[0];
			while (g.info(pred).op == partition_cast) {
				pred = g.inv_adj(pred)[0];
			}
			
			if (g.info(pred).t.k == scalar && g.adj(pred).size() > 1) {
				for (unsigned int i = 0; i < g.adj(pred).size(); ++i) {
					vertex s = g.adj(pred)[i];
					if (s == u) continue;
					
					if (g.info(s).op == input || g.info(s).op == output)
						return g.info(s).label + "[" + indx + "]";
				}
			}
			return "t" + boost::lexical_cast<string>(pred) 
			+ "[" + indx + "]";
		}
		case store_element:
		case store_add_element: {
			char itr = var_name[depth(g.find_parent(u))];
			for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
				if (g.info(g.adj(u)[i]).op == output) {
					return g.info(g.adj(u)[i]).label + "[" + itr + "]";
				}
			}
			
			return "t" + boost::lexical_cast<string>(g.adj(u)[0]) 
			+ "[" + itr + "]";
			//return "t" + boost::lexical_cast<string>(u);
            
		}
		case get_row:
		case get_column:
		case get_row_from_column:
		case get_column_from_row:
		case store_row:
		case store_column:
		case store_add_row:
		case store_add_column:
		case temporary:
			return "t" + boost::lexical_cast<string>(u);
		case input:
		case output:
			return g.info(u).label;
		case sumto:
			return "t" + boost::lexical_cast<string>(u);
		default:
			return "?";
	}
}

string expr_of_noPtr(vertex u, graph& g, subgraph* cur, std::map<unsigned int, 
					 std::pair<string, string> > &indexMap)
{			
	switch (g.info(u).op) {
		case trans:
			assert(g.inv_adj(u).size() == 1);
			return expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap);
		case negate_op:
			assert(g.inv_adj(u).size() == 1);
			return "(-" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + ")";
		case squareroot:
			assert(g.inv_adj(u).size() == 1);
			return "sqrt(" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + ")";
		case add:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + "+" + expr_of_noPtr(g.inv_adj(u)[1], g, cur, indexMap) + ")";
		case subtract:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + "-" + expr_of_noPtr(g.inv_adj(u)[1], g, cur, indexMap) + ")";
		case multiply:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + "*" + expr_of_noPtr(g.inv_adj(u)[1], g, cur, indexMap) + ")";
		case divide:
			assert(g.inv_adj(u).size() == 2);
			return "(" + expr_of_noPtr(g.inv_adj(u)[0], g, cur, indexMap) + "/" + expr_of_noPtr(g.inv_adj(u)[1], g, cur, indexMap) + ")";
		case get_element: {
			char itr = var_name[depth(g.find_parent(u))];
			map<vertex,pair<string,string> >::iterator index_found = indexMap.find(g.inv_adj(u)[0]);
			if (index_found != indexMap.end()) {
				return indexMap[g.inv_adj(u)[0]].second + indexMap[g.inv_adj(u)[0]].first + "[" + itr +"]";
			} else if (g.info(g.inv_adj(u)[0]).label.compare("") == 0) {
				std::cerr << "bad index in expr_of" << endl;
			} else {
				return g.info(g.inv_adj(u)[0]).label + "[" + itr + "]";
			}
			
		}
		case store_element:
		case store_add_element: {
			
			char itr = var_name[depth(g.find_parent(u))];
			vertex succ = g.adj(u)[0];
			for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
				if (g.info(g.adj(u)[i]).op == output) {
					succ = g.adj(u)[i];
					break;
				}
			}
			return indexMap[succ].second + indexMap[succ].first + "[" + itr +"]";
		}
		case get_row:
		case get_column:
		case get_row_from_column:
		case get_column_from_row:
		case store_row:
		case store_column:
		case store_add_row:
		case store_add_column:
		case temporary:
			return "t" + boost::lexical_cast<string>(u);
		case input:
		case output:
			return g.info(u).label;
		case sumto:
			return "t" + boost::lexical_cast<string>(u);
		default:
			return "?";
	}
}


string type_to_c(type t, bool ptr) 
{
    switch (t.k) {
    case unknown: return "";
    case scalar: 
      if (ptr)
        return precision_type + string("*");
      else
        return precision_type;
    case row:
    case column: return precision_type + string("*");
    default: return "";
    }
}

string type_to_noPtr(type &t, string name,  bool ptr)
{
    switch (t.k) {
    case unknown: return "";
    case scalar: 
      if (ptr)
        return precision_type + string("* ") + name;
      else
        return precision_type + " " + name;
    case row:
    case column: {
		string s = precision_type + " " + name + "[" + t.dim.dim + "]";
		//kind k = t.k;
		type *u = t.t;
		while (u) {
			if (u->k == scalar) {
				break;
			} else {
				s += "[" + u->dim.dim + "]";
			}
			u = u->t;
		}
		return s;
	}
    default: return "";
    }
}

string container_size_type(type &tt) {
	string high, low = "";
	high = tt.dim.dim;
	string pr = "";
    
    bool seenRow, seenCol;
    seenRow = false;
    seenCol = false;
    
	type *t =tt.t;
	while (t) {
        if (t->k == parallel_reduce) {
            if (pr.compare("") == 0)
                pr = t->dim.dim+"*";
            else
                pr = t->dim.dim+"*"+pr;
        }
		if (t->k == scalar)
			break;
		
        // special case parallel reduce structures
        // need to make sure we get all pr and top row and 
        // top column.
        if (tt.k == parallel_reduce) {
            if (t->k == row && !seenRow) {
                seenRow = true;
                low += "*" + t->dim.dim;
            }
            if (t->k == column && ! seenCol) {
                seenCol = true;
                low += "*" + t->dim.dim;
            }
            if (seenRow && seenCol)
                break;
        }
        else if (t->k != tt.k) {
			low = "*" + t->dim.dim;
			break;
		}
		t = t->t;
	}
	
	return pr + high + low;
}

string container_size(vertex v, graph const& g) {
    // strip the const qualifier from graph const&
    type &t = *(type*)(void*)&g.info(v).t;
    return container_size_type(t);
}

string size_params(type &t) 
{
	string tmp;
    switch (t.k) {
        case unknown: return "";
        case scalar: return "";
        case column:
        	tmp = "int " + t.dim.dim + ", ";
        	if (t.t && t.t->k == row)
        		return tmp + "int " + t.t->dim.dim + ", ";
        	return tmp; 
        case row:
        	tmp = "int " + t.dim.dim + ", ";
        	if (t.t && t.t->k == column)
        		return "int " + t.t->dim.dim + ", " + tmp;
        	return tmp;
        default: return "";
    }
}

string size_params_no_type(type &t) 
{
	string tmp;
    switch (t.k) {
        case unknown: return "";
        case scalar: return "";
        case column:
        	tmp = t.dim.dim + ", ";
        	if (t.t && t.t->k == row)
        		return tmp + t.t->dim.dim + ", ";
        	return tmp; 
        case row:
        	tmp = t.dim.dim + ",";
        	if (t.t && t.t->k == column)
        		return t.t->dim.dim + ", " + tmp;
        	return tmp;
        default: return "";
    }
}

string size_params_type_only(type &t) 
{
	string tmp;
    switch (t.k) {
        case unknown: return "";
        case scalar: return "";
        case column:
        	tmp = "int, ";
        	if (t.t && t.t->k == row)
        		return tmp + "int, ";
        	return tmp; 
        case row:
        	tmp = "int, ";
        	if (t.t && t.t->k == column)
        		return "int, " + tmp;
        	return tmp;
        default: return "";
    }
}

	
	
//generates the function args, i.e. (double *A, int A_ncols....)
//ideally this code would also be shared with translate_utils.cpp, but it's not yet
//diffout = used so blas can send to different outputs, otherwise ""
std::string function_args(map<string,type*> const &inputs, 
						map<string, type*> const &outputs, 
						map<string,pair<vertex,type> > &data, 
						std::string diffout,
						signature_t sig,
						bool noPtr) {
	
	/* currently this is supported
	   enum signature_t declared in translate_utils.hpp
	 typeOnly,					// (double, int, double*, ...)
	 nameOnly,					// (alpha, y, A, ...)
	 typeWithName,				// (double alpha, int y, double *A, ...)
								// (double alpha, int y, double A[y], ...)
	*/
	 
	bool typeOnlyB = false, nameOnlyB = false, typeWithNameB = false;
	
	if (sig == typeOnly) {
		// (double, int, double*, ...)
		typeOnlyB = true; 
	}
	else if (sig == nameOnly) { 
		// (alpha, y, A, ...)
		nameOnlyB = true; 
	}
	else if (sig == typeWithName) { 
		// (double alpha, int y, double *A, ...) or 
		// (double alpha, int y, double A[y], ...)
		typeWithNameB = true; 
	}
	else { 
		std::cout << "translate_utils.cpp; function_args(); incorrect signature requested\n";
	}
	
	std::string args = "(";
  	for (map<string,type*>::const_iterator i = inputs.begin(); i != inputs.end(); ) {
  		// This is checking for inouts, they will be handled with outputs.
  		// The inouts could be checked for this instead of inputs which may
  		// be more correct.
  		if (outputs.find(i->first) != outputs.end()) {
			++i;
			continue;
		}
		string name = i->first;
  		
		if (typeOnlyB) {
			// (double, int, double*, ...)
			// vla's are just pointers, so no need to special case them.
			args += size_params_type_only(*i->second);
			args += type_to_c(*i->second);
		}
		else if (typeWithNameB) {
			args += size_params(*i->second);
			if (noPtr) {
				// (double alpha, int y, double A[y], ...)
				args += type_to_noPtr(*i->second,name);
			} else {
				// (double alpha, int y, double *A, ...)
				args += type_to_c(*i->second) + " " + name;
			}
		} else if (nameOnlyB) {
			// (alpha, y, A, ...)
			args += size_params_no_type(data[i->first].second) + name;
		}
    	++i;
    	while (i != inputs.end() && outputs.find(i->first) != outputs.end())
    		++i;
    	if (i != inputs.end()) {
			args += ", ";
		}
  	}
  	
  	if (inputs.begin() != inputs.end()) {
		args += ", ";
	}
	
  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) { 
		string name;
		if (i->second->k != scalar) {
			if (typeOnlyB || typeWithNameB) {
				name = i->first;
			}
			else {
				//name = diffall + diffout + i->first;
				name = diffout + i->first;
			}
		} else {
			name = diffout + i->first;
		}
		
		if (typeOnlyB) {
			// (double, int, double*, ...)
			// vla's are just pointers, so no need to special case them.
			args += size_params_type_only(*i->second);
			args += type_to_c(*i->second, true);
		}
		else if (typeWithNameB) {
			args += size_params(*i->second);
			if (noPtr) {
				// (double alpha, int y, double A[y], ...)
				args += type_to_noPtr(*i->second, name, true);
			} else {
				// (double alpha, int y, double *A, ...)
				args += type_to_c(*i->second, true) + " " + name;
			}
			if (i->second->k == scalar) {
				args += "_ptr";
			}
		} else if (nameOnlyB) {
			// (alpha, y, A, ...)
			if (i->second->k == scalar) {
				args += "&";
			}
			args += size_params_no_type(data[i->first].second) + name;
		}
		
	    if (boost::next(i) != outputs.end()) {
			args += ", ";
		}
  	}
	
  	args += ")";
	return args;
}

string make_function_signature(string name, map<string,type*>const& inputs, 
							   map<string,type*>const& outputs, 
							   map<string,pair<vertex,type> >& data,
							   string diffout,
							   enum signature_t sig,
							   bool noPtr) {
	
	// check
	switch (sig) {
		case nameOnly: 
		case typeOnly:
		case typeWithName: 
			break;
		default:
			std::cout << "ERROR: translate_utils.cpp: make_function_signature(): unhandled "
					<< "signature type\n";
			return "";
	}
	
	string ret = "";
	
	ret += "void " + name + "\n";
	ret += function_args(inputs,outputs,data,diffout,sig,noPtr);
	ret += "\n{\n";
	
	
	return ret;
}



