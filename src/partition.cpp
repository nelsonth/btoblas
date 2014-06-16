#include "partition.hpp"
#include "build_graph.hpp"
#include "analyze_graph.hpp"
#include "md5wrapper.h"
#include <fstream>


////////////////////////////// PARTITION HELPERS ///////////////////////////////////


string scrub_dollar(string s, string replace) {
    while (s.find("$$") != string::npos) {
        int loc = s.find("$$");
  		s.erase(loc,2);
  		s.insert(loc, replace);
  	}
    return s;
}


void check_depth(int curr, int &max_depth, vector<subgraph*> subs) {
	// count levels of subgraphs
	
	for (unsigned int i = 0; i != subs.size(); i++) {
		if (subs[i]->subs.size() == 0) {
			if (curr > max_depth)
				max_depth = curr;
		}
		else {
			check_depth(curr+1, max_depth, subs[i]->subs);
		}
	}
}


void update_graph_dispatch(vertex u, graph &g, vector<bool> &up) {
	//std::cout << u << "\n";
	if (g.info(u).op == trans)
		update_graph(u, g, up);
	
	for (unsigned int i = 0; i != g.adj(u).size(); i++) {
		update_graph(g.adj(u)[i], g, up);
	}
	for (unsigned int i = 0; i != g.inv_adj(u).size(); i++) {
		update_graph(g.inv_adj(u)[i], g, up);
	}
}

void update_graph(vertex u, graph &g, vector<bool> &up) {
	// propogate partition information
	// this does not handle leading dimension (currently unused)
	//std::cout << "\t" << u << "\t" << g.info(u).t.height << "\n";
		
	switch (g.info(u).op) {
	case input: {
		update_graph_dispatch(u, g, up);
		break;
	}
	case output:
	case temporary: {
		// output and temporary can only have one in edge; if change is above take that change
		if (g.inv_adj(u).size() != 1)
			std::cout << "WARNING: partition.cpp: update_graph(): unexpected type (out, tmp)\n";
		
		vertex v = g.inv_adj(u)[0];
		if (up[v] && up[u])
			break;
		
		if (up[v]) {
			type &old_t = g.info(u).t;
			type *new_t = new type(g.info(v).t);
			old_t = *new_t;
			up[u] = true;
			update_graph_dispatch(u, g, up);
		}
		else if (up[u]) {
			type &old_t = g.info(v).t;
			type *new_t = new type(g.info(u).t);
			old_t = *new_t;
			up[v] = true;
			update_graph_dispatch(v, g, up);
		}
		break;
	}
	case trans: {
		vertex v = g.inv_adj(u)[0];
		if (up[u] && up[v])
			break;

		if (up[v]) {
			type &old_t = g.info(u).t;
			type *new_t = new type(g.info(v).t);
			type *t = new_t;
			// transpose
			while (t) {
				if (t->k == row)
					t->k = column;
				else if (t->k == column)
					t->k = row;
				else
					break;
				t = t->t;
			}
			old_t = *new_t;
			up[u] = true;
			update_graph_dispatch(u, g, up);
		}
		else if (up[u]) {
			type &old_t = g.info(v).t;
			type *new_t = new type(g.info(u).t);
			type *t = new_t;
			// transpose
			while (t) {
				if (t->k == row)
					t->k = column;
				else if (t->k == column)
					t->k = row;
				else
					break;
				t = t->t;
			}
			old_t = *new_t;
			up[v] = true;
			update_graph_dispatch(v, g, up);
		}
		break;
	}
	case add:
	case subtract: {
		vertex l = g.inv_adj(u)[0];
		vertex r = g.inv_adj(u)[1];
		if (up[u] && up[l] && up[r])
			break;
		
		if (up[u]) {
			// result brings change
			type &old_l = g.info(l).t;
			type &old_r = g.info(r).t;
			type *new_t = new type(g.info(u).t);
			old_l = *new_t;
			new_t = new type(g.info(u).t);
			old_r = *new_t;
			up[l] = true;
			up[r] = true;
			update_graph_dispatch(l, g, up);
			update_graph_dispatch(r, g, up);
		}
		else if (up[l]) {
			// left operand brings change
			type &old_u = g.info(u).t;
			type &old_r = g.info(r).t;
			type *new_t = new type(g.info(l).t);
			old_u = *new_t;
			new_t = new type(g.info(l).t);
			old_r = *new_t;
			up[u] = true;
			up[r] = true;
			update_graph_dispatch(u, g, up);
			update_graph_dispatch(r, g, up);
		}
		else if (up[r]) {
			// right operand brings change
			type &old_u = g.info(u).t;
			type &old_l = g.info(l).t;
			type *new_t = new type(g.info(r).t);
			old_u = *new_t;
			new_t = new type(g.info(r).t);
			old_l = *new_t;
			up[u] = true;
			up[l] = true;
			update_graph_dispatch(u, g, up);
			update_graph_dispatch(l, g, up);
		}
		break;
	}
	case multiply: {
		update_mult(u, g, up);
		break;
	}
	default: {
		std::cout << "FINISH: partition.cpp: update_graph(): finish for op "
				  << g.info(u).op << "\n";
		break;
	}
	}
	return;
}

void updateStructure(type *t, string base_rows, string base_cols, string lead_dim) {
	while (t->k != scalar) {
		t->dim.base_rows = base_rows;
		t->dim.base_cols = base_cols;
		t->dim.lead_dim = lead_dim;
		t = t->t;
	}
}

