#include "translate_to_code.hpp"
#include "partition.hpp" // for check_depth
#include "iterator.hpp"

using namespace std;
extern std::string precision_type;


string translate_declareVariables_intrin(ostream& out, graph &g, vertex u, bool parallel) {
	string iter = string(1,var_name[depth(g.find_parent(u))]);

	string toFree = "";
	switch (g.info(u).op) {
		case get_row_from_column:
		case get_column_from_row: {
			vertex predOrig;
			vertex pred = g.inv_adj(u)[0];
			predOrig = pred;
			while (g.info(pred).op == partition_cast) {
				pred = g.inv_adj(pred)[0];
			}
			
			string prl = g.info(pred).label;
			if (g.info(pred).op != input && g.info(pred).op != output)
				prl = "t" + boost::lexical_cast<string>(pred);
			
			out << precision_type << "* t" << u << " = " << prl
			<< " + " << iter << get_next_elem_stride(g.info(predOrig).t) << ";\n";
			break;
		}
		case get_row:
		case get_column: {
			vertex predOrig;
			vertex pred = g.inv_adj(u)[0];
			predOrig = pred;
			while (g.info(pred).op == partition_cast) {
				pred = g.inv_adj(pred)[0];
			}
			string prl = g.info(pred).label;
			if (g.info(pred).op != input && g.info(pred).op != output)
				prl = "t" + boost::lexical_cast<string>(pred);
			
			out << precision_type << "* t" << u << " = " << prl
			<< " + " << iter << get_next_elem(g.info(predOrig).t) << ";\n";
			break;
		}
		case squareroot: {												//Add more here
			out << precision_type << " t" << u << ";\n";
			break;
		}
		case store_add_row:
		case store_add_column:
		case store_row:
		case store_column: {
			// are any of the successor or cast trails of successors
			// outputs?
			vertex succOrig, succ;
			vertex goodSucc = 100001;
			vertex goodOrig;
			for (int k = 0; k < g.adj(u).size(); ++k) {
				succ = g.adj(u)[k];
				succOrig = succ;
				while (g.info(succ).op == partition_cast) {
					succ = g.adj(succ)[0];
				}
				if (g.info(succ).op == output) {
					goodSucc = succ;
					goodOrig = succOrig;
					break;
				}

				if ((goodSucc > 100000) && (g.info(succ).op & OP_STORE) || g.info(succ).op == temporary || 
                    g.info(succ).op == sumto) {
					goodSucc = succ;
					goodOrig = succOrig;
				}
			}
			
			if (goodSucc > 100000) {
				std::cout << "Error generating code for store\n";
				std::cout << u << "\n";
				exit(0);
			}

			succ = goodSucc;
			succOrig = goodOrig;
			
			string scl = g.info(succ).label;
			if (g.info(succ).op != output)
				scl = "t" + boost::lexical_cast<string>(succ);
			
			out << precision_type << "* t" << u << " = " << scl
			<< " + " << iter << get_next_elem(g.info(succOrig).t) << ";\n";
			break;
		}
		case get_element: {
			break;
		}
		case store_element: 
		case store_add_element: {
			break;
		}
		case temporary:
			if (parallel)
				break;
			switch (g.info(u).t.k) {
				case row:
				case column:
					out << precision_type << "* t" << u << " = (" << precision_type 
					<< "*) malloc(sizeof(" << precision_type << ") * "
					<< container_size(u, g) << ");\n";
					toFree += " free(t"+boost::lexical_cast<string>(u)+");";
					break;
				case scalar:
					out << precision_type << " t" << u << ";\n";
					break;
				default:
					break;
			}//switch inside temporary case
			break;
		case sumto: {
			vertex succ = g.adj(u)[0];
			//while (g.info(succ).op == partition_cast) {
			//	succ = g.adj(succ)[0];
			//}
			string scl = "t" + boost::lexical_cast<string>(succ);
			if (g.info(succ).op == output) {
                scl = g.info(succ).label;
            }
			
			if (g.info(u).t.height < g.info(succ).t.height) {
				// store
				if (g.info(u).t.k == scalar) {
					out << precision_type << " t" << u << " = " << scl;
					if (g.info(succ).t.k != scalar)
						out << "[" << iter << "]";
					out << ";\n";
				}
				else {
					out << precision_type << "* t" << u << " = " << scl
					<< " + " << iter << get_next_elem(g.info(succ).t) << ";\n";
				}
			}
			else {
				if (g.info(u).t.k == scalar) {
					if (depth(g.find_parent(u)) > depth(g.find_parent(succ))) {
						if (parallel)
							out << precision_type << " *t" << u << " = " << scl
								<< "+disp";
						else
							out << precision_type << " *t" << u << " = " << scl;
					}
					else {
						out << precision_type << " t" << u;
					}
					out << ";\n";
				}
				else
					if (depth(g.find_parent(u)) > depth(g.find_parent(succ))) {
						if (parallel) {
							out << precision_type << " *t" << u << " = " << scl 
								<< "+disp*" << container_size(u,g) << ";\n";
						}
						else
							out << precision_type << " *t" << u << " = " << scl << ";\n";
					}
					else {
						out << precision_type << " *t" << u << "= (" << precision_type
						<< "*) malloc(sizeof(" << precision_type << ") * " 
						<< container_size(u,g) << ");\n";
						toFree += " free(t"+boost::lexical_cast<string>(u)+");";
					}
			}
			
			if (parallel)
				break;
			
			if (!been_cleared(u,g,0)) {
				if (g.info(u).t.k == scalar) {
					out << "t" + boost::lexical_cast<string>(u) 
					<< " = 0.0;\n";
				}
				else {
					string cs = container_size(u,g);
					size_t pos = cs.find("__s",0);
					while (pos != string::npos) {
						cs.replace(pos,3,"__m");
						pos = cs.find("__s",0);
					}
					out << "for (ii = 0; ii < " << cs << "; ++ii)\n";
					out << "t" + boost::lexical_cast<string>(u) << "[ii] = 0.0;\n";
				}
			}
			
			break;
		}
		default:
			break;
	}
	return toFree;
}

