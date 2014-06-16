#ifndef TRANSLATE_TO_CODE_HPP
#define TRANSLATE_TO_CODE_HPP

#include <iostream>
#include <string>
#include <map>
#include <set>
#include "syntax.hpp"
#include "translate_utils.hpp"

struct partitionInfo {
	partitionInfo(string size, subgraph* sg) :
	parentSize(size) {reduction.push_back(sg);}
	
	partitionInfo() {}
	
	string parentSize;
	// this may be useful, so Im setting it up but its unused
	vector<subgraph*> reduction;			// any subgraph that makes uses of this
											// is it performing a reduction?
};

bool been_cleared(vertex u, graph &g, int min_depth);

void translate_to_cpp(std::ostream& out,
            std::string name,
            std::map<std::string,type*>const& inputs, 
            std::map<std::string,type*>const& outputs, 
            graph& g);

// top level function that will create function entry
void translate_to_intrin(std::ostream& out,
					  std::string name,
					  std::map<std::string,type*>const& inputs, 
					  std::map<std::string,type*>const& outputs, 
					  graph& g);

void translate_to_noPtr(std::ostream& out,
						 std::string name,
						 std::map<std::string,type*>const& inputs, 
						 std::map<std::string,type*>const& outputs, 
						 graph& g);

// low level function that just translates code
string translate_graph_intrin(std::ostream& out,
							 subgraph* current,
							 vector<vertex> const& vertices,
							 vector<subgraph*> const& subgraphs,
							 graph& g);
// used to create top level temporary structures
string translate_tmp_intrin(std::ostream& out, graph &g, vertex u);
// used to declare variables and pointers
string translate_declareVariables_intrin(std::ostream& out, graph &g, vertex u, bool parallelReduce);

// top level function that will create function entry
void translate_to_pthreads(std::ostream& out,
						 std::string name,
						 std::map<std::string,type*>const& inputs, 
						 std::map<std::string,type*>const& outputs, 
						 graph& g,
						 int threadDepth, int numThreads);

void translate_to_mpi(std::ostream& out,
						 std::string name,
						 std::map<std::string,type*>const& inputs, 
						 std::map<std::string,type*>const& outputs, 
						 graph& g,
						 int threadDepth, int numThreads);

void translate_to_cuda(std::ostream& out,
            std::string name,
            std::map<std::string,type*>const& inputs, 
            std::map<std::string,type*>const& outputs, 
            graph& g);


void init_partitions(graph &g, vector<subgraph*> &subs, std::ostream &out);
std::string get_next_elem(type in);
std::string get_next_elem_stride(type in);



#endif // TRANSLATE_TO_C_HPP