#include "type_analysis.hpp"
void update_mult(vertex u, graph &g, vector<bool> &up) {
	// number of cols of left must equal number of cols of result
	// number of rows of right must equal number of rows of result
	// number of rows of left must equal number of cols of right
	//...except in the case of scaling...
	
	//std::cout << "\t" << u << "\n";
	//std::ofstream out("lower1.dot");
	//print_graph(out, g);
	
	vertex l = g.inv_adj(u)[0];
	vertex r = g.inv_adj(u)[1];

	if (up[u] && up[l] && up[r])
		return;

	// will leave lhr pointing to highest row or a scalar	
	// will leave rhc pointing to highest column or a scalar
	type *lhr = g.info(l).t.get_highest_row();
	type *rhc = g.info(r).t.get_highest_column();

	type *uhr = g.info(u).t.get_highest_row();
	type *uhc = g.info(u).t.get_highest_column();
	
	type *lhc = g.info(l).t.get_highest_column();
	type *rhr = g.info(r).t.get_highest_row();

	int left = 0, right = 0, op_r = 0, op_l = 0, r_op = 0, l_op = 0;
	int op_r_s = 0, r_op_s = 0, op_l_s = 0, l_op_s = 0; 
	int lr = g.info(l).t.num_rows();
	int lc = g.info(l).t.num_cols();
	int rr = g.info(r).t.num_rows();
	int rc = g.info(r).t.num_cols();
	int ur = g.info(u).t.num_rows();
	int uc = g.info(u).t.num_cols();
	
	if (g.info(l).t.k == scalar && g.info(r).t.k == scalar) {
		std::cout << "WARNING: partition.cpp: update_mult(): updating scalar mult?\n";
		return;
	}
	if (g.info(l).t.k != scalar && g.info(r).t.k != scalar) {
		// not scaling
		if (lr != rc) {
			if (up[l])
				right = 1;
			else if (up[r])
				left = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected l/r\n";
		}
		if (lc != uc) {
			if (up[l])
				op_l = 1;
			else if (up[u])
				l_op = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected l/u\n";	
		}
		if (rr != ur) {
			if (up[r])
				op_r = 1;
			else if (up[u])
				r_op = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected r/u\n\t\t"
						  << r << "\t" << u << "\n";
		}
	}
	if (g.info(l).t.k == scalar && g.info(r).t.height != g.info(u).t.height) {
		// scaling right
		if (rr != ur) {
			if (up[r])
				op_r = 1;
			else if (up[u])
				r_op = 1;
			else {
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected r/u\n\t\t"
						  << r << "\t" << u << "\n";
			}
		}
		if (rc != uc) {
			if (up[r])
				op_r_s = 1;
			else if (up[u])
				r_op_s = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected r/u\n";
		}
	}
	if (g.info(r).t.k == scalar && g.info(l).t.height != g.info(u).t.height) {
		// scaling left
		if (lc != uc) {
			if (up[l])
				op_l = 1;
			else if (up[u])
				l_op = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected l/u\n";	
		}
		if (lr != ur) {
			if (up[l])
				op_l_s = 1;
			else if (up[u])
				l_op_s = 1;
			else
				std::cout << "WARNING: partition.cpp: update_mult(): unexpected l/u\n";
		}
	}

	if (left + right + op_r + op_l + l_op + r_op + r_op_s + op_r_s + l_op_s + op_l_s == 0)
		return;

	if (left + right + op_r + op_l + l_op + r_op + r_op_s + op_r_s + l_op_s + op_l_s != 1) {
		std::cout << "ERROR: partition.cpp: update_mult(): wrong analysis\n";
		std::cout << left << ";" << right << ";" << op_r << ";" << op_l << ";" << r_op << ";" << l_op << "\n";
		std::cout << up[u] << ";" << up[l] << ";" << up[r] << "\n";
		std::cout << u << "\n";
		std::ofstream out("lower.dot");
		print_graph(out, g);
		exit(0);
	}
	
	if (left) {
		//std::cout << "left\n";
		//std::cout << prnt_detail(&g.info(l).t);
		// left needs updating based on rights information
		type *nw = new type(g.info(r).t.s);
		*nw = g.info(r).t;
		nw->k = row;
		nw->height = g.info(l).t.height + 1;
		
		lhr->dim.dim = g.info(r).t.dim.step;
		
		nw->t = new type(g.info(l).t);
		updateStructure(nw, g.info(l).t.dim.base_rows, g.info(l).t.dim.base_cols, 
						g.info(l).t.dim.lead_dim);
		g.info(l).t = *nw;
		//std::cout << prnt_detail(&g.info(l).t);
		up[l] = true;
		update_graph_dispatch(l, g, up);
	}
	else if (right) {
		//std::cout << "right\n";
		//std::cout << prnt_detail(&g.info(r).t);
		// right needs updating based on lefts information
		type *nw = new type(g.info(l).t.s);
		*nw = g.info(l).t;
		nw->k = column;
		nw->height = g.info(r).t.height + 1;
		
		rhc->dim.dim = g.info(l).t.dim.step;
		
		nw->t = new type(g.info(r).t);
		updateStructure(nw, g.info(r).t.dim.base_rows, g.info(r).t.dim.base_cols, 
						g.info(r).t.dim.lead_dim);
		g.info(r).t = *nw;
		//std::cout << prnt_detail(&g.info(r).t);
		up[r] = true;
		update_graph_dispatch(r, g, up);
	}
	else if (op_r) {
		//std::cout << "op_r\n";
		//std::cout << prnt_detail(&g.info(u).t);
		// result(u) needs updating based on rights information
		type *nw = new type(g.info(r).t.s);
		*nw = g.info(r).t;
		nw->height = g.info(u).t.height + 1;
		
		uhr->dim.dim = g.info(r).t.dim.step;
		
		nw->t = new type(g.info(u).t);
		updateStructure(nw, g.info(u).t.dim.base_rows, g.info(u).t.dim.base_cols, 
						g.info(u).t.dim.lead_dim);
		g.info(u).t = *nw;
		//std::cout << prnt_detail(&g.info(u).t);
		up[u] = true;
		update_graph_dispatch(u, g, up);
	}
	else if (op_l) {
		//std::cout << "op_l\n";
		//std::cout << prnt_detail(&g.info(u).t);
		// result(u) needs updating based on lefts information
		type *nw = new type(g.info(l).t.s);
		*nw = g.info(l).t;
		nw->height = g.info(u).t.height + 1;
		
		uhc->dim.dim = g.info(l).t.dim.step;
		
		nw->t = new type(g.info(u).t);
		updateStructure(nw, g.info(u).t.dim.base_rows, g.info(u).t.dim.base_cols, 
						g.info(u).t.dim.lead_dim);
		g.info(u).t = *nw;
		//std::cout << prnt_detail(&g.info(u).t);
		up[u] = true;
		update_graph_dispatch(u, g, up);
	}
	else if (r_op) {
		//std::cout << "r_op\n";
		//std::cout << prnt_detail(&g.info(r).t);
		// right needs updating based on results(u) information
		type *nw = new type(g.info(u).t.s);
		*nw = g.info(u).t;
		nw->height = g.info(r).t.height + 1;
		
		rhr->dim.dim = g.info(u).t.dim.step;
		
		nw->t = new type(g.info(r).t);
		updateStructure(nw, g.info(r).t.dim.base_rows, g.info(r).t.dim.base_cols, 
						g.info(r).t.dim.lead_dim);
		g.info(r).t = *nw;
		//std::cout << prnt_detail(&g.info(r).t);
		up[r] = true;
		update_graph_dispatch(r, g, up);
	}
	else if (l_op) {
		//std::cout << "l_op\n";
		//std::cout << prnt_detail(&g.info(l).t);
		// left needs updating based on results(u) information
		type *nw = new type(g.info(u).t.s);
		*nw = g.info(u).t;
		nw->height = g.info(l).t.height + 1;
		
		lhc->dim.dim = g.info(u).t.dim.step;
		
		nw->t = new type(g.info(l).t);
		updateStructure(nw, g.info(l).t.dim.base_rows, g.info(l).t.dim.base_cols, 
						g.info(l).t.dim.lead_dim);
		g.info(l).t = *nw;
		//std::cout << prnt_detail(&g.info(l).t);
		up[l] = true;
		update_graph_dispatch(l, g, up);
	}
	else if (op_r_s) {
		//std::cout << "op_r_s\n";
		//std::cout << prnt_detail(&g.info(u).t);
		// scaling and mismatch in columns
		// result(u) needs updating based on rights information
		type *nw = new type(g.info(r).t.s);
		*nw = g.info(r).t;
		nw->height = g.info(u).t.height + 1;
		
		uhc->dim.dim = g.info(r).t.dim.step;
		
		nw->t = new type(g.info(u).t);
		updateStructure(nw, g.info(u).t.dim.base_rows, g.info(u).t.dim.base_cols, 
						g.info(u).t.dim.lead_dim);
		g.info(u).t = *nw;
		//std::cout << prnt_detail(&g.info(u).t);
		up[u] = true;
		update_graph_dispatch(u, g, up);
	}
	else if (op_l_s) {
		//std::cout << "op_l_s\n";
		//std::cout << prnt_detail(&g.info(u).t);
		// scaling and mismatch in rows
		// result(u) needs updating based on lefts information
		type *nw = new type(g.info(l).t.s);
		*nw = g.info(l).t;
		nw->height = g.info(u).t.height + 1;
		
		uhr->dim.dim = g.info(l).t.dim.step;
		
		nw->t = new type(g.info(u).t);
		updateStructure(nw, g.info(u).t.dim.base_rows, g.info(u).t.dim.base_cols, 
						g.info(u).t.dim.lead_dim);
		g.info(u).t = *nw;
		//std::cout << prnt_detail(&g.info(u).t);
		up[u] = true;
		update_graph_dispatch(u, g, up);
	}
	else if (r_op_s) {
		//std::cout << "r_op_s\n";
		//std::cout << prnt_detail(&g.info(r).t);
		// scaling and mismatch in columns
		// right needs updating based on results(u) information
		type *nw = new type(g.info(u).t.s);
		*nw = g.info(u).t;
		nw->height = g.info(r).t.height + 1;
		
		rhc->dim.dim = g.info(u).t.dim.step;
		
		nw->t = new type(g.info(r).t);
		updateStructure(nw, g.info(r).t.dim.base_rows, g.info(r).t.dim.base_cols, 
						g.info(r).t.dim.lead_dim);
		g.info(r).t = *nw;
		//std::cout << prnt_detail(&g.info(r).t);
		up[r] = true;
		update_graph_dispatch(r, g, up);
	}
	else if (l_op_s) {
		//std::cout << "l_op_s\n";
		//std::cout << prnt_detail(&g.info(l).t);
		// scaling and mismatch in rows
		// left needs updating based on results(u) information
		type *nw = new type(g.info(u).t.s);
		*nw = g.info(u).t;
		nw->height = g.info(l).t.height + 1;
		
		lhr->dim.dim = g.info(u).t.dim.step;
		
		nw->t = new type(g.info(l).t);
		updateStructure(nw, g.info(l).t.dim.base_rows, g.info(l).t.dim.base_cols, 
						g.info(l).t.dim.lead_dim);
		g.info(l).t = *nw;
		//std::cout << prnt_detail(&g.info(l).t);
		up[l] = true;
		update_graph_dispatch(l, g, up);
	}

	return;
}

