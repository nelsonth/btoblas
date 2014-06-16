#ifndef GRAPH_QUERY_HPP
#define GRAPH_QUERY_HPP

#include <vector>
#include <string>
#include "syntax.hpp"

void typeToQuery(string id, vertex v, type *t, std::ofstream &fout, op_code op);

void graphToQuery_r(graph &g, std::ofstream &fout, vertex v,
                    vector<string> &connect);

void graphToFusionQuery_r(subgraph *sg, std::ofstream &fout,
                          vector<string> &connect);

void graphToQuery(graph &g, string fileName, unsigned int vid,
                  unsigned int threadDepth);

#endif  // GRAPH_QUERY_HPP
