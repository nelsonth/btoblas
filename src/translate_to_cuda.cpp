
#include "translate_to_code.hpp"

using namespace std;
vector<string> index_ar;
vector<string> step;
extern std::string precision_type;

void calc_index(vertex u, ostream &out, graph &g) {
	switch (g.info(u).op) {
		case get_row:
		case get_column: {
				vertex pred = g.inv_adj(u)[0];
				string prl = g.info(pred).label;
				if (prl.compare("") == 0 || g.info(pred).op == get_row 
						|| g.info(pred).op == get_column)
					prl = "t" + boost::lexical_cast<string>(pred);
				out << precision_type << "* t" << u << " = " << prl
				<< " + " << index_ar[depth(g.find_parent(u))]
				<< get_next_elem(g.info(pred).t) << "*" << g.info(pred).t.dim.step << ";\n";  
				break;
		}
		case store_row:
		case store_column: {
				vertex succ = g.adj(u)[0];
				string scl = g.info(succ).label;
				if (scl.compare("") == 0 || scl.find("tmp") == 0)
					scl = "t" + boost::lexical_cast<string>(succ);
				out << precision_type << "* t" << u << " = " << scl
				<< " + " << index_ar[depth(g.find_parent(u))] 
				<< get_next_elem(g.info(succ).t) << "*" << g.info(succ).t.dim.step << ";\n"; 
				break;
		}
		case get_element: {
				vertex pred = g.inv_adj(u)[0];
				out << precision_type << " t" << u << " = " 
					<< expr_of(pred,g,g.find_parent(u)) << "[" << index_ar[depth(g.find_parent(u))]
					<< "];\n";
				break;
		}
		case store_element:
		case store_add_element: 
			{
				string target;
				for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
				vertex v = g.adj(u)[i];
				switch (g.info(v).op) {
					case store_row:
					case store_column:
					case temporary:
  						target = "t" + boost::lexical_cast<string>(v);
			            goto end;
			            break;
		            case output:
		              	target = g.info(v).label;
		              	goto end;
		              	break;
		            default:
		              	break;
				}
		    }
			end:
	        out << precision_type << "& t" << u << " = " << target << "[" 
	        	<< index_ar[depth(g.find_parent(u))] << "];\n";
	        break;
		}
		case temporary:
				switch (g.info(u).t.k) {
					case row:
					case column:
					out << precision_type << "* t" << u << " = new " 
						<< precision_type << "[" << container_size(u, g) << "];\n";
					break;
					case scalar:
		            out << precision_type << " t" << u << ";\n";
		            break;
	        	default:
	        	 	break;
				}//switch inside temporary case
				break;
		default:
			//std::cout << u << "\n";
			//std::cout << "WARNING: translate_to_cuda.cpp: calc_index(): unexpected type\n";
			break;
	}//switch	
}

