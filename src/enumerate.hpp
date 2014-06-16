#ifndef ENUMERATE_HPP_
#define ENUMERATE_HPP_

#include <stack>
#include "syntax.hpp"

bool enumerate_loop_merges(std::stack<work_item> &s,
						   work_item &current,
						   vector<optim_fun_chk> const &checks,
						   vector<optim_fun> const &optimizations,
						   vector<rewrite_fun> & rewrites,
						   vector<algo> &algos);

void get_current_subgraphs(graph &g, int nesting, vector<subgraph*> & subs);

void enumerate(map<subgraph_id,set<subgraph_id> > common_itr,
		map<subgraph_id,set<subgraph_id> > &depend,
		vector<set<subgraph_id>* > &mergable);

void map_subgraphs(subgraph_id map_from, subgraph* map_to, 
		map<subgraph_id, subgraph_id> &sg_map);

void evaluate_edges(subgraph_id curr, subgraph_id curr_parent, subgraph *working,
		vector<subgraph*> &sgs, map<subgraph_id, subgraph_id> &sg_map, 
		graph &g, map<subgraph_id,set<subgraph_id> > &depend);

void generate_work_items(graph &g, vector<set<subgraph_id>* > &mergable,
						  vector<vertex> &workingVertices,
						  vector<optim_fun_chk> const &checks,
						  vector<optim_fun> const &optimizations,
						  std::stack<work_item> &s,
						  work_item &old_wi,
						  vector<rewrite_fun> & rewrites,
						  vector<algo> &algos);

int rewrite_graph(graph *g, std::set<subgraph_id> toMerge,
				   std::vector<vertex> &workingVertices,
				   vector<optim_fun_chk> const &checks,
				   vector<optim_fun> const &optimizations,
				   vector<rewrite_fun> & rewrites,
				   std::vector<algo> &algos);

#endif /*ENUMERATE_HPP_*/


