#include "translate_to_code.hpp"
#include "partition.hpp" // for check_depth
#include "iterator.hpp"

using namespace std;
extern std::string precision_type;
map<string,string> step_mp_noPtr;

void translate_declareVariables_noPtr(ostream& out, graph &g, vertex u, bool parallel, map<vertex, pair<string, string> > &indexMap) {
	string iter = string(1,var_name[depth(g.find_parent(u))]);
	string topName = "";
	bool need_iter = true;
	switch (g.info(u).op) {
		case get_row_from_column:
		case get_column_from_row: {
			vertex pred = g.inv_adj(u)[0];
			if (g.info(pred).op == input)
				topName = g.info(pred).label;
			else
				topName = indexMap[pred].second;
			break;
		}
		case get_row:
		case get_element:
		case get_column: {
			vertex pred = g.inv_adj(u)[0];
			if (g.info(pred).op == input) {
				topName = g.info(pred).label;
			} else {
				topName = indexMap[pred].second;
			}
			break;
		}
		case store_row:
		case store_element: 
		case store_add_element:
		case store_add_column:
		case store_column: {
			vertex succ = g.adj(u)[0];
			if (g.info(succ).op == output)
				topName = g.info(succ).label;
			else
				topName = indexMap[succ].second;
			break;
		}
		case temporary:
			topName = "t"+boost::lexical_cast<string>(u);
			need_iter = false;
			if (parallel)
				break;
			switch (g.info(u).t.k) {
				case row:
				case column:
					out << type_to_noPtr(g.info(u).t, "t"+boost::lexical_cast<string>(u), false) << ";\n";
					break;
				case scalar:
					out << precision_type << " t" << u << ";\n";
					break;
				default:
					break;
			}//switch inside temporary case
			break;
		case sumto: 
		{
			vertex succ = g.adj(u)[0];
			string scl = "t" + boost::lexical_cast<string>(succ);
			//new stuff
			//code coverage:
			/* I think the point here is that the output of the sumto node
			 * may or may not be an output in the total graph.
			 * If it isn't, we need to create a temporary for it.
			 * My question: why don't we have the same situation for the other nodes?
			 */
			if (g.info(succ).op == output) {
				topName = g.info(succ).label;
				scl = g.info(succ).label;
			} else {
				topName = scl;
			}

			// What does this mean?  Why do we care about the height difference of the two nodes?
			if (g.info(u).t.height < g.info(succ).t.height) {
				if (g.info(u).t.k == scalar) {
					out << precision_type << " t" << u << " = " << scl;
					if (g.info(succ).t.k != scalar) {
						out << "[" << iter << "]";
					}
					out << ";\n";
				}
				/*else {
					out << precision_type << "* t" << u << " = " << scl
						<< " + " << iter << get_next_elem(g.info(succ).t) << ";\n";
				}*/
			}
			else {
				if (g.info(u).t.k == scalar) {
					if (depth(g.find_parent(u)) > depth(g.find_parent(succ))) {
						out << precision_type << " *t" << u << " = " << scl;
					} else {
						out << precision_type << " t" << u;
					}
					out << ";\n";
				}
				else {
					if (depth(g.find_parent(u)) > depth(g.find_parent(succ))) {
						if (parallel) {
							out << precision_type << " *t" << u << " = " << scl 
								<< "+disp*" << container_size(u,g) << ";\n";
						} else {
							out << precision_type << " *t" << u << " = " << scl << ";\n";
						}
					} else {
						out << type_to_noPtr(g.info(u).t, "t"+boost::lexical_cast<string>(u), false) << ";\n";
					}
				}
			}

			if (!been_cleared(u,g,0)) {
				if (g.info(u).t.k == scalar) {
					need_iter = false; //scalar, so we don't need iter
					out << "t" + boost::lexical_cast<string>(u) 
						<< " = 0.0;\n";
				}
				else {
					need_iter = true;
					string cs = container_size(u,g);
					size_t pos = cs.find("__s",0);
					while (pos != string::npos) {
						cs.replace(pos,3,"__m");
						pos = cs.find("__s",0);
					}
					out << "for (ii = 0; ii < " << cs << "; ++ii)\n";
					//out << "t" + boost::lexical_cast<string>(u) << "[ii] = 0.0;\n";
					if (topName != "") {
						out << topName << "[" << iter << "][ii] = 0.0;\n";
					} else {
						out << indexMap[succ].second << "[" << iter << "][ii] = 0.0;\n";
					}
				}
			}
			break;
		}
		default:
			break;
	}
	if (topName != "") {
		if (need_iter) {
			indexMap[u] = make_pair(indexMap[u].first + "[" + iter + "]", topName);
		} else {
			indexMap[u] = make_pair(indexMap[u].first, topName);
		}
	}
}