void translate_cuda_loops(ostream& out,
		     subgraph* current,
		     vector<vertex> const& vertices,
		     vector<subgraph*> const& subgraphs,
		     map<vertex,string> &sumto,
		     graph& g)
{
    for (unsigned int j = 0; j != current->summations.size(); ++j) {
        vertex u = current->summations[j];
        switch (g.info(u).t.k) {
        	case scalar:
          		out << precision_type << " t" << u << " = 0;\n" ;
          		break;
        	case row:
        	case column: {
		        out << "for (int ii = 0; ii < " << container_size(u, g) << "; ++ii)\n";
				string u_label = g.info(u).op == output ? g.info(u).label : "t" 
					+ boost::lexical_cast<string>(u);
          		out << u_label << "[ii] = 0.0;\n";
          		break;
			}
        	default:
          		break;
        }
	}
	// create loop body for this subgraph
	// OpenMP expects the loop test to use < instead of !=.
	// The Intel compiler doesn't like loop indices of type unsigned int.
	string vn = index_ar[depth(current)];
	out << "for (int " << vn << " = 0;";
	out << vn << " < " << current->iterations
	<< "; " << vn << "++) {\n";
		
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
      			
      		if (g.adj(u).size() > 0) {
      			switch (g.info(u).op) {
      				case get_row:
      				case get_column: {
      						vertex pred = g.inv_adj(u)[0];
      						string prl = g.info(pred).label;
      						if (prl.compare("") == 0 || g.info(pred).op == get_row 
      								|| g.info(pred).op == get_column)
      							prl = "t" + boost::lexical_cast<string>(pred);
      						out << precision_type << "* t" << u << " = " << prl
      						<< " + " << index_ar[depth(g.find_parent(u))]
      						<< get_next_elem(g.info(pred).t) << "*" << g.info(pred).t.dim.step << ";\n";  
      						break;
      				}
      				case store_row:
      				case store_column: {
      						vertex succ = g.adj(u)[0];
      						string scl = g.info(succ).label;
      						if (scl.compare("") == 0 || scl.find("tmp") == 0)
      							scl = "t" + boost::lexical_cast<string>(succ);
      						if (sumto.find(succ) == sumto.end()) {
	      						out << precision_type << "* t" << u << " = " << scl
	      						<< " + " << index_ar[depth(g.find_parent(u))] 
	      						<< get_next_elem(g.info(succ).t) << "*" 
	      						<< g.info(succ).t.dim.step << ";\n";
      						}
      						else {
      							out << precision_type << "* t" << u << " = s" << scl
		      						<< " + " << index_ar[depth(g.find_parent(u))] 
		      						<< get_next_elem(g.info(succ).t) << "*" 
		      						<< g.info(succ).t.dim.step << ";\n";
      						}
      						break;
      				}
      				case get_element: {
      						vertex pred = g.inv_adj(u)[0];
      						out << precision_type << " t" << u << " = " 
      							<< expr_of(pred,g,g.find_parent(u)) << "[" << index_ar[depth(g.find_parent(u))]
      							<< "];\n";
      						break;
      				}
      				case store_element:
      				case store_add_element: 
      					{
      						string target;
      						vertex v;
      						for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
      						v = g.adj(u)[i];
      						switch (g.info(v).op) {
      							case store_row:
      							case store_column:
      							case temporary:
      		  						target = "t" + boost::lexical_cast<string>(v);
      					            goto end;
      					            break;
      				            case output:
      				              	target = g.info(v).label;
      				              	goto end;
      				              	break;
      				            default:
      				              	break;
      						}
      				    }
      					end:
      					if (sumto.find(v) == sumto.end()) {
	      			        out << precision_type << "& t" << u << " = " << target << "[" 
	      			        	<< index_ar[depth(g.find_parent(u))] << "];\n";
      					}
      					else {
	      			        out << precision_type << "& t" << u << " = st" << v << "[" 
	      			        	<< index_ar[depth(g.find_parent(u))] << "];\n";
      					}
      							
      			        break;
      				}
      				case temporary:
      						switch (g.info(u).t.k) {
      							case row:
      							case column:
      							out << precision_type << "* t" << u << " = new " 
      								<< precision_type << "[" << container_size(u, g) << "];\n";
      							break;
      							case scalar:
      				            out << precision_type << " t" << u << ";\n";
      				            break;
      			        	default:
      			        	 	break;
      						}//switch inside temporary case
      						break;
      				default:
      					//std::cout << u << "\n";
      					//std::cout << "WARNING: translate_to_cuda.cpp: calc_index(): unexpected type\n";
      					break;
      			}//switch
      		}//if
    	}//if
  	}//for
  	//std::cerr << "finished declaring variables" << std::endl;

  	// Do computations and store the results
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter != new_sub.end()) {
	    	subgraph* sg = iter->second;          		
      		translate_cuda_loops(out, sg, sg->vertices, sg->subs, sumto, g);       
    	} 
    	else {
      		map<vertex,vertex>::iterator iter = new_old.find(order[i]);
      		if (iter != new_old.end()) {
        		vertex u = iter->second;
        		switch (g.info(u).op) {
        			case store_element:
          				if (g.inv_adj(u).size() == 1)
            				out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				break;
        			case store_add_element: {
        					subgraph *sg = g.find_parent(u)->parent;
        					if (depth(sg) < 5) {
        						out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) 
        							<< ";\n";       						
        					}
        					else {
        						out << "t" << u << " += " << expr_of(g.inv_adj(u)[0], g, current) 
        							<< ";\n";
        					}
          				break;
        			}
        			default:
          				if (g.adj(u).size() > 0 && g.find_parent(u) 
          						&& std::find(g.find_parent(u)->summations.begin(),
                           		g.find_parent(u)->summations.end(),u) 
                           		!= g.find_parent(u)->summations.end()) {
          					out << "t" << u << " += ";
          					out << expr_of(u, g, current) << ";\n";
          				}
          				break;
        		}
      		}
    	}
	}

  	for (unsigned int i = 0; i != vertices.size(); ++i) {
    	vertex u = vertices[i];
    	if (g.info(u).op == output && g.info(u).t.k == scalar) {
      		vertex pred = g.inv_adj(u)[0];
      		string p_label = g.info(pred).label == "" ? "t" + boost::lexical_cast<string>(pred) 
      			: g.info(pred).label;
      		out << g.info(u).label << " = " << p_label << ";" << std::endl;
    	}
  	}
	
  	out << "}\n";
  	//std::cerr << "finished translating graph" << std::endl;
}

