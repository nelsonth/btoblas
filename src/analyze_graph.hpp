#ifndef ANALYZE_GRAPH_HPP
#define ANALYZE_GRAPH_HPP

#include <vector>
#include "syntax.hpp"

void init_algos(std::vector<algo>& algos);
void assign_algorithms(vector<algo> const& algos, graph& g);
void assign_work(vector<algo> const& algos, graph& g);
void reintroduce_variables(graph& g);
void initial_lower(graph &g, vector<algo> const &algos, vector<rewrite_fun> const &rewrites);
void lower_graph(vector<algo> const& algos, graph& g);
void apply_rewrites(vector<rewrite_fun>const& rewrites, graph& g);
std::vector<unsigned int> find_matches(vertex u, graph &g, vector<algo> const& algos);
void apply_match(unsigned int i, vertex u, graph &g, vector<algo> const& algos);

bool total_type_match_with_size(type &t0, type &t1);
bool total_type_match(type &t0, type &t1);

string getLoopStep(type *t);

/////////////////// REWRITERS - ALWAYS PERFORM /////////////////////////////////
bool flip_transpose(vertex u, graph& g);
bool flip_transpose_stride(vertex u, graph& g);
bool merge_tmp_output(vertex u, graph& g);
bool merge_tmp_cast_output(vertex u, graph& g);
bool remove_intermediate_temporary(vertex u, graph& g);
bool reduce_reductions(vertex u, graph& g);
bool merge_gets(vertex u, graph& g);
bool merge_sumto_store(vertex u, graph &g);
bool move_temporary(vertex u, graph &g);
bool remove_cast_to_output(vertex u, graph &g);
bool remove_input_to_cast(vertex u, graph &g);
bool remove_cast_to_cast(vertex u, graph &g);
bool merge_same_cast(vertex u, graph &g);
bool straighten_top_level_scalar_ops(vertex u, graph& g);

//////////////////// UNUSED ///////////////////////////////////////////////////
bool lower_stores(vertex u, graph& g);
bool merge_stores(vertex u, graph& g);
bool remove_deadends(vertex u, graph& g);
bool remove_useless_store(vertex u, graph& g);


#endif  // ANALYZE_GRAPH_HPP
