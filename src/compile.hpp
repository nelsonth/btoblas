#ifndef COMPILE_HPP
#define COMPILE_HPP

#include <string>
#include "boost/program_options.hpp"
#include "syntax.hpp"
#include "modelInfo.hpp"

// output graphs files (this is not dot files)
//#define DUMP_GRAPHS

// output dot file for each version
#define DUMP_DOT_FILES

// output vid : history string for each version.
#define DUMP_VERSION_STRINGS

// print any available data for model and empirical testing
//#define DUMP_DATA
// dump this data to file routine_name.csv when true
//or stdout when false
#define DUMP_DATA_TO_FILE true

// when MODEL_TIME is define, the time spent in the memory model will
// be displayed.  Requires USE_COST or USE_COST_SYM to be defined.
//#define MODEL_TIME

#ifdef MODEL_TIME
#define CLEAR_MODEL_TIME memmodelTimer.restart()
#define GET_MODEL_TIME memmodelTotal += memmodelTimer.elapsed()
#else
#define CLEAR_MODEL_TIME
#define GET_MODEL_TIME
#endif

enum code_generation {threadtile, cachetile, bothtile};

struct compile_details_t {
    string routine_name;
    string tmpPath;
    string fileName;
    
    bool runCorrectness;
    bool useModel;
    bool useEmpirical;
    
    bool noptr;
    bool mpi;

	code_generation codetype;
    
    map<string, type*> *inputs;
    map<string, type*> *inouts;
    map<string, type*> *outputs;
    
    compile_details_t(string rName, string tPath, string fName,
                      bool runC, bool useModel, bool useEmp,
                      bool noptr, bool mpi, code_generation ct,
                      map<string, type*> *in,
                      map<string, type*> *inout,
                      map<string, type*> *out) :
    routine_name(rName), tmpPath(tPath), fileName(fName),
    runCorrectness(runC), useModel(useModel), useEmpirical(useEmp),
    noptr(noptr), mpi(mpi), codetype(ct),
    inputs(in), inouts(inout), outputs(out) {}
};

struct build_details_t {
    vector<rewrite_fun> *part_checks;
    vector<partition_fun> *partitioners;
    
    vector<optim_fun_chk> *checks;
    vector<optim_fun> *optimizations;
    
    vector<algo> *algos;
    vector<rewrite_fun> *rewrites;
    
    vector<model_typ> *models;
    modelMsg *modelMessage;
    
    build_details_t(vector<rewrite_fun> *partCheck,
                    vector<partition_fun> *partition,
                    vector<optim_fun_chk> *check,
                    vector<optim_fun> *opts,
                    vector<algo> *algo,
                    vector<rewrite_fun> *rewrite,
                    vector<model_typ> *models,
                    modelMsg *modelMessage) :
    part_checks(partCheck), partitioners(partition),
    checks(check), optimizations(opts),
    algos(algo), rewrites(rewrite),
    models(models), modelMessage(modelMessage) {}
};

void compile(boost::program_options::variables_map vm,
			 std::string out_file_name, 
			 std::string routine_name,
			 std::map<std::string,type*>& inputs, 
			 std::map<std::string,type*>& inouts,
			 std::map<std::string,type*>& outputs,
			 std::vector<stmt*> const& prog);

#endif //COMPILE_HPP