bool part_check(type *l, type *r, kind lk, kind rk) {
	// look for the given kind somewhere in the corresponding type
	// return true if both have their requrested kind
	while (l) {
		if (l->k == scalar)
			return false;
		if (l->k == lk)
			break;
		l = l->t;
	}
	while (r) {
		if (r->k == scalar)
			return false;
		if (r->k == rk)
			break;
		r = r->t;
	}
	return true;
}

/////////////////////// END PART HELPERS ////////////////////////




///////////////////////// PARTITION_T ///////////////////////////

// these are used by the main graph, but build dependencies
// make it difficult to use more than void* there and put
// these utility functions elsewhere...
void cleanup_partition_t(void *p) {
	partition_t *pI = (partition_t*)p;
	pI->dimensions.clear();
	pI->partitionDeps.clear();
	pI->unique.clear();
	delete pI;
}

void* copy_partition_t(void *p) {
	if (p == NULL)
		return NULL;
	
	partition_t *pI = (partition_t*)p;
	partition_t *newP = new partition_t();
	newP->dimensions = pI->dimensions;
	newP->partitionDeps = pI->partitionDeps;
	newP->unique = pI->unique;
	return (void*)newP;
}

//////////////////// PARTITION_T /////////////////////////////




///////////////// FIND PARTITIONING //////////////////////////

void part_collect_work(vertex u,
		  graph& g, std::stack<work_item>& s,
		  vector<rewrite_fun>const& check,
		  string history,
		  bool & all)
{
    for (unsigned int i = 0; i != check.size(); ++i)
      if (check[i](u, g)) {
		work_item w(new graph(g), i, u, "",1);
		s.push(w);
      }

}

