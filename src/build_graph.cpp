#include "boost/lexical_cast.hpp"
#include "build_graph.hpp"
#include <fstream>
#include <boost/algorithm/string.hpp>

using std::vector;
using std::map;
using std::pair;
using std::make_pair;

static int subgraph_num = 0;

std::string type_to_string(type *t) {
	std::string s;
	while (t) {
		if (t->k == scalar)
			s += "scl";
		else if (t->k == row)
			s += "row";
		else if (t->k == column)
			s += "col";
		else if (t->k == parallel_reduce)
			s += "pr";
		t = t->t;
	}
	return s;
}

///////////// INITIAL DIM UPDATE ///////////////////////////////////

void update_dim_info(graph &g) {
	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		dim_info &d = g.info(i).t.dim;
		if (g.info(i).t.height == 0) {
			d.base_rows = "1";
			d.base_cols = "1";
			d.step = "1";
			d.lead_dim = "1";
		}
		else if (g.info(i).t.height == 1) {
			// vector
			dim_info &dd = g.info(i).t.t->dim;
			d.step = "1";
			d.lead_dim = "1";
			dd.step = "1";
			dd.lead_dim = "1";
			if (g.info(i).t.k == row) {
				d.base_rows = "1";
				d.base_cols = d.dim;
				dd.base_rows = "1";
				dd.base_cols = d.dim;
			}
			else {
				d.base_rows = d.dim;
				d.base_cols = "1";
				dd.base_rows = d.dim;
				dd.base_cols = "1";
			}
		}
		else if (g.info(i).t.height == 2) {

			// matrix
			dim_info &d1 = g.info(i).t.t->dim;
			dim_info &d0 = g.info(i).t.t->t->dim;
			d.step = "1";//d1.dim;
			d1.step = "1";
			d0.step = "1";

			d0.lead_dim = "1";
			d1.lead_dim = d1.dim;
			d.lead_dim = d1.dim;

			if (g.info(i).t.k == row) {
				d.base_rows = d1.dim;
				d.base_cols = d.dim;
				d1.base_rows = d1.dim;
				d1.base_cols = d.dim;
				d0.base_rows = d1.dim;
				d0.base_cols = d.dim;
			}
			else {
				d.base_rows = d.dim;
				d.base_cols = d1.dim;
				d1.base_rows = d.dim;
				d1.base_cols = d1.dim;
				d0.base_rows = d.dim;
				d0.base_cols = d1.dim;
			}
		}
		else {
			std::cout << "ERROR: build_graph.cpp: update_dim_info(): unexpected type\n";
		}
	}
}

//////////////////////// END DIM UPDATE //////////////////////////////////////

//////////////////////// INITIAL SIZE UNIFICATION ////////////////////////////

void register_type(type &r, graph &g) {
	type *t = &r;
	while (t) {
		g.register_iter(t->dim.dim);
		t->dim.dim = g.get_iter_rep(t->dim.dim);
		t = t->t;
	}
}

void merge_all_types(type &l, type &r, graph &g) {
	if (l.height != r.height)
		std::cout << "ERROR: build_graph.cpp: merge_all_types(): unexpected types\n";

	type *pl = &l;
	type *pr = &r;
	while (pl && pr) {
		g.merge_iters(pl->dim.dim, pr->dim.dim);
		pl->dim.dim = g.get_iter_rep(pl->dim.dim);
		pr->dim.dim = g.get_iter_rep(pr->dim.dim);
		pl = pl->t;
		pr = pr->t;
	}
}