int ruid = 0;
void translate_cuda_block(ostream& out,
		     subgraph* current,
		     vector<vertex> const& vertices,
		     vector<subgraph*> const& subgraphs,
		     map<vertex,string> &sumto,
		     graph& g)
{
  	//std::cerr << "starting graph translation" << std::endl;
  	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
  	order_subgraphs(order, new_sub, new_old, current, vertices, subgraphs, g);
  	//std::cerr << "declaring variables" << std::endl;
  	
  	for (int v = 0; v != current->summations.size(); v++) {
  		vertex u = current->summations[v];
  		if (g.info(u).op == deleted) {
  			std::cout << "WARNING: translate_to_cuda.cpp: tranlate_cuda_block(): deleted sum\n";
  			continue;
  		}
  		try {
  			int sz = boost::lexical_cast<int>(sumto[u]);
  			out << "__shared__ " << precision_type << " stm" << u << "[";
  			out << sz;
  			out << "];\n";
        }
        catch(boost::bad_lexical_cast &) {
            out << precision_type << " *stm" << u << " = r" << u << "+ bx *" 
            	<< sumto[u] << "+ by * gxsize * " << sumto[u] << ";\n";
        }

		int op_sum = depth(current) == 3 ? 4 : 3;
		string od = depth(current) == 4 ? "blockDim.x" : "blockDim.y";
		out << precision_type << " *st" << u << " = stm" << u << " + "
			<< index_ar[depth(current)] << "*" << od;
		
		if (depth(g.find_parent(g.inv_adj(u)[0])) > 4 && g.info(u).t.k != scalar)
			out << "*" << g.info(u).t.dim.dim;
		
		if (depth(current) == 3) {
			out << ";\n";
		}
		else {
			if (g.info(u).t.k != scalar)
				out << " + " << index_ar[3] << " * " << g.info(u).t.dim.dim << ";\n";
			else {
				out << ";\n";
				out << precision_type << " &t" << u << " = st" << u << "[" 
				  	<< index_ar[op_sum] << "];\n";
			}
		}
  	}

  	// Declare variables
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,vertex>::iterator iter = new_old.find(order[i]);
    	if (iter != new_old.end()) {
      		vertex u = iter->second;
      			
      		if (g.adj(u).size() > 0) {
      			switch (g.info(u).op) {
      				case get_row:
      				case get_column: {
      						vertex pred = g.inv_adj(u)[0];
      						string prl = g.info(pred).label;
      						if (prl.compare("") == 0 || g.info(pred).op == get_row 
      								|| g.info(pred).op == get_column)
      							prl = "t" + boost::lexical_cast<string>(pred);
      						out << precision_type << "* t" << u << " = " << prl
      						<< " + " << index_ar[depth(g.find_parent(u))]
      						<< get_next_elem(g.info(pred).t) << "*" << g.info(pred).t.dim.step << ";\n";  
      						break;
      				}
      				case store_row:
      				case store_column: {
      						vertex succ = g.adj(u)[0];
      						string scl = g.info(succ).label;
      						if (scl.compare("") == 0 || scl.find("tmp") == 0)
      							scl = "t" + boost::lexical_cast<string>(succ);
      						if (sumto.find(succ) != sumto.end()) { 
      							out << precision_type << "* t" << u << " = st" << succ
		      						<< " + " << index_ar[depth(g.find_parent(u))] 
		      						<< get_next_elem(g.info(succ).t) << "*" 
		      						<< g.info(succ).t.dim.step << ";\n";
      						}
      						else {
          						out << precision_type << "* t" << u << " = " << scl
    	      						<< " + " << index_ar[depth(g.find_parent(u))] 
    	      						<< get_next_elem(g.info(succ).t) << "*" 
    	      						<< g.info(succ).t.dim.step << ";\n";
      						}
      						break;
      				}
      				case get_element: {
      						vertex pred = g.inv_adj(u)[0];
      						out << precision_type << " t" << u << " = " 
      							<< expr_of(pred,g,g.find_parent(u)) << "[" << index_ar[depth(g.find_parent(u))]
      							<< "];\n";
      						break;
      				}
      				case store_element:
      				case store_add_element: 
      					{
      						vertex v;
      						string target;
      						for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
      						v = g.adj(u)[i];
      						switch (g.info(v).op) {
      							case store_row:
      							case store_column:
      							case temporary:
      		  						target = "t" + boost::lexical_cast<string>(v);
      					            goto end;
      					            break;
      				            case output:
      				              	target = g.info(v).label;
      				              	goto end;
      				              	break;
      				            default:
      				              	break;
      						}
      				    }
      					end:
      					if (sumto.find(v) == sumto.end()) {
	      			        out << precision_type << "& t" << u << " = " << target << "[" 
	      			        	<< index_ar[depth(g.find_parent(u))] << "];\n";
      					}
  						else {
  							out << precision_type << "& t" << u << " = s" << target << "[" 
	      			        	<<  index_ar[depth(current)] << "];\n";
  						}

      			        break;
      				}
      				case temporary:
      						switch (g.info(u).t.k) {
      							case row:
      							case column:
      							out << precision_type << "* t" << u << " = new " 
      								<< precision_type << "[" << container_size(u, g) << "];\n";
      							break;
      							case scalar:
      				            out << precision_type << " t" << u << ";\n";
      				            break;
      			        	default:
      			        	 	break;
      						}//switch inside temporary case
      						break;
      				default:
      					//std::cout << u << "\n";
      					//std::cout << "WARNING: translate_to_cuda.cpp: calc_index(): unexpected type\n";
      					break;
      			}//switch	
      		}//if
    	}//if
  	}//for
  	//std::cerr << "finished declaring variables" << std::endl;

  	// Do computations and store the results
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter != new_sub.end()) {
	    	subgraph* sg = iter->second;
	    	

		    switch (depth(sg)) {
		    	case 0:
		    	case 1:
		    	case 2:
		    		std::cout << "translate_to_cuda.cpp: translate_cuda_block(): unexpected graph\n";
		    		break;
		    	case 3:
		    	case 4:
		    		translate_cuda_block(out, sg, sg->vertices, sg->subs, sumto, g);
		    		break;
		    	default:
		    		translate_cuda_loops(out, sg, sg->vertices, sg->subs, sumto, g);
		    		break;
		    }

       
    	} 
    	else {
      		map<vertex,vertex>::iterator iter = new_old.find(order[i]);
      		if (iter != new_old.end()) {
        		vertex u = iter->second;
        		switch (g.info(u).op) {
        			case store_element:
          				if (g.inv_adj(u).size() == 1)
            				//out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          					//out << "t" << u << " = " << "st" << g.inv_adj(u)[0] << "[0];\n";
          				break;
        			case store_add_element:        				
          				out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
        				//out << precision_type << " t" << u << " = " 
        				//	<< expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				break;
        			default:
          				if (g.adj(u).size() > 0 && g.find_parent(u) 
          						&& std::find(g.find_parent(u)->summations.begin(),
                           		g.find_parent(u)->summations.end(),u) 
                           		!= g.find_parent(u)->summations.end()) {
          					out << /*precision_type <<*/ " t" << u << " = ";
          					out << expr_of(u, g, current) << ";\n";
          				}
          				break;
        		}
      		}
    	}
	}
  	
  	for (int i = 0; i != current->summations.size(); i++) {
  		vertex u = current->summations[i];
  		if (g.info(u).op == deleted) {
  			std::cout << "WARNING: translate_to_cuda.cpp: translate_cuda_block(): deleted sum\n";
  			continue;
  		}
  		// this may be wrong
  		vertex succ;
  		if (g.adj(u).size() == 0)
  			succ = u;
  		else
  			succ = g.adj(u)[0];
		string opu = depth(current) == 3 ? index_ar[4] : index_ar[3];
		string limit = depth(current) == 3 ? "blockDim.x" : "blockDim.y";
		string opudim = depth(current) == 3 ? "blockDim.y" : "blockDim.x";
		string iter = index_ar[depth(g.find_parent(g.inv_adj(u)[0]))];
		string label;
		if (g.info(u).t.k == scalar) {
			label = "t" + boost::lexical_cast<string>(succ);
		}
		else {
			label = "t" + boost::lexical_cast<string>(u) + "[" + iter + "]";
		}
		
		out << "if (" << index_ar[depth(current)] << "== 0) {\n";
		
		vertex pred = g.inv_adj(u)[0];
		bool loop = false;
		if (depth(g.find_parent(pred)) > 4 && g.info(u).t.k != scalar) {
  			out << "for (int " << iter << "=0;" << iter << "!=" << g.info(u).t.dim.dim
  				<< ";" << iter << "++) {\n";
  			loop = true;
		}
		
		out << "__syncthreads();\n";
		out << label << " = 0;\n"
			<< precision_type << " *str" << u << " = stm" << u << " + " << opu;
		if (loop)
			out << "*" << g.info(u).t.dim.dim;
		out	<< ";\n"
			<< "for (int s = 0; s != " << limit << "; s++) {\n";
		if (loop) {
			out << label << " += str" << u << "[" << iter << "];\n";
			out	<< "str" << u << " += " << opudim << "*" << g.info(u).t.dim.dim << ";\n";
		}
		else {
			out	<< label << " += str" << u << "[0];\n";
			out	<< "str" << u << " += " << opudim << ";\n";
		}
		out	<< "}\n";
  		
  		if (loop) {
  			out << "}\n";
  		}
  		//end if
  		out << "}\n";

  	}

  	for (unsigned int i = 0; i != vertices.size(); ++i) {
    	vertex u = vertices[i];
    	if (g.info(u).op == store_element && g.info(u).t.k == scalar) {
    		if (sumto.find(g.inv_adj(u)[0]) == sumto.end())
    			out << "t" << u << " = t" << g.inv_adj(u)[0] << ";\n";
    	}
    	
    	if (g.info(u).op == output && g.info(u).t.k == scalar) {
      		vertex pred = g.inv_adj(u)[0];
      		string p_label = g.info(pred).label == "" ? "t" + pred 
      			: g.info(pred).label;
      		out << g.info(u).label << " = " << p_label << ";" << std::endl;
    	}
  	}
  	
  	//std::cerr << "finished translating graph" << std::endl;
}

