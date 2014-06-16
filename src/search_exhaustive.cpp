#include "work.hpp"
#include "search_exhaustive.hpp"
#include "enumerate.hpp"
#include "type_analysis.hpp"
#include <list>
#include <iostream>
#include <fstream>
#include "compile.hpp"
#include "generate_code.hpp"
// #include "modelInfo.hpp"
#include "build_graph.hpp"

using namespace std;

void getDependencies(graph &g) {
	// find dependencies such as
	// row0 < row1 < scl >>
	// the size of row1 must be <= the size of row0
	
	// in this process, check for incorrect 
	// dim, step, base_rowsm base_cols, lead_dim
	
	vertex v;
	for (v = 0; v < g.num_vertices(); ++v) {
		if (g.info(v).op == deleted)
			continue;
		
		type *t = &g.info(v).t;
		
		list<string> rows, cols;
		rows.clear();
		cols.clear();
		
		if (t->k == scalar) {
			continue;
			if (t->dim.dim.compare("1") != 0
				|| t->dim.step.compare("1") != 0
				|| t->dim.base_rows.compare("1") != 0
				|| t->dim.base_cols.compare("1") != 0
				|| t->dim.lead_dim.compare("1") != 0) {
				std::cout << "Scalar error\n";
				std::cout << "\t" << t->dim.dim << "," << t->dim.step
				<< "," << t->dim.base_rows << "," 
				<< t->dim.base_cols << "," << t->dim.lead_dim
				<< "\n";
			}
		}
		else {
			if (t->k == row)
				rows.push_front(t->dim.dim);
			else
				cols.push_front(t->dim.dim);
		}
		
		string base_rows = t->dim.base_rows;
		string base_cols = t->dim.base_cols;
		string lead_dim = t->dim.lead_dim;
		t = t->t;
		
		while (t->k != scalar) {
			if (t->k == row)
				rows.push_front(t->dim.dim);
			else
				cols.push_front(t->dim.dim);
			
			if (base_rows.compare(t->dim.base_rows) != 0
				|| base_cols.compare(t->dim.base_cols) != 0
				|| lead_dim.compare(t->dim.lead_dim) != 0) {
				std::cout << "ERROR " << v << "\n";
				std::cout << prnt_detail(&(g.info(v).t)) << "\n";
			}
			t = t->t;
		}

		// 2 things to achieve
		// 1) ensure threads are divided correctly, for example consider
		//			gemm partitioned three ways, the code that divides
		//			up the threads must know there is an operation that
		//			operates as thread#M * thread#N * thread#K
		// 2) ensure sizes are same, specifically consider a single
		//			dimension partitioned more than once.  The inner
		//			partition size must be less than or equal to the
		//			outer partition size.
		//
		// 2 questions to answer for this
		// 1) is a matrix partitioned in more than one dimension
		//			rows.size() && cols.size() > 1
		// 2) is any operation partitioned more than once in a given
		//      dimension.  This is operation specific.

		// does this need to be thread/tile specific?
		/*
		 std::cout << v << "\n\trows: ";
		 list<string>::iterator i;
		 for (i = rows.begin(); i != rows.end(); ++i)
		 std::cout << *i << "\t";
		 std::cout << "\n\tcols: ";
		 for (i = cols.begin(); i != cols.end(); ++i)
		 std::cout << *i << "\t";
		 std::cout << "\n\n";
		 */
	}
}


void dump_bit_strings(graph &g, vector<vector<int> > &fusion_options) {
	vector<vertex> ops;
	map<vertex, vector<subgraph*> > sgMap;
	for (vertex v = 0; v < g.num_vertices(); ++v) {
		if (g.info(v).op != add && g.info(v).op != subtract &&
			g.info(v).op != multiply)
			continue;
		
		ops.push_back(v);
		
		subgraph *p = g.find_parent(v);
		sgMap[v] = vector<subgraph*>();
		
		while (p) {
			sgMap[v].insert(sgMap[v].begin(),p);
			p = p->parent;
		}
		
	}
	
	vector<pair<pair<vertex, vertex>, int> > edges;
	vector<vertex> done;
	map<vertex, vector<subgraph*> >::iterator sgItr0 = sgMap.begin();
	map<vertex, vector<subgraph*> >::iterator sgItr1;
	for (; sgItr0 != sgMap.end(); ++sgItr0) {
		if (find(done.begin(),done.end(),sgItr0->first) != done.end())
			continue;
		
		done.push_back(sgItr0->first);

		sgItr1 = sgItr0;
		for (++sgItr1; sgItr1 != sgMap.end(); ++sgItr1) {
			
			if (find(done.begin(),done.end(),sgItr1->first) != done.end())
				continue;
			
			vector<subgraph*> &vp0 = sgItr0->second;
			vector<subgraph*> &vp1 = sgItr1->second;
			int fuseDepth = 0;
			
			int size = vp0.size() < vp1.size() ? vp0.size() : vp1.size();
			for (int i = 0; i < size; ++i) {
				if (vp0[i] != vp1[i])
					break;
				
				fuseDepth++;
			}
			
			if (fuseDepth) {
				done.push_back(sgItr1->first);
				pair<vertex,vertex> e = pair<vertex,vertex>(sgItr0->first,sgItr1->first);
				edges.push_back(pair<pair<vertex,vertex>, int>(e,fuseDepth));
			}
			
		}
	}
	vector<int> v;
	
	vector<pair<pair<vertex, vertex>, int> >::iterator eItr;
	vector<vertex>::iterator opItr0 = ops.begin();
	vector<vertex>::iterator opItr1;
	bool found;
	for (; opItr0 != ops.end(); ++opItr0) {
		opItr1 = opItr0;
		for (++opItr1; opItr1 != ops.end(); ++opItr1) {
			found = false;
			for (eItr = edges.begin(); eItr != edges.end(); ++eItr) {
				if ((eItr->first.first == *opItr0 && eItr->first.second == *opItr1) || 
					(eItr->first.first == *opItr1 && eItr->first.second == *opItr0)) {
					
					found = true;
					v.push_back(eItr->second);
				}
			}
			if (!found) {
				v.push_back(0);
			}
		}
	}
	fusion_options.push_back(v);
}