void mult_sizes(type *u, type *l, type *r, graph &g) {
	if (l->k == row && r->k == column) {
		// operands
		g.merge_iters(l->dim.dim, r->dim.dim);
		r->dim.dim = g.get_iter_rep(r->dim.dim);
		l->dim.dim = g.get_iter_rep(l->dim.dim);

		mult_sizes(u,l->t,r->t,g);
	}
	else if (l->k == column && r->k == row) {
		if (u->k == row) {
			g.merge_iters(r->dim.dim,u->dim.dim);
			r->dim.dim = g.get_iter_rep(u->dim.dim);
			u->dim.dim = g.get_iter_rep(u->dim.dim);
			mult_sizes(u->t, l, r->t, g);
		}
		else {
			g.merge_iters(l->dim.dim,u->dim.dim);
			l->dim.dim = g.get_iter_rep(u->dim.dim);
			u->dim.dim = g.get_iter_rep(u->dim.dim);
			mult_sizes(u->t, l->t, r, g);
		}
	}
	else if ((l->k == scalar && r->k == column)
			|| (l->k == scalar && r->k == row)) {
		// scaling
		g.merge_iters(r->dim.dim,u->dim.dim);
		r->dim.dim = g.get_iter_rep(r->dim.dim);
		u->dim.dim = g.get_iter_rep(u->dim.dim);
	}
	else if ((l->k == row && r->k == scalar)
			|| (l->k == column && r->k == scalar)) {
		// scaling
		g.merge_iters(l->dim.dim, u->dim.dim);
		l->dim.dim = g.get_iter_rep(l->dim.dim);
		u->dim.dim = g.get_iter_rep(u->dim.dim);
	}
	else if (l->k == row && r->k == row) {	
		if (u->k == row) {
			// result and one or other operand
			g.merge_iters(u->dim.dim, r->dim.dim);
			r->dim.dim = g.get_iter_rep(r->dim.dim);
			u->dim.dim = g.get_iter_rep(u->dim.dim);
			mult_sizes(u->t,l,r->t,g);
		}
		else if (u->k == column) {
			// k
			g.merge_iters(l->dim.dim, r->t->dim.dim);
			r->t->dim.dim = g.get_iter_rep(l->dim.dim);
			l->dim.dim = g.get_iter_rep(l->dim.dim);

			type *tmp = new type(*(r->t));
			if (tmp->k == row)
				tmp->k = column;
			else
				tmp->k = row;
			tmp->dim.dim = r->dim.dim;
			tmp->dim.step = r->dim.step;

			mult_sizes(u,l->t,tmp,g);
			delete tmp;
		}
	}
	else if (l->k == column && r->k == column) {	
		if (u->k == row) {
			// k 
			g.merge_iters(l->t->dim.dim, r->dim.dim);
			l->t->dim.dim = g.get_iter_rep(r->dim.dim);
			r->dim.dim = g.get_iter_rep(r->dim.dim);

			type *tmp = new type(*(l->t));
			if (tmp->k == row)
				tmp->k = column;
			else
				tmp->k = row;
			tmp->dim.dim = l->dim.dim;
			tmp->dim.step = l->dim.step;

			mult_sizes(u,tmp,r->t,g);
			delete tmp;
		}
		else if (u->k == column) {
			// result and one or other operand
			g.merge_iters(u->dim.dim, l->dim.dim);
			l->dim.dim = g.get_iter_rep(u->dim.dim);
			u->dim.dim = g.get_iter_rep(u->dim.dim);
			mult_sizes(u->t,l->t,r,g);		
		}
	}
	else {
		// must be be scalar * scalar
		return;
	} 
}

void get_latest(type &ct, graph &g) {
	type *t = &ct;
	while (t) {
		if (t->k == scalar)
			break;
		t->dim.dim = g.get_iter_rep(t->dim.dim);
		t->dim.step = g.get_iter_rep(t->dim.step);

		string tmp = g.get_iter_rep(t->dim.lead_dim);
		t->dim.lead_dim = g.get_iter_rep(tmp);
		t->dim.base_rows = g.get_iter_rep(t->dim.base_rows);
		t->dim.base_cols = g.get_iter_rep(t->dim.base_cols);
		t = t->t;
	}
}

void update_iters_sg(graph &g, subgraph *sg) {
	vector<iterOp_t>::iterator i;
	for (i = sg->sg_iterator.conditions.begin();
			i != sg->sg_iterator.conditions.end(); ++i) {

		vector<string> separate;
		boost::split(separate, i->right, boost::is_any_of("+"));

		string newString = g.get_iter_rep(separate[0]);
		for (unsigned int k=1; k < separate.size(); ++k) {
			newString += "+" + g.get_iter_rep(separate[k]);
		}
		i->right = newString;
	}

	for (i = sg->sg_iterator.updates.begin();
			i != sg->sg_iterator.updates.end(); ++i) {

		vector<string> separate;
		boost::split(separate, i->right, boost::is_any_of("+"));

		string newString = g.get_iter_rep(separate[0]);
		for (unsigned int k = 1; k < separate.size(); ++k) {
			newString += "+" + g.get_iter_rep(separate[k]);
		}
		i->right = newString;
	}

	for (auto i : sg->subs) {
		update_iters_sg(g, i);
	}
}

void update_iters(graph &g) {
	for (unsigned int i = 0; i < g.num_vertices(); ++i) {
		if (g.info(i).op == deleted)
			continue;
		get_latest(g.info(i).t, g);
	}

	for (unsigned int i = 0; i < g.subgraphs.size(); ++i)
		update_iters_sg(g, g.subgraphs[i]);
}