void translate_cuda_grid(ostream& out,
		     subgraph* current,
		     vector<vertex> const& vertices,
		     vector<subgraph*> const& subgraphs,
		     map<vertex,string> &sumto,
		     graph& g)
{
  	//std::cerr << "starting graph translation" << std::endl;
  	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
  	order_subgraphs(order, new_sub, new_old, current, vertices, subgraphs, g);
  	//std::cerr << "declaring variables" << std::endl;
  	
  	for (int v = 0; v != current->summations.size(); v++) {
  		vertex u = current->summations[v];
		
        switch (g.info(u).t.k) {
        	case scalar:
        		out << precision_type << " &t" << u << " = st" << u << "[" 
        			<< index_ar[depth(current)] << "];\n";
          		break;
        	case row:
        	case column: {
        		if (depth(current) == 1) {
	        		out << precision_type << " *st" << u << " = red_x + "
	        			<< index_ar[depth(current)] << "*" << g.info(u).t.dim.dim;
	        		type *t = &g.info(u).t;
	        		kind k = t->k;
	        		t = t->t;
	        		while (t) {
	        			if (t->k != k && t->k != scalar) {
	        				out << "*" << t->dim.dim;
	        				break;
	        			}
	        			t = t->t;
	        		}
	        		
	        		out << ";\n";
        		}
        		else {
        			if (g.adj(u).size() != 0) {
		        		out << precision_type << " *st" << u << " = red_x + bx * "
		        			<< g.info(u).t.dim.dim << " + " << index_ar[depth(current)] << "*"
		        			<< g.info(g.adj(u)[0]).t.dim.dim;
        			}
        			else {
        				std::cout << "FINISH THIS: translate_to_cuda.cpp: translate_cuda_grid():"
        							<< " reduction in bx and by\n"; 
        				out << "reduction\n";
        			}
	        		
	        		out << ";\n";
        		}
          		break;
			}
        	default:
          		break;
        }
  	}

  	// Declare variables
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,vertex>::iterator iter = new_old.find(order[i]);
    	if (iter != new_old.end()) {
      		vertex u = iter->second;
      			
      		if (g.adj(u).size() > 0) {	
      			switch (g.info(u).op) {
      				case get_row:
      				case get_column: {
      						vertex pred = g.inv_adj(u)[0];
      						string prl = g.info(pred).label;
      						if (prl.compare("") == 0 || g.info(pred).op == get_row 
      								|| g.info(pred).op == get_column)
      							prl = "t" + boost::lexical_cast<string>(pred);
      						out << precision_type << "* t" << u << " = " << prl
      						<< " + " << index_ar[depth(g.find_parent(u))]
      						<< get_next_elem(g.info(pred).t) << "*" << g.info(pred).t.dim.step << ";\n";  
      						break;
      				}

      				case store_row:
      				case store_column: {
      						vertex succ = g.adj(u)[0];
      						string scl = g.info(succ).label;
      						if (scl.compare("") == 0 || scl.find("tmp") == 0)
      							scl = "t" + boost::lexical_cast<string>(succ);
      						if (sumto.find(succ) != sumto.end()) {
      							out << precision_type << "* t" << u << " = st" << succ
		      						<< " + " << index_ar[depth(g.find_parent(u))] 
		      						<< get_next_elem(g.info(succ).t) << "*" << g.info(succ).t.dim.step 
		      						<< ";\n"; 
      						}
      						else {
		      						out << precision_type << "* t" << u << " = " << scl
		      						<< " + " << index_ar[depth(g.find_parent(u))] 
		      						<< get_next_elem(g.info(succ).t) << "*" << g.info(succ).t.dim.step 
		      						<< ";\n"; 
	      						
      						}
      						break;
      				}
      				case get_element: {
      						vertex pred = g.inv_adj(u)[0];
      						out << precision_type << " t" << u << " = " 
      							<< expr_of(pred,g,g.find_parent(u)) << "[" << index_ar[depth(g.find_parent(u))]
      							<< "];\n";
      						break;
      				}
      				case store_element:
      				case store_add_element: 
      					{
      						string target;
      						for (unsigned int i = 0; i != g.adj(u).size(); ++i) {
      						vertex v = g.adj(u)[i];
      						switch (g.info(v).op) {
      							case store_row:
      							case store_column:
      							case temporary:
      		  						target = "t" + boost::lexical_cast<string>(v);
      					            goto end;
      					            break;
      				            case output:
      				              	target = g.info(v).label;
      				              	goto end;
      				              	break;
      				            default:
      				              	break;
      						}
      				    }
      					end:
      			        out << precision_type << "& t" << u << " = " << target << "[" 
      			        	<< index_ar[depth(g.find_parent(u))] << "];\n";
      			        break;
      				}
      				case temporary:
      						switch (g.info(u).t.k) {
      							case row:
      							case column:
      							out << precision_type << "* t" << u << " = new " 
      								<< precision_type << "[" << container_size(u, g) << "];\n";
      							break;
      							case scalar:
      				            out << precision_type << " t" << u << ";\n";
      				            break;
      			        	default:
      			        	 	break;
      						}//switch inside temporary case
      						break;
      				default:
      					//std::cout << u << "\n";
      					//std::cout << "WARNING: translate_to_cuda.cpp: translate_cuda_grid(): unexpected type\n";
      					break;
      			}//switch
      		}//if
    	}//if
  	}//for
  	//std::cerr << "finished declaring variables" << std::endl;

  	// Do computations and store the results
  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter != new_sub.end()) {
	    	subgraph* sg = iter->second;
	    	
		    switch (depth(sg)) {
		    	case 0:
		    	case 1:
		    	case 2:
		    		translate_cuda_grid(out, sg, sg->vertices, sg->subs, sumto, g);
		    		break;
		    	case 3:
		    	case 4:
		    		translate_cuda_block(out, sg, sg->vertices, sg->subs, sumto, g);
		    		break;
		    	default:
		    		translate_cuda_loops(out, sg, sg->vertices, sg->subs, sumto, g);
		    		break;
		    }

       
    	} 
    	else {
      		map<vertex,vertex>::iterator iter = new_old.find(order[i]);
      		if (iter != new_old.end()) {
        		vertex u = iter->second;
        		switch (g.info(u).op) {
        			case store_element:
          				if (g.inv_adj(u).size() == 1)
            				out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				break;
        			case store_add_element:        				
          				//out << "t" << u << " = " << expr_of(g.inv_adj(u)[0], g, current) << ";\n";
        				out << precision_type << " t" << u << " = " 
        					<< expr_of(g.inv_adj(u)[0], g, current) << ";\n";
          				break;
        			default:
          				if (g.adj(u).size() > 0 && g.find_parent(u) 
          						&& std::find(g.find_parent(u)->summations.begin(),
                           		g.find_parent(u)->summations.end(),u) 
                           		!= g.find_parent(u)->summations.end()) {
          					out << precision_type << " t" << u << " = ";
          					out << expr_of(u, g, current) << ";\n";
          				}
          				break;
        		}
      		}
    	}
	}

  	for (unsigned int i = 0; i != vertices.size(); ++i) {
    	vertex u = vertices[i];
    	if (g.info(u).op == output && g.info(u).t.k == scalar) {
      		vertex pred = g.inv_adj(u)[0];
      		string p_label = g.info(pred).label == "" ? "t" + pred 
      			: g.info(pred).label;
      		out << g.info(u).label << " = " << p_label << ";" << std::endl;
    	}
  	}	
}