string translate_tmp_intrin(ostream& out, graph &g, vertex u) {
	string toFree = "";
    
	// create temporary space
	switch (g.info(u).op) {
		case temporary:
            
			switch (g.info(u).t.k) {
				case row:
				case column:
					out << precision_type << "* t" << u << " = (" << precision_type
					<< "*) malloc(sizeof(" << precision_type << ") *"
					<< container_size(u, g) << ");\n";
					toFree += " free(t"+boost::lexical_cast<string>(u)+");";
					break;
				case scalar:
					//out << precision_type << " t" << u << "[1];\n";
					out << precision_type << " t" << u << ";\n";
					break;
				default:
					break;
			}//switch inside temporary case
			break;
		case sumto: {
			vertex succ = g.adj(u)[0];
			while (g.info(succ).op == partition_cast) {
				succ = g.adj(succ)[0];
			}
            
			string scl = "t" + boost::lexical_cast<string>(succ);
			if (g.info(succ).op == output) {
                scl = g.info(succ).label;
            }
			
			int d = (depth(g.find_parent(u)) - depth(g.find_parent(succ)));
			if (d == 0) {
				// just point to
				if (g.info(u).op == squareroot) {
					out << precision_type << " t" << u << ";\n";
				}
				else {
				if (g.info(u).t.k == scalar)
					out << precision_type << " t" << u << " = " << scl << ";\n";
				else
					out << precision_type << " *t" << u << " = " << scl << ";\n";
				}
			}
			else if (d < 0) {
				// treat as temporary
				if (g.info(u).t.k == scalar) {
					//out << precision_type << " t" << u << "[1];\n";
					out << precision_type << " t" << u << ";\n";
				}
				else {
					out << precision_type << "* t" << u << " = (" << precision_type
					<< "*) malloc(sizeof(" << precision_type << ") * "
					<< container_size(u, g) << ");\n";
					toFree += " free(t"+boost::lexical_cast<string>(u)+");";
				}
			}
			else {
				// if the adjacent node is an output, we can use the output space that exists
				// aready so do nothing.  Pointing to another sumto should have the space
				// already allocated, but this check may need to be improved.
				
				// For other ops
				// parallel has shown other cases in the dispatch loops that are working
				// correctly for now.  In these cases, it may be correct to allocate
				// this space here.  Need to find a case for this.
				
				//if (g.info(succ).op != output && g.info(succ).op != sumto) {
				//	std::cout << "WARNING: translate_to_intrin.cpp: translate_tmp_intrin(): "
				//			<< "unexpected graph -- " << u << "\n";
				//}
			}
			
			if (g.info(u).t.k == scalar) {
				out << "t" + boost::lexical_cast<string>(u) << " = 0.0;\n";
			}
			else {
				out << "for (ii = 0; ii < " << container_size(u, g) << "; ++ii)\n";
				out << "t" + boost::lexical_cast<string>(u) << "[ii] = 0.0;\n";
			}
			break;
		}
		case input:
		case output:
			break;
		default: {
			break;
		}
	}
	return toFree;
}