std::string graph_to_string(graph &g) {
	std::string s;
	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		s += type_to_string(&g.info(i).t);
	}
	
	md5wrapper md5;
	return md5.getHashFromString(s);
}

void examine_type(type *t, vector<string> &rows, vector<string> &cols, 
				  set<string> &unique) {
	while (t->k != scalar) {
		if (t->dim.dim.find("$$") == 0)
			unique.insert(t->dim.dim);
		
		if (t->k == row)
			rows.push_back(t->dim.dim);
		else
			cols.push_back(t->dim.dim);
		t = t->t;
	}
}

void mapDeps(vector<string> &rows, vector<string> &cols, map<string,string> &deps) {
	vector<string>::iterator i;
	
	if (rows.size() > 1) {
		i = rows.begin()+1;
		for (; i != rows.end(); ++i) {
			if ((i+1) != rows.end())
				deps[*i] = *(i+1);
		}
	}
	
	if (cols.size() > 1) {
		i = cols.begin()+1;
		for (; i != cols.end(); ++i) {
			if ((i+1) != cols.end())
				deps[*i] = *(i+1);
		}
	}
}

bool map_partitions_to_sizes(graph &g, map<vertex, vector<struct PartitionChoice> > &partitions) {
	// we have a graph that is partitioned ideally as described in partitions.
	// perform some checks to ensure that is true and also perform some checks
	// to ensure that dependencies are not broken.
	// if any of this fails return false, else return true
	
	// once checks are complete, map the unique names to the sizes specified in
	// partitions.
	
	partition_t *pInfo = (partition_t*)g.get_partition_information();
	if (!pInfo) {
		// cout << "no pInfo" << endl;
		return false;
	}
	
	// we know the dimensionality of the partitioning for each operation vertex
	// and partitions has described some dimensionality.  we cannot ensure the
	// order matches, but we can ensure the dimensionaity does.
	
	map<vertex,int> &dimensions = pInfo->dimensions;
	
	map<vertex, vector<PartitionChoice> >::iterator jtr;
	vector<PartitionChoice>::iterator wtr;
	map<vertex,int>::iterator itr;
	
	jtr = partitions.begin();
	for (; jtr != partitions.end(); ++jtr) {
		itr = dimensions.find(jtr->first);
		if (itr == dimensions.end() && jtr->second.size() > 0) {
			 cout << "graph does not match requested number of partitions" << endl;
			return false;
		}
		
		wtr = jtr->second.begin();
		uint8_t way = 0;
		for (; wtr != jtr->second.end(); ++wtr) {
			if (wtr->way == m)
				way |= 0x1;
			else if (wtr->way == n)
				way |= 0x2;
			else if (wtr->way == k)
				way |= 0x4;
		}
		
		way = (way&0x1) + ((way>>1)&0x1) + ((way>>2)&0x1);
		
		if (way != itr->second) {
			//"graph is partitioned itr->second ways, but specification wants way ways."
			return false;
		}
	}
	
	// map the sizes specified in partitions to the unique strings in graph.
	map<string, int> &sizeMap = pInfo->sizeMap;
	jtr = partitions.begin();
	for (; jtr != partitions.end(); ++jtr) {
		vector<string> *ids = g.get_partition_ids(jtr->first);
		if (ids == NULL) {
			if (jtr->second.size() > 0) {
				// graph does not have partition information for requsted
				return false;
			}
			else {
				continue;
			}	
		}
		
		if (ids->size() != jtr->second.size()) {
			// number of partitions for given vertex do not match
			return false;
		}
			
		wtr = jtr->second.begin();
		vector<string>::iterator idIts = ids->begin();
		for (; wtr != jtr->second.end(); ++wtr, ++idIts) {
			
			map<string, int>::iterator smIter;
			smIter = sizeMap.find(*idIts);
			if (smIter == sizeMap.end()) {
				// no size mapped yet, just add it
				sizeMap[*idIts] = wtr->blocksize;
			}
			else {
				// existing size, check to make sure they match
				// NOTE: THIS WILL NEED TO CHANGE TO ACCOMODATE A RANGE
				if (smIter->second != wtr->blocksize) {
					// the sizes do match (fusion is the only cause of this)
					return false;
				}
			}
		}
	}
	
	
	
	// with sizes in place, should be able to check dependencies.
	map<string,string> &partitionDeps = pInfo->partitionDeps;
	map<string,string>::iterator dItr = partitionDeps.begin();
	for (; dItr != partitionDeps.end(); ++dItr) {
		if (sizeMap[dItr->first] < sizeMap[dItr->second]) {
			// dependency not met
			//std::cout << sizeMap[dItr->first] << " < " << sizeMap[dItr->second] << "\n";
			//return false;
		}
	}
	
	// sizeMap is made, and all checks have passed
	
	return true;
}

void update_partition_information(graph &g) {
	// fusion can change the number of unique partitions
	// which can change dependencies etc.
	
	partition_t *pInfo = (partition_t*)g.get_partition_information();
	if (!pInfo)
		return;
	
	// find new set of unique partitions by getting iter_reps
	// of all of the original ones.
	std::set<string> lunique = std::set<string>(pInfo->unique);
	std::set<string>::iterator uit = lunique.begin();
	pInfo->unique.clear();
	
	for (; uit != lunique.end(); ++uit) {
		pInfo->unique.insert(g.get_iter_rep(*uit));
	}
	
	// update partition deps with new names.
	std::map<string,string> partDeps = std::map<string,string>(pInfo->partitionDeps);
	pInfo->partitionDeps.clear();
	std::map<string,string>::iterator dit = partDeps.begin();
	for (; dit != partDeps.end(); ++dit) {
		pInfo->partitionDeps[g.get_iter_rep(dit->first)] = g.get_iter_rep(dit->second);
	}
	
	// update the ids given to each partition.
	for (unsigned int i = 0; i < g.num_vertices(); ++i) {
		vector<string> *ids = g.get_partition_ids(i);
		if (ids == NULL)
			continue;
		
		vector<string>::iterator itr = ids->begin();
		for (; itr != ids->end(); ++itr) {
			*itr = g.get_iter_rep(*itr);
		}
	}
}

