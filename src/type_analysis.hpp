#ifndef TYPE_ANALYSIS_HPP_
#define TYPE_ANALYSIS_HPP_

#include <string>
#include "syntax.hpp"

////////////////// MANUAL TYPE ANALYSIS ////////////////////////////////////////
void compute_types(graph &g);

////////////////// YICES COMMAND LINE /////////////////////////////////////////
void generate_constraints(graph &g);

////////////////// UTILS //////////////////////////////////////////////////////
string prnt(type *t);
string prnt_detail(type *t);

#endif /*TYPE_ANALYSIS_HPP_*/