void update_sizes(graph &g) {

	// register sizes
	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		register_type(g.info(i).t,g);

		switch (g.info(i).op) {
			case input:
			case temporary: {
								break;
							}
			case output: {
							 register_type(g.info(g.inv_adj(i)[0]).t,g);
							 merge_all_types(g.info(i).t, g.info(g.inv_adj(i)[0]).t, g);
							 break;
						 }
			case trans: {
							// size at all levels of inv_adj must equal size at all levels of current
							register_type(g.info(g.inv_adj(i)[0]).t,g);
							merge_all_types(g.info(i).t, g.info(g.inv_adj(i)[0]).t, g);
							break;
						}
			case squareroot: {
								 register_type(g.info(g.inv_adj(i)[0]).t,g);
								 merge_all_types(g.info(g.inv_adj(i)[0]).t, g.info(i).t, g);
								 break;
							 }
			case add:
			case subtract: {
							   // size at all levels of both ops and result must be equal
							   register_type(g.info(g.inv_adj(i)[0]).t,g);
							   register_type(g.info(g.inv_adj(i)[1]).t,g);
							   merge_all_types(g.info(g.inv_adj(i)[0]).t, g.info(g.inv_adj(i)[1]).t,g);
							   merge_all_types(g.info(g.inv_adj(i)[0]).t, g.info(i).t, g);
							   break;
						   }
			case multiply: {
							   register_type(g.info(g.inv_adj(i)[0]).t,g);
							   register_type(g.info(g.inv_adj(i)[1]).t,g);
							   register_type(g.info(i).t, g);
							   mult_sizes(&g.info(i).t, &g.info(g.inv_adj(i)[0]).t, &g.info(g.inv_adj(i)[1]).t, g);
							   break;
						   }
			case divide: {
							 register_type(g.info(g.inv_adj(i)[0]).t,g);
							 register_type(g.info(g.inv_adj(i)[1]).t,g);
							 merge_all_types(g.info(g.inv_adj(i)[0]).t,g.info(i).t,g);
							 break;
						 }
			default: {
						 std::cout << "ERROR: build_graph.cpp: update_sizes(): unexpected type\n";
						 break;
					 }
		}
	}

	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		get_latest(g.info(i).t, g);
	}
}

/////////////////////////// END SIZE /////////////////////////////////////////

int find_or_add_op(op_code op, vector<vertex> const& rands, graph& g) {
	for (unsigned int i = 0; i != g.num_vertices(); ++i) {
		if (g.info(i).op == op) {
			if (rands.size() == g.inv_adj(i).size()
					&& includes(rands.begin(), rands.end(), g.inv_adj(i).begin(), g.inv_adj(i).end()))
				return i;
		}
	}
	int n;
	if (op == trans && rands.size() > 0 && g.info(rands[0]).label.compare("") != 0) {
		vertex_info vi(op, g.info(rands[0]).label);
		n = g.add_vertex(vi);
	}
	else {
		vertex_info vi(op); 
		n = g.add_vertex(vi);
	}

	for (unsigned int i = 0; i != rands.size(); ++i)
		g.add_edge(rands[i], n);

	return n;
}

vertex operation::to_graph(map<string, vertex>& env,
		map<string,type*>const& inputs,
		map<string,type*>const& outputs, graph& g)
{

	vector<vertex> v;
	for (unsigned int i = 0; i != operands.size(); ++i) {
		v.push_back(operands[i]->to_graph(env,inputs, outputs,g));
	}
	return find_or_add_op(op, v, g);
}

void set_container_size(string name, type &t) {
	if (t.height > 3)
		std::cout << "WARNING: build_graph.cpp: set_container_size(): unexpected type\n";

	if (t.t) {
		set_container_size(name, *(t.t));
		// this only work up to height of three (no larger than matrix input)
		// and for normal matrices: ie no row<row<scl>>
		t.dim.base_rows = t.t->dim.base_rows;
		t.dim.base_cols = t.t->dim.base_cols;
		if (t.k == row) {
			t.dim.base_cols = t.dim.dim;
			t.dim.base_rows = t.t->dim.base_rows;
		}
		else {
			t.dim.base_rows = t.dim.dim;
			t.dim.base_cols = t.t->dim.base_cols;
		}
	}

	switch (t.k) {
		case unknown:
			t.dim.dim = "??";
			break;
		case row:
			t.dim.dim = name + "_ncols";
			break;
		case column:
			t.dim.dim = name + "_nrows";
			break;
		case scalar:
			t.dim.dim = "1";
			t.dim.lead_dim = "1";
			t.dim.base_cols = "1";
			t.dim.base_rows = "1";
			break;
		default:
			std::cout << "ERROR: build_graph.cpp: set_container_size(): unknown kind\n";
	}

	t.dim.step = "1";
	return;
}	

