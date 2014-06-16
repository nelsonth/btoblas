#include "graphQuery.hpp"
#include <fstream>
#include <iostream>

void typeToQuery(string id, vertex v, type *t, std::ofstream &fout, 
				 op_code op) {	
	
	fout << v << ";" << id << ";" << op << "&&"; 
	
	while (t) {
		
		fout << t->height << ";";
		
		if (t->t) {
			int part = t->t->whichPartition(t->dim.step, t->k);
			if (part > 0)
				fout << part << ";";
			else 
				fout << ";";
		}
		else 
			fout << ";";
		
		if (t->k == row)
			fout << "row";
		else if (t->k == column)
			fout << "col";
		else if (t->k == scalar)
			fout << "scl";
		fout << ";";
		
		fout << t->dim.dim;
		
		t = t->t;
		if (t)
			fout << "::";
	}
	fout << "\n";
}

void graphToQuery_r(graph &g, std::ofstream &fout, vertex v, 
					vector<string> &connect) {
	
	string toMe;
	if (connect.size() > 0) {
		toMe = string(connect.back());
		connect.erase(connect.end());
	}
	else { 
		toMe = "";
	}
	
	string withMe = toMe + boost::lexical_cast<string>(v) + ",";
	
	if (g.adj(v).size() == 0) {
		//if (g.find_parent(v) == 0 && g.info(v).op != input) {
		connect.push_back(withMe);
		withMe = boost::lexical_cast<string>(v) + ",";
	}
	
	for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
		vertex l = g.adj(v)[i];
		
		connect.push_back(withMe);
		graphToQuery_r(g,fout,l,connect);
	}
}

void graphToFusionQuery_r(subgraph *sg, std::ofstream &fout, vector<string> &connect) {
	string toMe;
	if (connect.size() > 0) {
		toMe = string(connect.back());
		connect.erase(connect.end());
	}
	else { 
		toMe = "";
	}
	
	string withMe = toMe + sg->str_id + ",";
	
	if (sg->subs.size() == 0) {
		connect.push_back(withMe);
	}
	
	fout << sg->str_id << "::";
	for (unsigned int i = 0; i < sg->vertices.size(); ++i)
		fout << sg->vertices[i] << ",";
	
	fout << "\n";
	
	for (unsigned int i = 0; i < sg->subs.size(); ++i) {
		connect.push_back(withMe);
		graphToFusionQuery_r(sg->subs[i], fout, connect);
	}
}

void graphToQuery(graph &g, string fileName,unsigned int vid, unsigned int threadDepth) {
	std::ofstream fout((fileName + "__q" + boost::lexical_cast<string>(vid) 
						+ ".bto").c_str());
	
	fout << "## ops\n";
	for (unsigned int i = 0; i < 22; ++i)
		fout << i << "," << op_to_name((op_code)i) << ";";
	fout << "\n";
	
	fout << "## thread depth\n";
	fout << threadDepth << "\n";
	
	fout << "## vertices\n";
	std::set<string> connections; 
	for (vertex v = 0; v < g.num_vertices(); ++v) {
		if (g.info(v).op == input) {
			vector<string> connect;
			graphToQuery_r(g,fout,v,connect);
			
			for (unsigned int i = 0; i < connect.size(); ++i) {
				connections.insert(connect[i]);
			}
		}
		if (g.info(v).op != deleted) {
			string lbl = g.info(v).label;
			if (lbl.compare("") == 0)
				lbl = "t"+boost::lexical_cast<string>(v);
			
			typeToQuery(lbl, v, &g.info(v).t, fout, g.info(v).op);
		}
	}
	
	fout << "## vertex connections\n";
	std::set<string>::iterator i;
	for (i = connections.begin(); i != connections.end(); ++i) {
		fout << *i << "\n";
	}
	
	fout << "##\n\n## Loops\n";
	fout << "0::";
	for (vertex v = 0; v < g.num_vertices(); ++v) {
		if (g.info(v).op != deleted && g.find_parent(v) == 0)
			fout << v << ",";
	}
	fout << "\n";
	
	connections.clear();
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		vector<string> connect;
		graphToFusionQuery_r(g.subgraphs[i],fout, connect);
		
		for (unsigned int i = 0; i < connect.size(); ++i) {
			connections.insert(connect[i]);
		}
	}
	
	fout << "## loop connections\n";
	for (i = connections.begin(); i != connections.end(); ++i) {
		fout << *i << "\n";
	}
	fout << "##\n\n";
	fout.close();
}
