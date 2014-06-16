#ifndef SEARCH_EXHAUSTIVE_HPP
#define SEARCH_EXHAUSTIVE_HPP

#include <list>
#include <map>
#include <string>
#include <vector>
#include <stack>
#include "modelInfo.hpp"
#include "compile.hpp"

int search_exhaustive_old(
    graph &g, std::stack<work_item> s, vector<optim_fun_chk> const &checks,
    vector<optim_fun> const &optimizations, vector<rewrite_fun> & rewrites,
    vector<algo> &algos, string out_file_name, bool modelOff, modelMsg &msg,
    vector<model_typ> &models, std::list<versionData*> &orderedVersions,
    double threshold, string routine_name, map<string, type*>& inputs,
    map<string, type*>& outputs, int baseDepth, bool noptr, bool mpi);

int get_fusion_options(graph &g, vector<vector<int> > &fusion_options,
                       build_details_t &bDetails);

#endif  // SEARCH_EXHAUSTIVE_HPP