vertex variable::to_graph(map<string, vertex>& env, 
		map<string,type*> const& inputs, 
		map<string,type*> const& outputs, 
		graph& g)
{
	map<string,vertex>::iterator i = env.find(name);
	if (i == env.end()) {
		map<string,type*>::const_iterator j = inputs.find(name);    
		if (j == inputs.end()) {    
			map<string,type*>::const_iterator k = outputs.find(name);          
			if (k == outputs.end()) {
				std::cerr << "undefined variable " << name << std::endl;
				exit(-1);
				return 0;
			} else {
				std::cerr << "outputs should not be read before an assignment" << std::endl;
				exit(-1);
			}
		} else { // variable is an input variable
			vertex_info vi(*(j->second), input, name);
			set_container_size(name, vi.t);
			vertex v = g.add_vertex(vi);
			env[name] = v;
			return v;
		}
	} else { // already in the environment
		return i->second;
	}
}

vertex scalar_in::to_graph(map<string, vertex>& env, 
		map<string,type*> const& inputs, 
		map<string,type*> const& outputs, 
		graph& g)
{
	//type t(scalar, "1");
	type t(general);
	vertex_info vi(t, input, boost::lexical_cast<string>(val), val);
	return g.add_vertex(vi);
}

	void
program2graph(vector<stmt*> const& p, 
		map<string,type*>& inputs, 
		map<string,type*>& inouts, 
		map<string,type*>& outputs, 
		graph& g)
{



	map<string, type*>::iterator i;
	for (i = inouts.begin(); i != inouts.end(); ++i) {
		inputs.insert(*i);
		outputs.insert(*i);
	}

	for (i = inputs.begin(); i != inputs.end(); i++) {
		set_container_size(i->first, *(i->second));
	}
	for (i = outputs.begin(); i != outputs.end(); i++) {
		set_container_size(i->first, *(i->second));
	}

	map<string, vertex> env;
	for (unsigned int i = 0; i != p.size(); ++i) {
		stmt* s = p[i];

		vertex tmp = s->rhs->to_graph(env, inputs, outputs, g);
		env[s->lhs] = tmp;
	}

	///// DEBUG
#ifdef DEBUG
	std::ofstream out("lower0.dot");
	print_graph(out, g);
#endif

	for (map<string,type*>::iterator i = outputs.begin(); i != outputs.end(); ++i) {
		vertex def = env[i->first];
		vertex_info vi; vi.t = *i->second; vi.op = output; vi.label = i->first;
		vi.eval = evaluate; vi.trivial = true;
		string name = i->first;
		set_container_size(name, vi.t);
		vertex v = g.add_vertex(vi);
		g.add_edge(def, v);
	}

	// push out types up to operation types
	for (vertex i = 0; i < g.num_vertices(); i++) {
		if (g.info(i).op == output) {
			std::vector<vertex> const& iav = g.inv_adj(i);
			for (vertex j = 0; j < iav.size(); j++) {
				if (g.info(iav[j]).t.k == unknown) {
					g.info(iav[j]).t = g.info(i).t;
				}
			}
		}
	}
}

string format_to_name(storage);
string attribute_to_string(sg_iterator_t*);

void print_subgraph(std::ostream& out, subgraph* sg, graph const& g) {
	out << "subgraph cluster" << subgraph_num++ << " {\n" << std::endl;

	out << "label = \"conditions:" << sg->sg_iterator.printConditions()
		<< "\\nupdates: "
		<< sg->sg_iterator.printUpdates() 
		<< "\\n" << format_to_name(sg->sg_iterator.format)
		<< attribute_to_string(&sg->sg_iterator)
		<< "\\n id:" << sg->str_id << "\";" << std::endl;
	out << "style = solid;\n";

	for (unsigned int j = 0; j != sg->vertices.size(); ++j) {
		vertex u = sg->vertices[j];
		if (g.adj(u).size() > 0 || g.inv_adj(u).size() > 0)
			out << u << ";\n";
	}
	for (unsigned int j = 0; j != sg->subs.size(); ++j)
		print_subgraph(out, sg->subs[j], g);
	out << "}" << std::endl;
}

void print_graph(std::ostream& out, graph const& g) {
	out << "digraph G {" << std::endl;
	for (unsigned int i = 0; i != g.num_vertices(); ++i) {
		if (g.adj(i).size() > 0 || g.inv_adj(i).size() > 0) {
			const vertex_info &lsd = g.info(i);
			string l = lsd.to_string(i);
			out << i << g.info(i).to_string(i) << std::endl;
		}
		for (unsigned int j = 0; j != g.adj(i).size(); ++j) {
			out << i << " -> " << g.adj(i)[j] << ";" << std::endl;
		}

	}
	for (unsigned int i = 0; i != g.subgraphs.size(); ++i)
		print_subgraph(out, g.subgraphs[i], g);
	out << "}" << std::endl;
}

