#ifndef GENERATE_CODE_HPP
#define GENERATE_CODE_HPP

#include <map>
#include <string>
#include "modelInfo.hpp"
#include "compile.hpp"

void generate_code(string out_file_name, int vid, graph &g, string message,
    int baseDepth, int threadDepth, string routine_name, bool noptr,
    map<string, type*>& inputs, map<string, type*>& outputs,
    modelMsg &msg, bool mpi, code_generation codegen_type);

#endif  // GENERATE_CODE_HPP