void get_size(type *t, ostream &out, graph &g) {
	out << t->dim.dim;
	type *tt = t;
	while (tt) {
		if (tt->k != t->k) {
			if (tt->k != scalar)
				out << " * " << tt->dim.dim;
			break;
		}
		tt = tt->t;
	}
}

string calc_temp(vertex u, map<string,string> &step_map, subgraph *current, graph &g) { 
	string &d = g.info(u).t.dim.dim;
	if (depth(current) != 3 && depth(current) != 4) {
		return "?";
	}
	
	if (step_map.find(d) == step_map.end() || step_map.find(current->iterations) == step_map.end()
			|| (depth(current) == 4 && step_map.find(current->parent->iterations) == step_map.end())) {
		return d + "*" + current->iterations + "*" + current->parent->iterations;
	}
	int sz = boost::lexical_cast<int>(step_map[d]);
	if (current->iterations.compare("1") != 0) {
		sz *= boost::lexical_cast<int>(step_map[current->iterations]);
	}
	if (depth(current) == 4) {
		if (current->parent->iterations.compare("1") != 0) {
			sz *= boost::lexical_cast<int>(step_map[current->parent->iterations]);
		}
	}
	return boost::lexical_cast<string>(sz);
}	

void find_sum_sizes(vector<subgraph*> &subs, map<vertex,string> &sumto, 
		map<string,string> &step_map, graph &g) {
	// find summations requires reduction across blocks and across threads
	for (int i = 0; i != subs.size(); i++) {
		vector<vertex> &sums = subs[i]->summations;
		for (int j = 0; j != sums.size(); j++) {
			sumto[sums[j]] = calc_temp(sums[j], step_map, subs[i], g);
		}
		
		if (depth(subs[i]) < 4)
			find_sum_sizes(subs[i]->subs, sumto, step_map, g);
	}
}

void find_summation_vertex(vector<subgraph*> &subs, map<vertex,string> &sumto, 
		graph &g) {
	// find summations requires reduction across blocks and across threads
	for (int i = 0; i != subs.size(); i++) {
		vector<vertex> &sums = subs[i]->summations;
		for (int j = 0; j != sums.size(); j++) {
			sumto[sums[j]] = "?";
		}
		
		if (depth(subs[i]) < 4)
			find_summation_vertex(subs[i]->subs, sumto, g);
	}
}

void init_step_map(vector<subgraph*> subs, vector<string> &def_iters, 
		vector<string> &step, map<string, string> &vals) {
	for (int i = 0; i != subs.size(); i++) {
		init_step_map(subs[i]->subs, def_iters,step,vals);
		
		string &s = subs[i]->step;
		if (s.compare("1") == 0)
			continue;
		if (find(def_iters.begin(), def_iters.end(), s) != def_iters.end())
			continue;
		vals[s] = step[depth(subs[i])];
		def_iters.push_back(s);
	}
}