void get_partition_information(graph &g) {
	// attach information about the partitioning to the graph.
	
	
	// 2 things to achieve
	// 1) ensure threads are divided correctly, for example consider
	//			gemm partitioned three ways, the code that divides
	//			up the threads must know there is an operation that
	//			operates as threadCntM * threadCntN * threadCntK
	// 2) ensure sizes are sane, specifically consider a single
	//			dimension partitioned more than once.  The inner
	//			partition size must be less than or equal to the
	//			outer partition size.
	//
	// 2 questions to answer for this
	// 1) is a matrix partitioned in more than one dimension
	//			rows.size() && cols.size() > 1
	// 2) is any operation partitioned more than once in a given
	//      dimension.  This is operation specific.
	
	// see definition of pInfo for how this data is reported.
	partition_t *pInfo = new partition_t();
	
	std::map<string,string> &partitionDeps = pInfo->partitionDeps;
	std::map<vertex,int> &dimensions = pInfo->dimensions;
	std::set<string> &unique = pInfo->unique;
	
	vertex v;
	for (v = 0; v < g.num_vertices(); ++v) {

		if (!(g.info(v).op & OP_2OP_ARITHMATIC)) 
			continue;

		vertex lop,rop;
		lop = g.inv_adj(v)[0];
		rop = g.inv_adj(v)[1];
				
		vector<string> lrows, lcols;
		vector<string> rrows, rcols;
		vector<string> vrows, vcols;
		
		if (g.info(v).op == multiply) {
			lrows.clear();
			lcols.clear();
			examine_type(&g.info(lop).t, lrows, lcols, unique);
			
			rrows.clear();
			rcols.clear();
			examine_type(&g.info(rop).t, rrows, rcols, unique);
		}
		
		vrows.clear();
		vcols.clear();
		examine_type(&g.info(v).t, vrows, vcols, unique);
		
		/*
		// this is all just debugging print statements...
		std::cout << "\n" << v << " = " << lop << " op " << rop << "\n";
		if  (g.info(v).op == multiply) {
			std::cout << lop << "\n\trows: ";
			vector<string>::iterator i;
			for (i = lrows.begin(); i != lrows.end(); ++i)
				std::cout << *i << "\t";
			std::cout << "\n\tcols: ";
			for (i = lcols.begin(); i != lcols.end(); ++i)
				std::cout << *i << "\t";
			std::cout << "\n";
			
			std::cout << rop << "\n\trows: ";
			for (i = rrows.begin(); i != rrows.end(); ++i)
				std::cout << *i << "\t";
			std::cout << "\n\tcols: ";
			for (i = rcols.begin(); i != rcols.end(); ++i)
				std::cout << *i << "\t";
			std::cout << "\n";
		}
        vector<string>::iterator i;
		std::cout << v << "\n\trows: ";
		for (i = vrows.begin(); i != vrows.end(); ++i)
			std::cout << *i << "\t";
		std::cout << "\n\tcols: ";
		for (i = vcols.begin(); i != vcols.end(); ++i)
			std::cout << *i << "\t";
		std::cout << "\n\n";
		*/
		
		// if the operation is multiply, look for 3D partitioning max
		//		this requires looking at both operands and the output types
		// else look for 2D partitioning max
		//		this only requires looking at any of the operands or the
		//		result type

		int rcnt = vrows.size();
		int ccnt = vcols.size();
		int dim;
		if (rcnt > 1 && ccnt > 1) {
			// 2D
			dim = 2;
		}
		else if (rcnt > 1 || ccnt > 1) {
			// 1D
			dim = 1;
		}
		else {
			// unparitioned.
			dim = 0;
		}
		
		if (g.info(v).op == multiply) {
			// v partitioning provides for us M and N dimensions
			// looking at lop rows or rop columns we can identify
			// K dimension.
			rcnt = lrows.size();
			if (rcnt > 1)
				++dim;
			
			mapDeps(lrows,lcols,partitionDeps);
			mapDeps(rrows,rcols,partitionDeps);
		}
		mapDeps(vrows,vcols,partitionDeps);
		
		dimensions[v] = dim;
				
		//std::cout << v << " is partitioned in " << dim << " ways\n";
		
	}
	/*
	std::cout << "this version has " << unique.size() << " unique partitions\n";
	std::cout << "and the following dependencies between partitions.\n";
	if (partitionDeps.size() == 0)
		std::cout << "\tnone\n";
	map<string,string>::iterator mi;
	for (mi = partitionDeps.begin(); mi != partitionDeps.end(); ++mi) {
		std::cout << "\t" << mi->first << " > " << mi->second << "\n";
	}
	*/
	
	g.set_partition_information((void*)pInfo);
}

void find_partitioning(graph& upg, std::stack<work_item>& s,
		  vector<rewrite_fun> const &check, vector<partition_fun> const &optimizations, vector<algo> const &algos,
		  vector<rewrite_fun> const &rewrites, int min_depth, int max_depth)
{
	int pid = 0;
	bool all = false;
	std::stack<work_item> ws;
	vector<string> uid;
	//int cnt = 0;
	
	//ws.push(&upg, -1, -1, 'tmp');
	for (unsigned int i = 0; i != upg.num_vertices(); i++) {
		part_collect_work(i,upg,ws,check,"",all);
	}
	
	
	while (!ws.empty()) {
		work_item current = ws.top();
		ws.pop();
		graph& g = *current.g;
		
		if (0 <= current.rewrite_rule) {
			optimizations[current.rewrite_rule](current.u, g, true);
			
			// must update algorithms
			assign_algorithms(algos, g);				
		}
		
		// use hashing to determine duplicate partitionings
		string id = graph_to_string(g);
		bool unique = true;
		for (unsigned int uid_itr = 0; uid_itr != uid.size(); uid_itr++) {
			if (uid[uid_itr].compare(id) == 0) {
				//cnt++;
				unique = false;
				break;
			}
		}
		if (unique)
			uid.push_back(id);
		else {
			current.del();
			continue;
		}
		
		graph *ng = new graph(g);
		// the easiest time to collect information about that partitions
		// that have been introduced is before the lowering.
		// attach that to the graph.
		get_partition_information(*ng);
		initial_lower(*ng, algos, rewrites);
		
		// if min_depth not met, continue
		int maxd = 0;
		check_depth(1,maxd, ng->subgraphs);
		if (maxd >= min_depth && maxd <= max_depth) {
			pid++;
			work_item w(ng, -1, -1, "",1);
			s.push(w);
		}
		else {
			delete ng;
		}
		
		if (maxd < max_depth) {
			part_collect_work(current.u,g,ws,check,"",all);
		}
		
		current.del();
		//std::cout << i << " done\n";
	}
	uid.clear();
	//std::cout << "count: " << cnt << "\n";
}