void translate_tmp_noPtr(ostream& out, graph &g, vertex u, map<vertex, pair<string,string> > &indexMap) {
	string iter = string(1,var_name[depth(g.find_parent(u))]);
	string topname = "";
	// create temporary space
	switch (g.info(u).op) {
		case temporary:
			topname = "t"+boost::lexical_cast<string>(u);
			switch (g.info(u).t.k) {
				case row:
				case column:
					out << type_to_noPtr(g.info(u).t, "t"+boost::lexical_cast<string>(u), false) << ";\n";
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
					topname = "t"+boost::lexical_cast<string>(u);
					vertex succ = g.adj(u)[0];
					string scl = "t" + boost::lexical_cast<string>(succ);
					if (g.info(succ).op == output)
						scl = g.info(succ).label;

					int d = (depth(g.find_parent(u)) - depth(g.find_parent(succ)));
					if (d == 0) {
						// just point to
						if (g.info(u).t.k == scalar)
							out << precision_type << " t" << u << " = " << scl << ";\n";
						else
							out << precision_type << " *t" << u << " = " << scl << ";\n";
					}
					else if (d < 0) {
						// treat as temporary
						if (g.info(u).t.k == scalar) {
							//out << precision_type << " t" << u << "[1];\n";
							out << precision_type << " t" << u << ";\n";
							}
							else {
								out << type_to_noPtr(g.info(u).t, "t"+boost::lexical_cast<string>(u), false) << ";\n";
							}
						}
						else {
							// if the adjacent node is an output, we can use the output space that exists
							// aready so do nothing.Pointing to another sumto should have the space
							// already allocated, but this check may need to be improved.

							// For other ops
							// parallel has shown other cases in the dispatch loops that are working
							// correctly for now.In these cases, it may be correct to allocate
							// this space here.Need to find a case for this.

							//}
						}

					if (g.info(u).t.k == scalar) {
						out << "t" + boost::lexical_cast<string>(u) << " = 0.0;\n";
					}
					else {
						out << "for (ii = 0; ii < " << container_size(u, g) << "; ++ii)\n";
						out << "t" + boost::lexical_cast<string>(u) << "[ii] = 0.0; //in translate_tmp\n";
						}
						//indexMap[u] = make_pair("t"+ boost::lexical_cast<string>(u), itr);
						break;
					}
		case input:
		case output:
				break;
		default: {
				 break;
			 }
	}
	if (topname != "") {
		indexMap[u] = make_pair(indexMap[u].first, topname);
	}
}