int search_exhaustive_old(graph &g, std::stack<work_item> s, 
					  vector<optim_fun_chk> const &checks,
					  vector<optim_fun> const &optimizations,
					  vector<rewrite_fun> & rewrites,
					  vector<algo> &algos,
					  string out_file_name,
					  bool modelOff,
					  modelMsg &msg,
					  vector<model_typ> &models,
					  std::list<versionData*> &orderedVersions,
					  double threshold,
					  string routine_name,
					  map<string,type*>& inputs, 
					  map<string,type*>& outputs,
					  int baseDepth,
					  bool noptr,
					  bool mpi,
					  code_generation codetype) {
	int threadDepth = 0;
	
	
	bool printBitString = false;
	std::ofstream fout;
	if (printBitString) {
		fout.open((out_file_name + "_bit_string.csv").c_str());
	}
				  
	/////////// NEW STACK
	// this is a new graph so I can blindly cleanup
	// if the original graph is newed to start with
	// this can change
	graph *newG = new graph(g);
	work_item w(newG, 0,0, "",1);
	s.push(w);
	
	int vid = 1;		// unique version id
	
	while (! s.empty()) {
		
		work_item current = s.top();
		s.pop();
		
		// Work items either represent code to be generated or graphs
		// that have not been completely evaluated for loop merging combintations.
		// If this work item represents a graph that has been completely 
		// evaluated, generate code, otherwise create work items for each
		// new optimization
		if (!enumerate_loop_merges(s, current, checks, optimizations, rewrites,
								   algos)) {
			// all nesting levels have been evaluated
			
			// find dependencies and check types
			// this is easier to do before lower and as long as no change
			// occurs during optimization, it should be ok to perform this
			// earlier.  IF TYPE INFORMATION STARTS LOOKING WRONG TURN
			// THIS BACK ON TO PERFORM SOME SANITY CHECKING
			//getDependencies(*current.g);
			
			if (printBitString) {
				//dump_bit_strings(*current.g, fout);
			}
			
			// generate code
			generate_code(out_file_name, vid, *current.g, current.history,
						  baseDepth, threadDepth, routine_name, noptr,
						  inputs, outputs,
						  msg, mpi, codetype);
			
			
			//////////////////// models ///////////////////
			
			versionData *verData = new versionData(vid);
			
			// if (!modelOff) {
				// // model is on
				// CLEAR_MODEL_TIME;
				// msg.threadDepth = threadDepth;
				// modelVersion(orderedVersions, verData, models, *current.g, msg,
							 // threshold,vid, routine_name);
				// GET_MODEL_TIME;
			// }
			// else {
				// model is off
				orderedVersions.push_back(verData);				
			// }
			/////////////////// end models /////////////////////
			
			// clean up work item
			current.del();
			
			vid++;
			
			continue;
		}
		
		// clean up work item
		current.del();
	}
	
	if (printBitString) {
		fout.close();
	}
	
	return vid;
}


int get_fusion_options(graph &g, 
					   vector<vector<int> > &fusion_options,
                       build_details_t &bDet) {
	
    vector<optim_fun_chk> const &checks = *bDet.checks;
    vector<optim_fun> const &optimizations = *bDet.optimizations;
    vector<rewrite_fun> & rewrites = *bDet.rewrites;
    vector<algo> &algos = *bDet.algos;
    
	bool printBitString = true;
				  
	graph *newG = new graph(g);
	std::stack<work_item> s;
	work_item w(newG, 0,0, "",1);
	s.push(w);
	
	while (! s.empty()) {

		work_item current = s.top();
		s.pop();
		
		if (!enumerate_loop_merges(s, current, checks, optimizations, rewrites,
								   algos)) {
			if (printBitString) {
				dump_bit_strings(*current.g, fusion_options);
			}
			
		}
		
		current.del();
	}
    return 0;
}