string translate_graph_intrin(ostream& out,
		     subgraph* current,
		     vector<vertex> const& vertices,
		     vector<subgraph*> const& subgraphs,
		     graph& g)
{
	string toFree = "";
	
    // get correct loop based on subgraph iterator details 
    out << current->sg_iterator.getSerialCLoop(current);
	
  	//std::cerr << "starting graph translation" << std::endl;
  	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
  	order_subgraphs(order, new_sub, new_old, current, vertices, 
                    subgraphs, g);

  	//std::cerr << "declaring variables" << std::endl;

  	// Declare variables  	
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,vertex>::iterator iter = new_old.find(order[i]);
    	if (iter != new_old.end()) {
      		vertex u = iter->second;
      			
      		if (g.adj(u).size() > 0) {
        		//std::cerr << "getting iter height " << height(g.info(u).parent) << std::endl;        
        		//std::cerr << "iter: " << iter << std::endl;
				toFree += translate_declareVariables_intrin(out, g, u, 
                                                            false);
      		}//if
    	}//if
  	}//for
  	//std::cerr << "finished declaring variables" << std::endl;

  	// Do computations and store the results
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter != new_sub.end()) {
	    	subgraph* sg = iter->second;
      		toFree += translate_graph_intrin(out, sg, sg->vertices, sg->subs, g);
    	} 
    	else {
      		map<vertex,vertex>::iterator iter = new_old.find(order[i]);
      		if (iter != new_old.end()) {
        		vertex u = iter->second;
			
        		switch (g.info(u).op) {
        			case store_element:
          				if (g.inv_adj(u).size() == 1)
          				{
          					string target;
          					char itr = var_name[depth(g.find_parent(u))];
    						vertex succ = g.adj(u)[0];
              				 
              				if (g.info(succ).op != output)
              					target = "t" + boost::lexical_cast<string>(succ);
              				else
              					target = g.info(succ).label;

							vertex pred = g.inv_adj(u)[0];
							//while (g.info(pred).op == partition_cast) {
							//	pred = g.inv_adj(pred)[0];
							//}
							
              				out << target << "[" << itr << "] = " << expr_of(pred, g, current)
              					<< ";\n";
              								
            				//out << "*t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				}
          				break;
        			case store_add_element:
          				if (g.inv_adj(u).size() == 1)
          				{
          					string target;
          					char itr = var_name[depth(g.find_parent(u))];
    						vertex succ = g.adj(u)[0];
              				 
              				if (g.info(succ).op != output)
              					target = "t" + boost::lexical_cast<string>(succ);
              				else
              					target = g.info(succ).label;

              				out << target << "[" << itr << "] += " << expr_of(g.inv_adj(u)[0], g, current)
              					<< ";\n";
            				//out << "*t" << u << " += " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				}
          				break;
        			case sumto:
        				break;
        				if (g.info(u).t.k == scalar) {
        					if (been_cleared(u, g, 0)) {
        						out << "*t" << u << " += t" << g.inv_adj(u)[0] << ";\n";
        					}
        					else {
        						out << "*t" << u << " = t" << g.inv_adj(u)[0] << ";\n";
        					}
        				}
        				break;
                    case multiply:
						if (g.adj(u).size() > 0) { 
							vertex adj = g.adj(u)[0];
							while (g.info(adj).op == partition_cast) {
								adj = g.adj(adj)[0];
							}

                            string name = "t" + 
                                    boost::lexical_cast<string>(adj);
                            
							if (g.info(adj).op == sumto) { 
								out << name << " += " << expr_of(u, g, current) << ";\n";
							}
						}
        			default:
						// old summation code, sumto should eliminate the need for this
						//out << "t" << g.adj(u)[0] << " += " << expr_of(u, g, current) << ";\n";
          				break;
        		}
      		}
    	}
	}

  	// Sumto cleanup  	
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,vertex>::iterator iter = new_old.find(order[i]);
    	if (iter != new_old.end()) {
      		vertex u = iter->second;
      			
      		if (g.adj(u).size() > 0) {       
        		char iter = var_name[depth(g.find_parent(u))];

        		switch (g.info(u).op) {
        		case sumto: {
      				vertex succ = g.adj(u)[0];
      				string scl = "t" + boost::lexical_cast<string>(succ);
      				if (g.info(succ).op == output) {
                        scl = g.info(succ).label;
                    }
      				
        			if (g.info(u).t.height < g.info(succ).t.height) {
      					// store
      					if (g.info(u).t.k == scalar) {
      						out << scl;
      						if (g.info(succ).t.k != scalar)
      							out << "[" << iter << "]";
      						out << " = t" << u << ";\n";
      					}
      				}
        			break;
        		}	
        		default: {
        			break;
        		}
        		}
      		}
    	}
  	}
  	/*
  	for (unsigned int i = 0; i != vertices.size(); ++i) {
    	vertex u = vertices[i];
    	if (g.info(u).op == output && g.info(u).t.k == scalar) {
    		vertex pred = g.inv_adj(u)[0];
    		
    		// in the single core case if pred is a sumto it is pointing to the output node
    		// any way so this should be skipped
    		if (g.info(pred).op == sumto)
    			continue;
      		
      		string p_label = g.info(pred).label == "" ? "t" + boost::lexical_cast<string>(pred) 
      			: g.info(pred).label;
      		out << g.info(u).label << " = " << p_label << ";" << std::endl;
    	}
  	}
	 */
	out << toFree;
	out << "}\n";
  	//std::cerr << "finished translating graph" << std::endl;
	return "";
}

