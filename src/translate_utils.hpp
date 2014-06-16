#ifndef TRANSLATE_UTILS_HPP
#define TRANSLATE_UTILS_HPP

#include <map>
#include <deque>
#include <iostream>
#include <fstream>
#include <vector>
#include "boost/next_prior.hpp"
#include "build_graph.hpp"

using std::vector;


int depth(subgraph* sg);
bool ancestor(subgraph* a, subgraph* p);
subgraph* get_child_ancestor(subgraph* a, subgraph* c);
void topo_sort_r(vertex u, graph& g, std::deque<vertex>& order, vector<bool>& visited);
void topo_sort(graph& g, std::deque<vertex>& order);
void order_subgraphs(std::deque<vertex>& order, 
        std::map<vertex,subgraph*>& new_sub, 
        std::map<vertex,vertex>& new_old,
        subgraph* current,
        vector<vertex> const& vertices,
        vector<subgraph*> const& subgraphs,
        graph& g);
void order_subgraphs_wrapper(vector<subgraph*> &sgorder, subgraph *sg, graph &g);

string expr_of(vertex u, graph& g, subgraph* cur);
string type_to_c(type t, bool byref=false);
string type_to_noPtr(type &t, string name, bool byref=false);
string size_params(type &t);
string size_params_no_type(type &t);
string container_size(vertex u, graph const& g);
string container_size_type(type &t);


enum signature_t { 
	typeOnly,					// (double, int, double*, ...)
	nameOnly,					// (alpha, y, A, ...)
	typeWithName,				// (double alpha, int y, double *A, ...)
								// (double alpha, int y, double A[y], ...)
};
//generates the function args, i.e. (double *A, int A_ncols....)
//diffout = used so blas can send to different outputs, otherwise ""
std::string function_args(map<string,type*> const &inputs, 
		map<string, type*> const &outputs, 
		map<string,pair<vertex,type> > &data, 
		std::string diffout,
		signature_t sig,
		bool noPtr);
std::string make_function_signature(string name, map<string,type*>const& inputs, 
									map<string,type*>const& outputs, 
									map<string,pair<vertex,type> >& data,
									string diffout,
									enum signature_t sig,
									bool noPtr);

string expr_of_noPtr(vertex u, graph& g, subgraph* cur, 
					 std::map<unsigned int, std::pair<string, string> > &);

#endif // TRANSLATE_UTILS_HPP