////////////////////////////// END FIND PARTITIONS /////////////////////////////////

//////////////////////////////////// PARTITIONERS /////////////////////////////////

// global counter for unique step identifiers
int suid = 0;

bool partition_add_same_operands(vertex u, graph& g, bool update) {
	// partition addition when both operands are the same
	// c = a*a
	if (g.info(u).op == add || g.info(u).op == subtract) {
		if (g.inv_adj(u)[0] != g.inv_adj(u)[1])
			return false;
		
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		g.add_partition_id(u,step);
		
		type &lt = g.info(g.inv_adj(u)[0]).t;
		type &ut = g.info(u).t;
		
		// create a new type for both operands and result and 
		// new type is same kind as highest level of old type
		// from old<T> to old<recursive_copy(old<T>)> or old<old<T>>
		type *newl = new type(lt);
		type *newu = new type(ut);
		
		delete lt.t;
		delete ut.t;
		
		lt.t = newl;
		ut.t = newu;
		
		lt.height = lt.t->height + 1;
		ut.height = ut.t->height + 1;
		
		lt.dim.step = step;
		ut.dim.step = step;
		
		lt.dim.dim = lt.t->dim.dim;
		lt.t->dim.dim = step;
		ut.dim.dim = ut.t->dim.dim;
		ut.t->dim.dim = step;
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

bool partition_add(vertex u, graph& g, bool update) {
	if (g.info(u).op == add || g.info(u).op == subtract) {
		if (g.inv_adj(u)[0] == g.inv_adj(u)[1])
			return partition_add_same_operands(u,g, update);
		
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		g.add_partition_id(u,step);
		
		type &lt = g.info(g.inv_adj(u)[0]).t;
		type &rt = g.info(g.inv_adj(u)[1]).t;
		type &ut = g.info(u).t;
		
		// create a new type for both operands and result and 
		// new type is same kind as highest level of old type
		// from old<T> to old<recursive_copy(old<T>)> or old<old<T>>
		type *newl = new type(lt);
		type *newr = new type(rt);
		type *newu = new type(ut);
		
		delete lt.t;
		delete rt.t;
		delete ut.t;
		
		lt.t = newl;
		rt.t = newr;
		ut.t = newu;
		
		lt.height = lt.t->height + 1;
		rt.height = rt.t->height + 1;
		ut.height = ut.t->height + 1;
			
		lt.dim.step = step;
		rt.dim.step = step;
		ut.dim.step = step;
		
		lt.dim.dim = lt.t->dim.dim;
		lt.t->dim.dim = step;
		rt.dim.dim = rt.t->dim.dim;
		rt.t->dim.dim = step;
		ut.dim.dim = ut.t->dim.dim;
		ut.t->dim.dim = step;
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

bool check_partition_add(vertex u, graph& g) {
	// in addition both operands and results is partitioned
	if (g.info(u).op == add || g.info(u).op == subtract)
		return true;
	return false;
}

bool partition_add_col(vertex u, graph& g, bool update) {
	if (g.info(u).op == add || g.info(u).op == subtract) {
		
		//if (g.inv_adj(u)[0] == g.inv_adj(u)[1])
		//	return partition_add_same_operands_row(u,g, update);
		
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		type &lt = g.info(g.inv_adj(u)[0]).t;
		type &rt = g.info(g.inv_adj(u)[1]).t;
		type &ut = g.info(u).t;
		
		type *modifyU = ut.get_highest_column();
		type *modifyL = lt.get_highest_column();
		type *modifyR = rt.get_highest_column();
		if (modifyU->k == scalar || modifyL->k == scalar || modifyR->k == scalar)
			return false;
		
		g.add_partition_id(u,step);
		
		type *newl = new type(lt.k, lt.dim, lt.s, lt.height);
		newl->t = lt.t;
		type *newr = new type(rt.k, rt.dim, rt.s, rt.height);
		newr->t = rt.t;
		type *newu = new type(ut.k, ut.dim, ut.s, ut.height);
		newu->t = ut.t;
		
		lt.t = newl;
		rt.t = newr;
		ut.t = newu;
		
		++lt.height;
		++rt.height;
		++ut.height;
		
		lt.k = modifyU->k;
		rt.k = modifyU->k;
		ut.k = modifyU->k;
		
		lt.dim = modifyU->dim;
		rt.dim = modifyU->dim;
		ut.dim = modifyU->dim;
		
		lt.dim.step = step;
		rt.dim.step = step;
		ut.dim.step = step;
		
		if (modifyU == &ut) {
			newl->dim.dim = step;
			newr->dim.dim = step;
			newu->dim.dim = step;
		}
		else {
			modifyL->dim.dim = step;
			modifyR->dim.dim = step;
			modifyU->dim.dim = step;
		}
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

bool check_partition_add_col(vertex u, graph& g) {
	// in addition both operands and results is partitioned
	if (g.info(u).op == add || g.info(u).op == subtract) {
		if ((g.info(u).t.get_highest_column())->k == scalar)
			return false;
		
		return true;
	}
	return false;
}

bool partition_add_row(vertex u, graph& g, bool update) {
	if (g.info(u).op == add || g.info(u).op == subtract) {
		
		//if (g.inv_adj(u)[0] == g.inv_adj(u)[1])
		//	return partition_add_same_operands_row(u,g, update);
		
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		type &lt = g.info(g.inv_adj(u)[0]).t;
		type &rt = g.info(g.inv_adj(u)[1]).t;
		type &ut = g.info(u).t;
		
		type *modifyU = ut.get_highest_row();
		type *modifyL = lt.get_highest_row();
		type *modifyR = rt.get_highest_row();
		if (modifyU->k == scalar || modifyL->k == scalar || modifyR->k == scalar)
			return false;
		
		g.add_partition_id(u,step);
		
		type *newl = new type(lt.k, lt.dim, lt.s, lt.height);
		newl->t = lt.t;
		type *newr = new type(rt.k, rt.dim, rt.s, rt.height);
		newr->t = rt.t;
		type *newu = new type(ut.k, ut.dim, ut.s, ut.height);
		newu->t = ut.t;
		
		lt.t = newl;
		rt.t = newr;
		ut.t = newu;
		
		++lt.height;
		++rt.height;
		++ut.height;
		
		lt.k = modifyU->k;
		rt.k = modifyU->k;
		ut.k = modifyU->k;
		
		lt.dim = modifyU->dim;
		rt.dim = modifyU->dim;
		ut.dim = modifyU->dim;
		
		lt.dim.step = step;
		rt.dim.step = step;
		ut.dim.step = step;
		
		if (modifyU == &ut) {
			newl->dim.dim = step;
			newr->dim.dim = step;
			newu->dim.dim = step;
		}
		else {
			modifyL->dim.dim = step;
			modifyR->dim.dim = step;
			modifyU->dim.dim = step;
		}
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

bool check_partition_add_row(vertex u, graph& g) {
	// in addition both operands and results is partitioned
	if (g.info(u).op == add || g.info(u).op == subtract) {
		if ((g.info(u).t.get_highest_row())->k == scalar)
			return false;
		
		return true;
	}
	return false;
}

bool part_mult_left_result(vertex u, graph &g, bool update) {
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	if (g.info(u).op == multiply && part_check(&l, &g.info(u).t, column, column)) {
		
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
				
		type &ut = g.info(u).t;
		
		// find highest column for left(lc) and u(uc)
		type *lc = &l;
		type *uc = &ut;

		lc = lc->get_highest_column();
		uc = uc->get_highest_column();

		if (uc == 0 || lc == 0)
			std::cout << "ERROR: partition.cpp: part_mult_left_result(): unexpected types\n";
		
		g.add_partition_id(u,step);
		
		// make copy of highest column(newuc, newlc) for new outermost type
		type *newcl = new type(*lc);
		type *newuc = new type(*uc);
		
		// new top level gets a size of old top level
		newuc->dim.step = step;
		newcl->dim.step = step;
		
		newuc->dim.dim = uc->dim.dim;
		uc->dim.dim = step;
		newcl->dim.dim = lc->dim.dim;
		lc->dim.dim = step;
		
		delete newcl->t;
		newcl->t = new type(l);
		newcl->height = l.height + 1;
		delete newuc->t;
		newuc->t = new type(ut);
		newuc->height = ut.height + 1;

		ut = *newuc;
		l = *newcl;
		
		delete newuc;
		delete newcl;
				
		if (update) {
			// push updated type information out to adjacent vertecies
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			//up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			// only the left side changes so only push that
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
		}
		
		//std::ofstream out1("lower1.dot");
	    //print_graph(out1, g);
	    
		return true;
	}
	return false;
}

bool check_part_mult_left_result(vertex u, graph &g) {
	// left operand and result can be partitioned
	if (g.inv_adj(u).size() != 2)
			return false;
		type &l = g.info(g.inv_adj(u)[0]).t;
		if (g.info(u).op == multiply && part_check(&l, &g.info(u).t, column, column))
		return true;
	return false;
}

bool part_mult_right_result(vertex u, graph &g, bool update) {
	if (g.inv_adj(u).size() != 2)
		return false;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (g.info(u).op == multiply && part_check(&g.info(u).t, &r, row, row)) {	

		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
				
		type &ut = g.info(u).t;
		// find higherst row for right(rr) and u(ur)
		type *rr = &r;
		type *ur = &ut;
		rr = rr->get_highest_row();
		ur = ur->get_highest_row();

		if (ur == 0 || rr == 0)
			std::cout << "ERROR: partition.cpp: part_mult_right_result(): unexpected types\n";
		
		g.add_partition_id(u,step);
		
		// make copy of highest row(newrr, newur) for new outermost type
		type *newrr = new type(*rr);
		type *newur = new type(*ur);	
		
		// new top level gets a size of old top level
		newur->dim.step = step;
		newrr->dim.step = step;
		
		newur->dim.dim = ur->dim.dim;
		ur->dim.dim = step;
		newrr->dim.dim = rr->dim.dim;
		rr->dim.dim = step;
		
		delete newrr->t;
		newrr->t = new type(r);
		newrr->height = r.height + 1;
		delete newur->t;
		newur->t = new type(ut);
		newur->height = ut.height + 1;

		ut = *newur;
		r = *newrr;
		
		if (update) {
			// push updated type information out to adjacent vertecies
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			//up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			// only the right side changes so only push that
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

bool check_part_mult_right_result(vertex u, graph &g) {
	// right operand and result can be partitions
	if (g.inv_adj(u).size() != 2)
		return false;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (g.info(u).op == multiply && part_check(&g.info(u).t, &r, row, row))
		return true;
	return false;
}

bool part_mult_scl_col(vertex u, graph &g, bool update) {
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if ((l.k == scalar && r.k != scalar) || (l.k != scalar && r.k == scalar)) {			
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		g.add_partition_id(u,step);
		
		// find non scalar operand
		vertex ns;
		type *t;
		if (l.k != scalar) {
			t = &l;
			ns = 0;
		}
		else {
			t = &r;
			ns = 1;
		}
				
		type &ut = g.info(u).t;
		
		type *modifyU = ut.get_highest_column();
		type *modifyNS = t->get_highest_column();
		if (modifyU->k != column || modifyNS->k != column)
			return false;
		
		type *newns = new type(ut.k, ut.dim, ut.s, ut.height);
		newns->t = t->t;
		type *newu = new type(ut.k, ut.dim, ut.s, ut.height);
		newu->t = ut.t;
		
		t->t = newns;
		ut.t = newu;
		
		++(t->height);
		++ut.height;
		
		t->k = modifyU->k;
		ut.k = modifyU->k;
		
		t->dim = modifyU->dim;
		ut.dim = modifyU->dim;
		
		t->dim.step = step;
		ut.dim.step = step;
		
		if (modifyU == &ut) {
			newns->dim.dim = step;
			newu->dim.dim = step;
		}
		else {
			modifyU->dim.dim = step;
			modifyNS->dim.dim = step;
		}
				
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[ns], g, up);
		}
		
		return true;
	}
	return false;
}
bool check_part_mult_scl_col(vertex u, graph &g) {
	// non scalar and result are partitioned
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (l.k == scalar && r.k != scalar) {
		if ((r.get_highest_column())->k != scalar)
			return true;
	}
	if (l.k != scalar && r.k == scalar) {
		if ((l.get_highest_column())->k != scalar)
			return true;
	}
	return false;
}


bool part_mult_scl_row(vertex u, graph &g, bool update) {
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if ((l.k == scalar && r.k != scalar) || (l.k != scalar && r.k == scalar)) {			
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		g.add_partition_id(u,step);
		
		// find non scalar operand
		vertex ns;
		type *t;
		if (l.k != scalar) {
			t = &l;
			ns = 0;
		}
		else {
			t = &r;
			ns = 1;
		}
		
		type &ut = g.info(u).t;
		
		type *modifyU = ut.get_highest_row();
		type *modifyNS = t->get_highest_row();
		if (modifyU->k != row || modifyNS->k != row)
			return false;
		
		type *newns = new type(ut.k, ut.dim, ut.s, ut.height);
		newns->t = t->t;
		type *newu = new type(ut.k, ut.dim, ut.s, ut.height);
		newu->t = ut.t;
		
		t->t = newns;
		ut.t = newu;
		
		++(t->height);
		++ut.height;
		
		t->k = modifyU->k;
		ut.k = modifyU->k;
		
		t->dim = modifyU->dim;
		ut.dim = modifyU->dim;
		
		t->dim.step = step;
		ut.dim.step = step;
		
		if (modifyU == &ut) {
			newns->dim.dim = step;
			newu->dim.dim = step;
		}
		else {
			modifyU->dim.dim = step;
			modifyNS->dim.dim = step;
		}
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[ns], g, up);
		}
		
		return true;
	}
	return false;
}
bool check_part_mult_scl_row(vertex u, graph &g) {
	// non scalar and result are partitioned
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (l.k == scalar && r.k != scalar) {
		if ((r.get_highest_row())->k != scalar)
			return true;
	}
	if (l.k != scalar && r.k == scalar) {
		if ((l.get_highest_row())->k != scalar)
			return true;
	}
	return false;
}

bool part_mult_scl(vertex u, graph &g, bool update) {
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if ((l.k == scalar && r.k != scalar) || (l.k != scalar && r.k == scalar)) {			
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
		
		g.add_partition_id(u,step);
		
		// find non scalar operand
		vertex ns;
		type *t;
		if (l.k != scalar) {
			t = &l;
			ns = 0;
		}
		else {
			t = &r;
			ns = 1;
		}
	
		type &ut = g.info(u).t;
		
		type *newns = new type(*t);
		type *newu = new type(ut);
		
		// new top level gets a size of old top level
		newu->dim.step = step;
		newns->dim.step = step;
		
		newu->dim.dim = ut.dim.dim;
		ut.dim.dim = step;
		newns->dim.dim = t->dim.dim;
		t->dim.dim = step;
		
		delete newns->t;
		newns->t = new type(*t);
		newns->height = t->height + 1;
		delete newu->t;
		newu->t = new type(ut);
		newu->height = ut.height + 1;

		ut = *newu;
		*t = *newns;	
		
		if (update) {
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			update_graph_dispatch(u, g, up);
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[ns], g, up);
		}
		
		return true;
	}
	return false;
}
bool check_part_mult_scl(vertex u, graph &g) {
	// non scalar and result are partitioned
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if ((l.k == scalar && r.k != scalar) || (l.k != scalar && r.k == scalar))
		return true;
	return false;
}

bool check_part_mult_left_right(vertex u, graph &g) {
	// left and right operand can be partitions
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (g.info(u).op == multiply && part_check(&l,&r,row,column))
		return true;
	return false;
}

bool part_mult_left_right(vertex u, graph &g, bool update) {
	// left and right operand can be partitions
	if (g.inv_adj(u).size() != 2)
		return false;
	type &l = g.info(g.inv_adj(u)[0]).t;
	type &r = g.info(g.inv_adj(u)[1]).t;
	if (g.info(u).op == multiply && part_check(&l, &r, row, column)) {
		suid++;
		string step = "$$" + boost::lexical_cast<string>(suid);
				
		// find higherst row for left(lr) anr column for right(rc)
		type *lr = &l;
		type *rc = &r;
		lr = lr->get_highest_row();
		rc = rc->get_highest_column();

		if (lr == 0 || rc == 0)
			std::cout << "ERROR: partition.cpp: part_mult_right_result(): unexpected types\n";
		
		g.add_partition_id(u,step);
		
		// make copy of highest row(newlr, newrc) for new outermost type
		type *newlr = new type(*lr);
		type *newrc = new type(*rc);	
		
		// new top level gets a size of old top level
		newlr->dim.step = step;
		newrc->dim.step = step;
		
		newlr->dim.dim = lr->dim.dim;
		lr->dim.dim = step;
		newrc->dim.dim = rc->dim.dim;
		rc->dim.dim = step;
		
		delete newrc->t;
		newrc->t = new type(r);
		newrc->height = r.height + 1;
		delete newlr->t;
		newlr->t = new type(l);
		newlr->height = l.height + 1;
		
		l = *newlr;
		r = *newrc;

		delete newlr;
		delete newrc;
		
		if (update) {
			// no change to result.. only need to push information up
			vector<bool> *ud = new vector<bool>(g.num_vertices(), false);
			vector<bool> &up = *ud;
			//up[u] = true;
			up[g.inv_adj(u)[0]] = true;
			up[g.inv_adj(u)[1]] = true;
			
			// push changes up the graph
			update_graph_dispatch(g.inv_adj(u)[0], g, up);
			update_graph_dispatch(g.inv_adj(u)[1], g, up);
		}
		
		return true;
	}
	return false;
}

////////////////////////////////// END PARTITIONERS /////////////////////////////////////////////