subgraph *find_summation_sg(vertex v, graph &g) {
	if (g.info(v).op == 0)
		return 0;
	vertex u = g.inv_adj(v)[0];
	subgraph *sg = g.find_parent(u);
	while (sg) {
		if (find(sg->summations.begin(), sg->summations.end(), v) != sg->summations.end())
			return sg;
		sg = sg->parent;
	}
	return sg;
}

void translate_to_cuda(ostream& out,
        string name,
        map<string,type*>const& inputs, 
        map<string,type*>const& outputs, 
        graph & g)
{
	out << "#include <stdio.h>\n";
	vector<string> def_iters;
	step.clear();
	for (int i = 0; i != 10; i++)
		step.push_back("1");
  	step.insert(step.begin(),"16");
  	step.insert(step.begin(),"16");
  	step.insert(step.begin(),"0");
  	map<string,string> step_map;
  	step_map["1"] = "1";

  	// find all block and thread level reduction vertices
  	// and find thread level reduction space size
  	map<vertex,string> sumto;
  	find_summation_vertex(g.subgraphs, sumto, g);
  	vector<string>tmp;
  	init_step_map(g.subgraphs, tmp, step, step_map);
  	find_sum_sizes(g.subgraphs, sumto, step_map, g);
  	
  	if (g.subgraphs.size() != 1 || g.subgraphs[0]->subs.size() != 1)
  		std::cout << "translate_to_cuda.cpp: translate_to_cuda(): complex graph at grid level\n";
  	subgraph *blockx = g.subgraphs[0];
  	subgraph *blocky = blockx->subs[0];
  	if (blocky->subs.size() != 1 || blocky->subs[0]->subs.size() != 1)
  		std::cout << "translate_to_cuda.cpp: translate_to_cuda(): complex graph at block level\n";
  		
  	subgraph *thrdx = blocky->subs[0];
  	subgraph *thrdy = thrdx->subs[0];
  	
  	bool reduce = false;
  	if (blockx->summations.size() != 0 || blocky->summations.size() != 0)
  			reduce = true;
  	
  	// build index array (used to specify iterator for loops vs blocks and threads)
  	index_ar.push_back("");
  	index_ar.push_back("bx");
  	index_ar.push_back("by");
  	index_ar.push_back("tx");
  	index_ar.push_back("ty");
  	for (int i = 1;i != var_name.size(); i++) {
  		index_ar.push_back(var_name.substr(i,1));
  	}  	
  	
	// generate device function signature	
  	out << "__global__ void dev_" << name << "(";
  	for (map<string,type*>::const_iterator i = inputs.begin(); i != inputs.end(); ++i) {
    	out << type_to_c(*i->second) << " " << i->first;
    	out << size_params(*i->second);
    	if (boost::next(i) != inputs.end())
      		out << ", ";
  	}
  	
  	if (inputs.begin() != inputs.end())
    	out << ", ";
  
  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
	    out << type_to_c(*i->second, true) << " " << i->first;
	    out << size_params(*i->second);
	    if (boost::next(i) != outputs.end())
      		out << ", ";
  	}
  	if (reduce) {
	  	// temporary reduction space
	  	out << ", " << precision_type << " *red_x, int red_x_sz";
  	}
  	map<vertex,string>::iterator itr = sumto.begin();
  	bool thread_device = false;
  	for (; itr != sumto.end(); itr++) {
  		int d = depth(find_summation_sg(itr->first, g));
  		if (d != 3 && d != 4)
  			continue;
	  	try {
			int sz = boost::lexical_cast<int>(itr->second);
	    }
	    catch(boost::bad_lexical_cast &) {
	        out << ", " << precision_type << " *r" << itr->first;
	        thread_device = true;
	    }
  	}
  	if (thread_device)
  		out << ", int gxsize";
  	
  	out << ") {\n";
  	
  	// assign values to all of the variable step sizes
  	init_partitions(g.subgraphs, out, def_iters, step);
  	
  	// generate device function code
  	out << "\n"
  	        "// Block index\n"
  	        "int bx = blockIdx.x;\n"
  	        "int by = blockIdx.y;\n"
  	        "\n"
  	        "// Thread index\n"
  	        "int tx = threadIdx.x;\n"
  	        "int ty = threadIdx.y;\n"
  	        "\n";

  	// calculate indexes for grid level
  	for (int i = 0; i != g.subgraphs.size(); i++) {
  		subgraph *sg = g.subgraphs[i];
  		translate_cuda_grid(out, sg, sg->vertices, sg->subs, sumto, g);
  	}
  	
  	out << "}\n";
  	
  	
  	
  	
  	
  	bool reduce_3d = false;
  	if (reduce) {
	  	// generate reduce_x (function for reducing across blocks)
	  	out << "__global__ void reduce_x(" << precision_type << " *red_x, int red_x_sz ";
	  	// we need all nodes in top level graph that are outputs or temporaries that are a result
	  	// of blockx subgraph
	  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
		    out << ", " << type_to_c(*i->second, true) << " " << i->first;
		    out << size_params(*i->second);
	  	}
	  	out << ") {\n";
	  	out << "int bx = blockIdx.x;\n";
	  	out << "int by = blockIdx.y;\n";
	  	out << "int tx = threadIdx.x;\n";
	  	out << "int ty = threadIdx.y;\n";
	  	
	  	vertex r;
	  	for (int i = 0; i != blockx->summations.size(); i++) {
	  		calc_index(blockx->summations.size(),out,g);
	  		vertex u = blockx->summations[i];
	  		if (g.info(u).t.dim.base_rows.compare("1") != 0 
	  				&& g.info(u).t.dim.base_cols.compare("1") != 0)
	  			reduce_3d = true;
	  		r = u;
	  	}
	  	for (int i = 0; i != blocky->summations.size(); i++) {
	  		calc_index(blockx->summations.size(),out,g);
	  		vertex u = blocky->summations[i];
	  		if (g.info(u).t.dim.base_rows.compare("1") != 0 
	  		  		&& g.info(u).t.dim.base_cols.compare("1") != 0)
	  		  	reduce_3d = true;
	  		r = u;
	  	}
	  	
	  	if (true) {
	  	//if (reduce_3d) {
	  		/*
	  		vertex p = r;
	  		while (g.find_parent(p) != 0) {
	  			p = g.adj(p)[0];
	  		}
	  		string rlabel = g.info(p).label;

	  		out << precision_type << " *tmp = red_x + bx * blockDim.x;\n";
	  		out << rlabel << " = " << rlabel << " + bx * blockDim.x;\n";
	  		out << rlabel << "[tx] = 0;\n";
	  		out << "for (int i = 0; i != red_x_sz; i++) {\n"
	  			<< rlabel << "[tx] += tmp[tx];\n"
	  			<< "tmp += " << g.info(p).t.dim.base_rows << "*" << g.info(p).t.dim.base_cols << ";\n"
	  			<< "}\n";
	  		*/
	  		
	  		vertex p = r;
	  		while (g.find_parent(p) != 0) {
	  			p = g.adj(p)[0];
	  		}
	  		type *t = &g.info(p).t;
	  		kind kt = t->k;
	  		while (t->k != scalar) {
	  			if (t->k != kt) {
	  				break;
	  			}
	  			t = t->t;
	  		}
	  		string s = t->dim.dim;
	  		string rlabel = g.info(p).label;

	  		out << precision_type << " *tmp = red_x + bx * blockDim.x + by * blockDim.y *"
	  			<< s << "+ ty *" << s << ";\n";
	  		out << rlabel << " = " << rlabel << " + bx * blockDim.x + by * blockDim.y *"
	  			<< s << "+ ty *" << s << ";\n";
	  		out << rlabel << "[tx] = 0;\n";
	  		out << "for (int i = 0; i != red_x_sz; i++) {\n"
	  			<< rlabel << "[tx] += tmp[tx];\n"
	  			<< "tmp += " << g.info(p).t.dim.base_rows << "*" << g.info(p).t.dim.base_cols << ";\n"
	  			<< "}\n";
	  	}
	  	else {
		  	// handle cases larger than thread count
		  	out << precision_type << " t = 0.0;\n"
		  		<< "for (int i = 0; i < red_x_sz; i += blockDim.x) {\n";
		  	// calculate index into temp storage
		  	out << precision_type << " *tmp = red_x + " << index_ar[depth(blockx)] << "* red_x_sz + i;\n";
		  	
		  	out << "int min = i + blockDim.x <= red_x_sz ? blockDim.x : red_x_sz-i;\n";
		  	
		  	// reduction code
			out << "__syncthreads();\n"
				<< "for (int s = 1; s < min; s *= 2) { \n"
				<< "int index = 2 * s * " << "tx" << ";\n"
				<< "if (index < min && index+s < min) {\n"
				<< "tmp" << "[index] += tmp" << "[index + s];\n"
				<< "}\n"
				<< "__syncthreads();\n"
				<< "}\n";
			
			out << "t += tmp[0];\n"
				<< "}\n";
			
			for (int i = 0; i != blockx->summations.size(); i++) {
				vertex v = blockx->summations[i];
				string label = g.info(v).label == "" 
								? "t" + boost::lexical_cast<string>(v)
								: g.info(v).label;
				out << label << "[" << index_ar[depth(blockx)] << "] = t;\n";
			}
			for (int i = 0; i != blocky->summations.size(); i++) {
				vertex v = blocky->summations[i];
				string label = g.info(v).label == "" 
								? "t" + boost::lexical_cast<string>(v)
								: g.info(v).label;
				out << label << "[" << index_ar[depth(blockx)] << "] = t;\n";
			}

	  	}
		out << "}\n";
  	}	
  		
  		
  		
  		
	  	
	
	// generate host function signature
	out << "__host__ void " << name << "(";
  	for (map<string,type*>::const_iterator i = inputs.begin(); i != inputs.end(); ++i) {
    	out << type_to_c(*i->second) << " " << i->first;
    	out << size_params(*i->second);
    	if (boost::next(i) != inputs.end())
      		out << ", ";
  	}
  	
  	if (inputs.begin() != inputs.end())
    	out << ", ";
  
  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
	    out << type_to_c(*i->second, true) << " " << i->first;
	    out << size_params(*i->second);
	    if (boost::next(i) != outputs.end())
      		out << ", ";
  	}
  
  	out << ") {\n";

  	// generate host function code  	
  	// assign values to all of the variable step sizes
  	def_iters.clear();
  	init_partitions(g.subgraphs, out, def_iters, step);

  	out << "int gxsize = " << blockx->iterations << "/" << blockx->step << ";\n";
  	out << "int gysize = " << blocky->iterations << "/" << blocky->step << ";\n";
  	out << "if (gxsize > 65535 || gysize > 65535) {\n";
  	out << "printf(\"ERROR: grid dimension too large\\n\");";
  	out << "exit(0);\n}\n";
  			
  	out << "dim3 dimGrid(gxsize, gysize);\n";

  	out << "int bxsize = " << thrdx->iterations << "/" << thrdx->step << ";\n";
  	out << "int bysize = " << thrdy->iterations << "/" << thrdy->step << ";\n";
  	out << "if ( bxsize * bysize > 512) {\n";
  	out << "printf(\"ERROR: block dimension too large\\n\");";
  	out << "exit(0);\n}\n";
  	out << "dim3 dimBlock(bxsize, bysize);\n";
  	
  	vertex reduction_size;
  	if (reduce) {
	  	// figure out temporary reduction space
	  	for (int i = 0; i != blockx->summations.size(); i++) {
	  		out << "float *red_x;\n";
	  		//out << "cudaMalloc((void**) &red_x, gxsize * " << blocky->iterations
	  		vertex u = blockx->summations[i];
	  		out << "cudaMalloc((void**) &red_x, gxsize * " << g.info(u).t.dim.base_rows 
	  			<< "*" << g.info(u).t.dim.base_cols
	  			<< "*" << "sizeof(" << precision_type << "));\n";
	  		reduction_size = u;
	  	}
	  	for (int i = 0; i != blocky->summations.size(); i++) {
	  		out << "float *red_x;\n";
	  		//out << "cudaMalloc((void**) &red_x, gxsize * " << blocky->iterations
	  		vertex u = blocky->summations[i];
	  		out << "cudaMalloc((void**) &red_x, gxsize * " << g.info(u).t.dim.base_rows 
	  			<< "*" << g.info(u).t.dim.base_cols
	  			<< "*" << "sizeof(" << precision_type << "));\n";
	  		reduction_size = u;
	  	}
  	}
  	
  	// thread level reduction space too large for shared memory
  	for (itr = sumto.begin(); itr != sumto.end(); itr++) {
  		int d = depth(find_summation_sg(itr->first, g));
  		if (d != 3 && d != 4)
  			continue;
	  	try {
			int sz = boost::lexical_cast<int>(itr->second);
	    }
	    catch(boost::bad_lexical_cast &) {
	        out << precision_type << " *r" << itr->first << "\n;";
	        out << "cudaMalloc((void**) &r" << itr->first << ", gxsize * gysize *"
	        	<< itr->second << "* sizeof(" << precision_type << "));\n";
	    }
  	}
  	
  	// allocate and transfer inputs to device
  	for (int i = 0; i != g.num_vertices(); i++) {
  		if (g.info(i).op == input || g.info(i).op == output || g.info(i).op == temporary) {
  			// ret = cudaMalloc((void**)&device, mem_size);
  			out << precision_type << " *dev_" << g.info(i).label << ";\n"
  				<< "cudaMalloc((void**)&dev_" << g.info(i).label << ","; 
  			get_size(&(g.info(i).t), out, g);
  			out << "* sizeof(" << precision_type << "));\n";
  			
  			// cudaMemcpy(device, host, mem_size, cudaMemcpyHostToDevice);
  			if (g.info(i).op == input) {
	  			out << "cudaMemcpy(dev_" << g.info(i).label << "," << g.info(i).label << ","
	  				<< "sizeof(" << precision_type << ")" << "*";
	  			get_size(&(g.info(i).t), out, g);
	  			out << ", cudaMemcpyHostToDevice);\n";
  			}
  		}
  	}
  	
  	// call   name<<<dimGrid, dimBlock>>>(args,..);
  	out << "dev_" << name << "<<<dimGrid, dimBlock>>>(";
  	for (map<string,type*>::const_iterator i = inputs.begin(); i != inputs.end(); ++i) {
    	out << "dev_" << i->first;
    	out << size_params_no_type(*i->second);
    	if (boost::next(i) != inputs.end())
      		out << ", ";
  	}
  	
  	if (inputs.begin() != inputs.end())
    	out << ", ";
  
  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
	    out << "dev_" << i->first;
	    out << size_params_no_type(*i->second);
	    if (boost::next(i) != outputs.end())
      		out << ", ";
  	}
  	
  	// arguments to device for reduction across blocks
  	if (reduce) {
  		// reduction space
	  	out << ", red_x, gxsize";
	  	
  	}
  	
  	// arguments for reduction across threads requiring more than shared memory
  	for (itr = sumto.begin(); itr != sumto.end(); itr++) {
  		int d = depth(find_summation_sg(itr->first, g));
  		if (d != 3 && d != 4)
  			continue;
	  	try {
			int sz = boost::lexical_cast<int>(itr->second);
	    }
	    catch(boost::bad_lexical_cast &) {
	        out << ", r" << itr->first;
	    }
  	}
  	if (thread_device)
  		out << ", gxsize";
  	
  	out << ");\n";
  	
  	
  	// reduction across blocks
  	if (reduce) {  	
	  	// call reduce across block kernel
	  	
	  	if (true) {
	  	//if (reduce_3d) {
	  		vertex p = reduction_size;
	  		while (g.find_parent(p) != 0) {
	  			p = g.adj(p)[0];
	  		}
	  		type *t = &g.info(p).t;
	  		kind kt = t->k;
	  		while (t->k != scalar) {
	  			if (t->k != kt) {
	  				break;
	  			}
	  			t = t->t;
	  		}
	  		string s = t->dim.dim;
	  		if (s.compare("1") == 0) {
	  			out << "dim3 redGrid (" << g.info(p).t.dim.dim << "/32, 1);\n";
		  		out << "dim3 redBlock(32,1);\n";
	  		}
	  		else {
		  		out << "dim3 redGrid (" << g.info(p).t.dim.dim << "/32, " << s << "/16);\n";
		  		out << "dim3 redBlock(32,16);\n";
	  		}
	  	}
	  	else {
	  		out << "dim3 redGrid (" << g.info(reduction_size).t.dim.dim<< ");\n";
	  		out << "dim3 redBlock (bxsize);\n";
	  	}
	  	out << "reduce_x <<<redGrid, redBlock>>> (red_x, gxsize";
	  	for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
		    out << ",dev_" << i->first;
		    out << size_params_no_type(*i->second);
	  	}
	  	out << ");\n";
  	}
  	
  	
  	
  	// transfer back outputs
  	for (int i = 0; i != g.num_vertices(); i++) {
  		if (g.info(i).op == output) {
  			// cudaMemcpy(host, device, mem_size, cudaMemcpyDeviceToHost);
  			out << "cudaMemcpy(" << g.info(i).label << ",dev_" << g.info(i).label << ","
  				<< "sizeof(" << precision_type << ")" << "*";
  			get_size(&(g.info(i).t), out, g);
  			out << ", cudaMemcpyDeviceToHost);\n";
  		}
  	}
  	
  	out << "}\n";
}

