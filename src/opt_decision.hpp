#ifndef OPT_DECISION_HPP
#define OPT_DECISION_HPP

#include <iostream>
#include <boost/config.hpp>
#include <vector>
#include <algorithm>
#include <utility>
#include <ctime>
#include <fstream>
#include <sstream>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_smallint.hpp>
#include "syntax.hpp"
#include "work.hpp"
#include "modelInfo.hpp"
#include "type_analysis.hpp"
#include "generate_code.hpp"
#include "enumerate.hpp"
#include "analyze_graph.hpp"
#include "partition.hpp"
#include "build_graph.hpp"
#include "search_exhaustive.hpp"
#include "optimize_analysis.hpp"
#include "compile.hpp"

using namespace std;

struct PartitionChoice {
	Way way;
	int blocksize; // currently no way to specify # blocks, only block size
};

/*
 * This function should apply each PartitionChoice to each vector in order, so if
 * vec = partitions[i]
 * it will do the partitioning vec[0] to g[i], then vec[1], etc.
 * if a contradiction occurs, return false
 */
bool buildPartition(graph *g, map<vertex, vector<PartitionChoice> > &partitions,
					build_details_t &bDetails);

/* vertex property = subgraph majorID, edge property = fuse depth */
typedef boost::adjacency_matrix <boost::undirectedS, unsigned int, unsigned int, boost::no_property> FuseGraph;
typedef boost::graph_traits<FuseGraph>::edge_descriptor FuseEdge;
typedef boost::graph_traits<FuseGraph>::vertex_descriptor FuseVertex;
typedef boost::graph_traits<FuseGraph>::edge_iterator edge_iter;
typedef boost::graph_traits<FuseGraph>::out_edge_iterator out_edge_iter;
typedef boost::graph_traits<FuseGraph>::vertex_iterator vertex_iter;
typedef boost::graph_traits<FuseGraph>::out_edge_iterator adj_edge_iter;


struct OperationInfo {
	vertex id;
	unsigned int maxFuseDepth;
};

struct ParamBound {
	unsigned int low;
	unsigned int high;
	unsigned int stride;
};

// Number of partitions to apply
// Should be 1 for threads, 2 for threads + cache tiling
extern int MAX_PARTS;

//Partition range to search over: low high step
extern int partition_bounds[32][3];

class OptPoint {
	public:
		FuseGraph fsgr;
		unsigned int num_nodes;
		vector<unsigned int> num_parts;
		vector< vector<PartitionChoice> > partitions;
		OptPoint(unsigned int n);
		unsigned int size();
		void set(unsigned int i, unsigned int val);
		void orthoSet(unsigned int i, unsigned int val);
		unsigned int get(unsigned int i);
		void print(vector<OperationInfo> &op_nodes);
};

class OptSpace {
	public:
		vector<OperationInfo> op_nodes;
		map<vertex, vector<PartitionChoice> > mapPartition(OptPoint *p);
		bool checkPoint(OptPoint *p);
		OptSpace(graph &g);
		unsigned int search_dimension;
		unsigned int max_fuse;
		ParamBound bounds(OptPoint point, unsigned int i);
};

void printSet(set<vertex> s);

int buildVersion(graph *g, 
				 map<vertex, vector<PartitionChoice> > &partitions,
				 FuseGraph fg, 
				 vector<unsigned int> num_parts,
				 vector<OperationInfo> &op_nodes,
                 int vid, bool strict,
                 build_details_t &bDetails);

double evaluateVersion(graph &g, int vid, int threadDepth,
                       compile_details_t &cDetails,
                       build_details_t &bDetails);

int random_search(int N, graph &g, graph &baseGraph, 
                  compile_details_t &cDetails,
                  build_details_t &bDetails);

int exhaustive_search(graph &g, graph &baseGraph, 
                      compile_details_t &cDetails,
                      build_details_t &bDetails);

int smart_exhaustive(graph &g, graph &baseGraph, 
				compile_details_t &cDetails,
                build_details_t &bDetails);

int orthogonal_search(graph &g, graph &baseGraph, 
                      compile_details_t &cDetails,
                      build_details_t &bDetails);

int smart_hybrid_search(graph &g, graph &baseGraph, 
                        compile_details_t &cDetails,
                        build_details_t &bDetails);

int debug_search(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
                 compile_details_t &cDetails,
                 build_details_t &bDetails);

double getPerformance(int vid, graph &g, graph &baseGraph, 
                      map<vertex, vector<PartitionChoice> > partitions,
                      FuseGraph fg, vector<unsigned int> num_parts,
                      vector<OperationInfo> &op_nodes,
                      bool strict,
                      compile_details_t &cDetails,
                      build_details_t &bDetails);
#endif //OPT_DECISION_HPP