void translate_to_intrin(ostream& out,
		    string name,
		    map<string,type*>const& inputs,
		    map<string,type*>const& outputs, 
		    graph& g)
{
	string toFree = "";
	// change all vertex sizes from $$* to s*
	// have to touch all levels of a type because functions like get_next_element use
	// lower levels of a given type
	for (int i = 0; i != g.num_vertices(); i++) {
		type *t = &g.info(i).t;
		while (t) {
			string &s = t->dim.dim;
			if (s.find("$$") == 0) {
				s.erase(0,2);
				s = "__s" + s;
			}
			t = t->t;
		}
	}
	
	
	// for malloc
	out << "#include <stdlib.h>\n";
	out << "#include <math.h>\n";
	
	// map for function (required by noPtr, empty here)
	map<string, pair<vertex, type> > data;
	
	out << make_function_signature(name,inputs,outputs,data,"",typeWithName,false);
	//out << "void " << name << function_args(inputs,outputs,data,"",typeWithName,false);
    //out << "{\n";
	
    // string of pointers for scalar output
    string ptrOutputLcl;
    string ptrOutputEnd;
    for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
        if (i->second->k == scalar) {
            // create local working scalar value
            ptrOutputLcl += type_to_c(*i->second) + " " + i->first + " = ";
            ptrOutputLcl += "*" + i->first + "_ptr;\n";
            // create store back to argument
            ptrOutputEnd +="*" + i->first + "_ptr = " + i->first + ";\n";
        }
    }

	// local copies of scalar outputs
	out << ptrOutputLcl;
	
	// declare iteration vars
	int maxd = 0;
	check_depth(1,maxd, g.subgraphs);
	if (maxd > 0) {
		out << "int ii";
		for (int i = 1; i <= maxd; ++i)
			out << "," << var_name[i];
		out << ";\n";
	}
	else {
	  	out << "int ii;\n";
	}
	
  	init_partitions(g, g.subgraphs, out);
  	
  	for (unsigned int u = 0; u != g.num_vertices(); ++u) {
    	if (g.find_parent(u) == 0 && (g.adj(u).size() > 0 || g.inv_adj(u).size() > 0)) {
      		toFree += translate_tmp_intrin(out, g, u);
    	}
  	}
  	
	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
	vector<vertex> vertices;
	for (unsigned int i = 0; i < g.num_vertices(); ++i)
		if (g.find_parent(i) == NULL)
			vertices.push_back(i);
	
  	order_subgraphs(order, new_sub, new_old, NULL, vertices, g.subgraphs, g);
	
	for (unsigned int i = 0; i != order.size(); ++i) {
    		map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
		map<vertex,vertex>::iterator iter2 = new_old.find(order[i]);
		if (iter2 != new_old.end()) {
			vertex u = iter2->second;
			if (g.info(u).op == squareroot) {
				out << "t" << g.inv_adj(u)[0] << " = sqrt(" << expr_of(u,g,NULL) << ");\n";
			}
		}
    		if (iter != new_sub.end()) {
			subgraph* sg = iter->second;
			toFree += translate_graph_intrin(out, sg, sg->vertices, sg->subs, g);
		}
	}

	for (unsigned int i = 0; i != g.num_vertices(); ++i) {
		if (g.find_parent(i) != 0) 
			continue;
    	vertex u = i;
    	if (g.info(u).op == output && g.info(u).t.k == scalar) {
    		vertex pred = g.inv_adj(u)[0];
      		
      		//string p_label = g.info(pred).label.compare("") == 0 ? "t" + boost::lexical_cast<string>(pred) 
			//: g.info(pred).label;
			//string p_label = "t" + boost::lexical_cast<string>(pred);
      		//out << g.info(u).label << " = " << p_label << ";" << std::endl;
            
            out << g.info(u).label << " = " << expr_of(pred,g,NULL) << ";"
                    << std::endl;
    	}
  	}
	
	// handle any scalar outputs
	out << ptrOutputEnd;
	out << toFree << "\n";
  	out << "}\n";
  	
}
