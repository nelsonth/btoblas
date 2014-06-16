#ifndef PARTITION_HPP
#define PARTITION_HPP

#include "syntax.hpp"
#include <stack>
#include "work.hpp"
#include "opt_decision.hpp"

//////////////////// HELPERS //////////////////////////////////////////////////
void update_graph(vertex u, graph &g, vector<bool> &up);
void update_mult(vertex u, graph &g, vector<bool> &up);
void check_depth(int curr, int &max_depth, vector<subgraph*> subs);
string scrub_dollar(string s, string replace);

//////////////////// FIND PARTITIONING ///////////////////////////////////////
void find_partitioning(graph& g, std::stack<work_item>& s,
		  vector<rewrite_fun> const &check, vector<partition_fun> const &optimizations, vector<algo> const &algos,
		  vector<rewrite_fun> const &rewrites, int min_depth, int max_depth);

//////////////////// PARTITIONERS ////////////////////////////////////////////
bool partition_add(vertex u, graph& g, bool update);
bool check_partition_add(vertex u, graph& g);

bool partition_add_row(vertex u, graph& g, bool update);
bool check_partition_add_row(vertex u, graph& g);

bool partition_add_col(vertex u, graph& g, bool update);
bool check_partition_add_col(vertex u, graph& g);

bool part_mult_left_result(vertex u, graph &g, bool update);
bool check_part_mult_left_result(vertex u, graph &g);

bool part_mult_right_result(vertex u, graph &g, bool update);
bool check_part_mult_right_result(vertex u, graph &g);

bool part_mult_scl(vertex u, graph &g, bool update);
bool check_part_mult_scl(vertex u, graph &g);

bool part_mult_scl_col(vertex u, graph &g, bool update);
bool check_part_mult_scl_col(vertex u, graph &g);

bool part_mult_scl_row(vertex u, graph &g, bool update);
bool check_part_mult_scl_row(vertex u, graph &g);

bool part_mult_left_right(vertex u, graph &g, bool update);
bool check_part_mult_left_right(vertex u, graph &g);

struct partition_t {
	// map partition dependencies as
	// size0 -> size1 with the relationship of size0 > size1
	// an empty map does not mean that there is no partitioning
	// there could still be many partitions that have no
	// size relation ship between them
	map<string,string> partitionDeps;
	
	// for each operation vertex, specify the dimensionality of the partitioning.
	// for add/sub this can be 0,1,2 where M and/or N can be partitioned
	// for multiply this can be 0,1,2,3 where M,N,K can be partitioned.
	map<vertex,int> dimensions;
    
	// names and number of unique partitions. this represents the number
	// of partitions that require parameters.
	set<string> unique;
	
	// map the id given to each partition to a size
	// NOTE: this is not created until end of optimization process.
	map<string,int> sizeMap;
};

void get_partition_information(graph &g);
void update_partition_information(graph &g);
bool map_partitions_to_sizes(graph &g, map<vertex, vector<struct PartitionChoice> > &partitions);
#endif /*PARTITION_HPP_*/