void translate_graph_noPtr(ostream& out,
		subgraph* current,
		vector<vertex> const& vertices,
		vector<subgraph*> const& subgraphs,
		graph& g,
		std::map<vertex,std::pair<string,string> > &indexMap)
{

    // get correct loop based on subgraph iterator details 
    out << current->sg_iterator.getSerialCLoop(current);

	//std::cerr << "starting graph translation" << std::endl;
	// topologically sort the subgraphs
	deque<vertex> order;
	map<vertex,subgraph*> new_sub;
	map<vertex,vertex> new_old;
	order_subgraphs(order, new_sub, new_old, current, vertices, subgraphs, g);

	//std::cerr << "declaring variables" << std::endl;

	// Declare variables	
	for (unsigned int i = 0; i != order.size(); ++i) {
		map<vertex,vertex>::iterator iter = new_old.find(order[i]);
		if (iter != new_old.end()) {
			vertex u = iter->second;
			translate_declareVariables_noPtr(out, g, u, false, indexMap);
		}//if
	}//for
	//std::cerr << "finished declaring variables" << std::endl;

	// Do computations and store the results
	for (unsigned int i = 0; i != order.size(); ++i) {
		map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
		if (iter != new_sub.end()) {
			subgraph* sg = iter->second;
			translate_graph_noPtr(out, sg, sg->vertices, sg->subs, g, indexMap);
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

							map<vertex,pair<string,string> >::iterator index_found = indexMap.find(succ);
							if (index_found != indexMap.end()) {
								out << indexMap[succ].second << indexMap[succ].first + "[" + itr << "] = " 
									<< expr_of_noPtr(g.inv_adj(u)[0], g, current, indexMap)
									<< "; //accessing indexMap[" << succ << "]\n";
							} else {
								if (g.info(succ).label.compare("") == 0) {
									std::cerr << "Failing to find in indexMap, unexpected case" << endl;
								} else {
									out << g.info(succ).label << "[" << itr << "] = " 
										<< expr_of_noPtr(g.inv_adj(u)[0], g, current, indexMap)
										<< "; //accessing indexMap[" << succ << "]\n";
								}
							}
							//out << "*t" << u << " = " << expr_of_noPtr(g.inv_adj(u)[0], g, current) << ";\n";
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

							map<vertex,pair<string,string> >::iterator index_found = indexMap.find(succ);
							if (index_found != indexMap.end()) {
								out << indexMap[succ].second << indexMap[succ].first + "[" + itr << "] += " 
									<< expr_of_noPtr(g.inv_adj(u)[0], g, current, indexMap)
									<< "; //accessing indexMap[" << succ << "]\n";
							} else {
								if (g.info(succ).label.compare("") == 0) {
									std::cerr << "Failing to find in indexMap, unexpected case" << endl;
								} else if (g.info(succ).label.compare(0, 3,"tmp") == 0) {
									out << "t" + boost::lexical_cast<string>(succ) << "[" << itr << "] += " 
										<< expr_of_noPtr(g.inv_adj(u)[0], g, current, indexMap)
										<< "; //accessing indexMap[" << succ << "]\n";
								} else {
									out << g.info(succ).label << "[" << itr << "] += " 
										<< expr_of_noPtr(g.inv_adj(u)[0], g, current, indexMap)
										<< "; //accessing indexMap[" << succ << "]\n";
								}
							}
							//out << "*t" << u << " += " << expr_of_noPtr(g.inv_adj(u)[0], g, current) << ";\n";
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
							if (g.info(adj).op == sumto) { 
								out << "t" << adj << " += " 
									<< expr_of_noPtr(u, g, current, indexMap) << ";\n";
							}
						}
						
					default:
						// old summation code, sumto should eliminate the need for this		
						//out << "t" << g.adj(u)[0] << " += " << expr_of_noPtr(u, g, current, indexMap) << ";\n";
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
						if (g.info(succ).op == output)
							scl = g.info(succ).label;

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
out << "}\n";
//std::cerr << "finished translating graph" << std::endl;
}

void translate_to_noPtr(ostream& out,
		string name,
		map<string,type*>const& inputs,
		map<string,type*>const& outputs, 
		graph& g)
{
	std::map<vertex, std::pair<string,string> > indexMap;

	// get all of the top level data into the map
	for (int i = 0; i < g.num_vertices(); ++i) {
		if (g.find_parent(i) == 0) {
			if (g.info(i).op == input || g.info(i).op == output)  {
				indexMap[i] = make_pair("",g.info(i).label);
			} else if (g.info(i).op == temporary) {
				indexMap[i] = make_pair("","t"+boost::lexical_cast<string>(i));
			}
		}
	}
	
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

	step_mp_noPtr.clear();

	// for malloc
	out << "#include <stdlib.h>\n";

	//dummy map for function
	map<string,pair<vertex,type> > data; 

	out << make_function_signature(name,inputs,outputs,data,"",typeWithName,true);
	//out << "void " << name << function_args(inputs,outputs,data,"",typeWithName,true);
	//out << "{\n";

	// string of pointers for scalar output
	string ptrOutputLcl;
	string ptrOutputEnd;

	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) { 		
		if (i->second->k == scalar) {
			// create local working scalar value
			ptrOutputLcl += type_to_noPtr(*i->second, i->first)  + " = ";
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
			translate_tmp_noPtr(out, g, u, indexMap);
		}
	}

	for (int i = 0; i != g.subgraphs.size(); i++) {
		subgraph *sg = g.subgraphs[i];
		translate_graph_noPtr(out, sg, sg->vertices, sg->subs, g, indexMap);
	}
	/*
	std::map<vertex, std::pair<string,string> >::iterator i;
	for (i=indexMap.begin(); i!=indexMap.end(); ++i) {
		std::cout << "map[" << i->first << "] = " << i->second.first <<  ", " << i->second.second  << "\n";
	}
	*/


	for (unsigned int i = 0; i != g.num_vertices(); ++i) {
		if (g.find_parent(i) != 0) 
			continue;
		vertex u = i;
		if (g.info(u).op == output && g.info(u).t.k == scalar) {
			vertex pred = g.inv_adj(u)[0];

			//string p_label = g.info(pred).label == "" ? "t" + boost::lexical_cast<string>(pred) 
			//: g.info(pred).label;
			string p_label = "t" + boost::lexical_cast<string>(pred);
			out << g.info(u).label << " = " << p_label << ";" << std::endl;
		}
	}

	// handle any scalar outputs
	out << ptrOutputEnd;
	out << "}\n";

}
