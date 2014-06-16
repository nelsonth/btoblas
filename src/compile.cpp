#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <math.h>
#include <signal.h>
#include "workSpace.hpp"
#include "build_graph.hpp"
#include "analyze_graph.hpp"
#include "optimizers.hpp"
#include "partition.hpp"
#include "enumerate.hpp"
#include "translate_to_code.hpp"
#include "compile.hpp"
#include "work.hpp"
#include "type_analysis.hpp"
#include "test_generator.hpp"
#include "boost/program_options.hpp"
#include "boost/timer.hpp"
#include <set>
#include <list>
#include "modelInfo.hpp"
#include "search_exhaustive.hpp"
#include "opt_decision.hpp"
#include "optimize_analysis.hpp"
#include "evolve.hpp"
#include "cost_estimate_common.hpp"
#include "memmodel_par/build_machine.h"
extern "C" {
#include "memmodel_par/parallel_machines.h"
}

//set precision type
std::string precision_type;

int MAX_PARTS;
int partition_bounds[32][3];

void sig_term_handler(int signum) {
    exit(0);
}

void sig_alarm_handler(int signum) {
    cout << "\n\nTime Limit Reached; Terminating All Processes\n" << endl;
    signal(SIGTERM, sig_term_handler);
    killpg(getpgrp(),SIGTERM);
}

int parse_range(int* lo, int* hi, int* stride, string s) {
	std::string::size_type start = 0;
	std::string::size_type end;
	end = s.find_first_of(":");
	if (end==string::npos) {
		return 1;
	}
	*lo = boost::lexical_cast<int>(s.substr(start, end-start));
	start = end+1;
	end = s.find_first_of(":", start);
	if (end==string::npos) {
		return 1;
	}
	*hi = boost::lexical_cast<int>(s.substr(start, end-start));
	*stride = boost::lexical_cast<int>(s.substr(end+1));
	return 0;
}

void compile(boost::program_options::variables_map vm,
			 string out_file_name,
			 string routine_name,
			 map<string,type*>& inputs,
			 map<string,type*>& inouts,
			 map<string,type*>& outputs,
			 vector<stmt*> const& prog)
{
    // set up signal to end search after certain amount of time
    int sMinutes = vm["limit"].as<int>();
    if (sMinutes > 0) {
        int sSeconds = sMinutes * 60;
        signal(SIGALRM, sig_alarm_handler);
        alarm(sSeconds);
    }
    
	// search strategy
	enum search_strategy {exhaustive, orthogonal, random, 
		hybrid_orthogonal, ga, debug, thread};

	search_strategy strat = ga;
	if (vm["search"].as<std::string>().compare("ex") == 0) {
		strat = exhaustive;
	} 
    else if (vm["search"].as<std::string>().compare("debug") == 0) {
		strat = debug;
	}
	else if (vm["search"].as<std::string>().compare("orthogonal") == 0) {
		strat = orthogonal;
	}
	else if (vm["search"].as<std::string>().compare("random") == 0) {
		strat = random;
	}
	else if (vm["search"].as<std::string>().compare("hybrid_orthogonal") == 0) {
		strat = hybrid_orthogonal;
	}
	else if (vm["search"].as<std::string>().compare("ga") == 0) {
		strat = ga;
	}
    else if (vm["search"].as<std::string>().compare("thread") == 0) {
        strat = thread;
    }
	else {
		std::cout << "Error: unknown search strategy (--search):" << vm["search"].as<std::string>() << "\n\n";
		exit(0);
	}
	
	// which backend
	bool noptr;
	if (vm["backend"].as<std::string>().compare("noptr") == 0) {
		noptr = true;
	} else {
		noptr = false;
	}
	std::cout << noptr << std::endl;
	
	// partitiong FIXME can't turn off anymore?
	/*
	bool partitionSet = true;
	if (vm.count("partition_off")) {
		partitionSet = false;
	}
	*/

	bool mpi = false;
	if (vm.count("distributed")) {
		mpi = true;
	}
	
	string pathToTop = getcwd(NULL,0);	
	pathToTop += "/";
	// std::cout << pathToTop << std::endl;
	
	// set up temporary workspace
	string tmpPath, fileName, path;
	if (setUpWorkSpace(out_file_name, fileName, path, tmpPath, pathToTop,
                       vm.count("delete_tmp"))) {
		std::cout << "Failed to set up temporary directory for unique implementations\n";
		return;
	}
	
	// set all work to be performed in temporary work directory
	out_file_name = tmpPath + fileName;
	
	
	// {{{ COST MODELS
	std::list<versionData*> orderedVersions;
	string testParam = vm["test_param"].as<std::string>();
	string start, stop, step;
	
	string::size_type pos = testParam.find_first_of(":");
	if (pos != string::npos)
		start = testParam.substr(0, pos);
	string::size_type last = pos+1;
	pos = testParam.find_first_of(":",last);
	if (pos != string::npos)
		stop = testParam.substr(last, pos-last);
	step = testParam.substr(pos+1,string::npos);
	
	if (boost::lexical_cast<int>(start) > boost::lexical_cast<int>(stop)) {
		std::cout << "Test parameters are illegal (start > stop)\n";
		std::cout << "\tstart: " << start << "\n";
		std::cout << "\tstop:  " << stop << "\n";
		return;
	}
	
	modelMsg msg(boost::lexical_cast<int>(start),
				 boost::lexical_cast<int>(stop),
				 boost::lexical_cast<int>(step));
	msg.pathToTop = pathToTop;
	// build_machine 0 -> parallel, 1 -> serial
	// msg.parallelArch = build_machine((char*)(pathToTop.c_str()),0);
	if (msg.parallelArch == NULL) {
		std:: cout << "Error attempting to get cost with parallel analytic model\n";
		//return;
	}
	// msg.serialArch = build_machine((char*)(pathToTop.c_str()),1);
	// if (msg.serialArch == NULL) {
		// std:: cout << "Error attempting to get cost with parallel analytic model\n";
		//return;
	// }
	
	// analyticSerial, tempCount, analyticParallel, symbolic, noModel			
	vector<model_typ> models;
	models.clear();
	// the model we want to rank with should go first in this list
	//models.push_back(noModel);
	models.push_back(analyticParallel);
	//models.push_back(analyticSerial);
	//models.push_back(tempCount);
	// }}}
	
	
	precision_type = vm["precision"].as<std::string>();
	
#ifdef MODEL_TIME
	// time spent in memmodel routines
	boost::timer memmodelTimer;
	double memmodelTotal = 0.0;
#endif
	
	
	/* Model deprecated
	double threshold;
	if (vm.count("use_model")) {
		std::cout << "\nAnalytic model enabled\n";
		threshold = vm["threshold"].as<double>();
	}
	*/
	
	std::vector<std::pair<int, double> >::iterator itr;
    
	graph g;
	
	std::vector<algo> algos;
	//std::cerr << "finished parsing" << std::endl;
	program2graph(prog, inputs, inouts, outputs, g);  
	//std::cerr << "graph created" << std::endl;
    //std::ofstream out44("lower0.dot");
    //print_graph(out44, g);
	//out44.close();
	//use yices to compute types externally
//#define TYPES
#ifdef TYPES
    std::ofstream out("lower0.dot");
    print_graph(out, g); 
	out.close();
    generate_constraints(g);
#endif
	compute_types(g);
#ifdef TYPES
    std::ofstream out("lower1.dot");
    print_graph(out, g);
	out.close();
    exit(0);
#endif
    
	update_sizes(g);
	update_dim_info(g);
    
	init_algos(algos);
		
	assign_algorithms(algos, g);
	//std::cerr << "algorithms assigned" << std::endl;
	
	assign_work(algos, g);
	//std::cerr << "work assigned" << std::endl;
	
	rewrite_fun rewriters[] =
    {	flip_transpose,                 //0
		flip_transpose_stride,          //1
		merge_tmp_output,               //2
		merge_tmp_cast_output,          //3
		remove_intermediate_temporary,  //4
		merge_gets,                     //5
		merge_sumto_store,              //6
		move_temporary,                 //7
		remove_cast_to_output,          //8
		remove_input_to_cast,           //9
		remove_cast_to_cast,            //10
		merge_same_cast,                //11
        straighten_top_level_scalar_ops,//12
        reduce_reductions               //13
    };
	
	optim_fun optimizers[] =
    {	fuse_loops,			//0
		fuse_loops_nested,	//1
		merge_scalars,		//2
		pipeline,			//3
        fuse_loops_n        //4
    };

	optim_fun_chk checkers[] =
    {	check_fuse_loops, 
		check_fuse_loops_nested,
		check_merge_scalars,
		check_pipeline,
        check_fuse_loops_n
    };
	
	partition_fun partition[] = {
		part_mult_left_result,	//0
		part_mult_right_result,	//1
		part_mult_left_right, 	//2
		part_mult_scl_row,		//3
		part_mult_scl_col,		//4
		partition_add_col,		//5
		partition_add_row,		//6
		part_mult_scl,			//7
		partition_add			//8
	};
	rewrite_fun part_checkers[] = {
		check_part_mult_left_result,
		check_part_mult_right_result,
		check_part_mult_left_right,
		check_part_mult_scl_row,
		check_part_mult_scl_col,
		check_partition_add_col,
		check_partition_add_row,
		check_part_mult_scl,
		check_partition_add
	};
	
	vector<rewrite_fun> rewrites(rewriters, rewriters + 
								 sizeof(rewriters) / sizeof(rewrite_fun));
	
	vector<optim_fun> optimizations(optimizers, optimizers + 
									sizeof(optimizers) / sizeof(optim_fun));
	vector<optim_fun_chk> checks(checkers, checkers + 
								 sizeof(checkers) / sizeof(optim_fun_chk));
	vector<rewrite_fun> part_checks(part_checkers, part_checkers + 
									sizeof(part_checkers)/sizeof(rewrite_fun));
	vector<partition_fun> partitioners(partition, partition +
									 sizeof(partition) / sizeof(partition_fun));
	
	//std::cerr << "about to start lowering and optimizine" << std::endl;
	
	std::stack<work_item> s;
	string history = "";
	
#ifdef DUMP_DOT_FILES
  	std::ofstream out("lower1.dot");
	print_graph(out, g);
	out.close();
#endif
	
#ifdef DUMP_GRAPHS
	graphToQuery(g,out_file_name,0,0);
#endif
	
	if (vm.count("correctness")) {
		createCorrectnessTest(g, routine_name, out_file_name, inputs, 
                              outputs,msg,noptr);
	}
	
	// keep copy of base graph around
	graph *baseGraph = new graph(g);

	// {{{ Partition Space
	code_generation codetype = threadtile;
	string spec = vm["level1"].as<std::string>();
	if (!spec.empty()) {
		string::size_type pos = spec.find_first_of(" ");
		if (pos == string::npos) {
			std::cout << "Bad format for level1 arg:" << spec << endl;
			exit(0);
		} else {
			string codetype_str = spec.substr(0, pos);
			if (codetype_str == "thread") {
				codetype = threadtile;
			} else if (codetype_str == "cache") {
				codetype = cachetile;
			} else {
				std::cout << "Bad format for level1 arg:" << spec << endl;
				std::cout << "needs to be thread or cache" << endl;
				exit(0);
			}
		}
		int err = parse_range(&partition_bounds[0][0], 
				&partition_bounds[0][1],
				&partition_bounds[0][2],
				spec.substr(pos+1));
		if (err) {
			std::cout << "Couldn't parse range for level1: " << spec << endl;
			exit(0);
		}
		if (partition_bounds[0][0] > partition_bounds[0][1]) {
			std::cout << "Test parameters are illegal (start > stop)\n";
			std::cout << "\tstart: " << partition_bounds[0][0] << "\n";
			std::cout << "\tstop:  " << partition_bounds[0][1] << "\n";
			exit(0);
		}

		// Now level 2
		string spec = vm["level2"].as<std::string>();
		std::cout << "spec:" << spec << endl;
		if (!spec.empty()) {
			MAX_PARTS = 2;
			string::size_type pos = spec.find_first_of(" ");
			cout << "pos = " << pos << endl;
			if (pos == string::npos) {
				std::cout << "Bad format for level2 arg:" << spec << endl;
				exit(0);
			} else {
				string codetype_str = spec.substr(0, pos);
				cout << "codetype_str: '" << codetype_str << "'" << endl;
				if (codetype_str == "thread" && codetype==cachetile) {
					std::cout << "ERROR: threads inside tile loops not supported" << endl;
					std::cout << "consider swapping level1 and level2 args" << endl;
					exit(0);
				} else if (codetype_str == "cache" && codetype==threadtile) {
					codetype = bothtile;
				} else {
					std::cout << "Bad format for level2 arg:" << spec << endl;
					std::cout << "needs to be thread or cache" << endl;
					exit(0);
				}
			}
			int err = parse_range(&partition_bounds[1][0], 
					&partition_bounds[1][1],
					&partition_bounds[1][2],
					spec.substr(pos+1));
			if (err) {
				std::cout << "Couldn't parse range for level2: " << spec.substr(pos) << endl;
				exit(0);
			}
			if (partition_bounds[1][0] > partition_bounds[1][1]) {
				std::cout << "Test parameters are illegal (start > stop)\n";
				std::cout << "\tstart: " << partition_bounds[1][0] << "\n";
				std::cout << "\tstop:  " << partition_bounds[1][1] << "\n";
				exit(0);
			}
		} else {
			// Only 1 level specified
			MAX_PARTS = 1;
		}
	} else {
		// no idea what this will do!
		// Maybe we should just throw an error here?  Or at least a warning?
		std::cout << "WARNING: No threading or cache tiling specified" << endl;
		std::cout << "Performing Loop fusion only, search may get confused" << endl;
		MAX_PARTS = 0;
	}

	// }}}

	// initializing the partition type forest
	// cout << "initializing partition forest..." << endl;
	vector< partitionTree_t* > partitionForest;
    buildForest(partitionForest, g, MAX_PARTS);
	// cout << "success" << endl;

	/*
	for (int i = 0; i < partitionForest.size(); ++i) {
		partitionTree_t *t = partitionForest[i];
		if (t != NULL) {
			stringstream o;
			o << "tree" << i << ".dot";
			ofstream treefile(o.str().c_str());
			printTree(treefile, t);
		}
	}
	*/

	initial_lower(g, algos, rewrites);
	//std::cout << "finished lowering and rewriting" << std::endl;
	
#ifdef DUMP_DOT_FILES
  	std::ofstream out2("lower2.dot");
	print_graph(out2, g);
	out2.close();
#endif
	
#if 0
    for (int i = 0; i < g.num_vertices(); ++i)
        if (g.info(i).op & OP_ANY_ARITHMATIC)
            cout << i << "\t";
    cout << "\n";
#endif
    
	int reps = vm["empirical_reps"].as<int>();
	if (vm.count("distributed")) {
		createMPIEmpiricalTest(g, routine_name, out_file_name, inputs,
                               outputs, noptr, reps);
	} else if (!vm.count("empirical_off")) {
		createEmpiricalTest(g, routine_name, out_file_name, inputs,
                            outputs, noptr, reps);
	}
	
    compile_details_t cDetails(routine_name,tmpPath,fileName,
                               vm.count("correctness"), 
                               vm.count("use_model"),
                               !vm.count("empirical_off"),
                               noptr, mpi, codetype,
                               &inputs, &inouts, &outputs);
    
    build_details_t bDetails(&part_checks, &partitioners,
                             &checks, &optimizations,
                             &algos, &rewrites,
                             &models, &msg);

	// int maxT = max(0,msg.parallelArch->threads);
	// std::cout << "Testing with " << maxT << " threads, step = " << min(4,maxT) << std::endl;
	// int stride = 2;
    
	// {{{ Strategy cases
	switch (strat) {
		case exhaustive:
		{
			std::cout << "entering exhaustive search" << endl;
			//int numVersions = 5;
			int bestVersion = smart_exhaustive(g, *baseGraph, 
                                               cDetails, bDetails);
			// One way or another we have selected a version we are calling the best
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
		case orthogonal:
		{
			std::cout << "entering orthogonal search" << endl;
			//int numVersions = 5;
			int bestVersion = orthogonal_search(g, *baseGraph, 
                                                cDetails, bDetails);
			// One way or another we have selected a version we are calling the best
            
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
		case random:
		{
			std::cout << "entering random search" << endl;
			int seconds = vm["random_count"].as<int>();
			int bestVersion = genetic_algorithm_search(g, *baseGraph,
				partitionForest, 
				seconds, 1, false, cDetails, bDetails).best_version; // pop = 1
			// One way or another we have selected a version we are calling the best
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
		case hybrid_orthogonal:
		{
			std::cout << "entering hybrid orthogonal search " << endl;
			int bestVersion = smart_hybrid_search(g, *baseGraph, 
                                                  cDetails, bDetails);
			// One way or another we have selected a version we are calling the best
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
		case debug:
		{
			std::cout << "entering debug search " << endl;
			int bestVersion = debug_search(g, *baseGraph, partitionForest,
											cDetails, bDetails);
			// One way or another we have selected a version we are calling the best
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
		case ga:
		{
			std::cout << "entering genetic algorithm search " << endl;
			bool gamaxfuse = true;
			if (vm.count("ga_nomaxfuse")) {
				gamaxfuse = false;
			}
			int seconds = vm["ga_timelimit"].as<int>();
			int popsize = vm["ga_popsize"].as<int>();
			int bestVersion = -1;
			if (vm.count("ga_noglobalthread")) {
				bestVersion = genetic_algorithm_search(g, *baseGraph,
					partitionForest, seconds, popsize, gamaxfuse, 
					cDetails, bDetails).best_version;
			} else if (vm.count("ga_exthread")) {
				bestVersion = ga_exthread(g, *baseGraph,
						 partitionForest, seconds, gamaxfuse, popsize, cDetails,
						 bDetails);
			} else { // global thread
				bestVersion = ga_thread(g, *baseGraph,
						 partitionForest, seconds, gamaxfuse, popsize, cDetails,
						 bDetails);
			}
			// One way or another we have selected 
			// a version we are calling the best
			// or we failed
			if (bestVersion < 0) {
				std::cout << "All versions failed to generate or compile\n";
				return;
			}
			else {
				std::cout << "\n----------------------------------\n";
				std::cout << "Best Version: " << fileName + "__" << bestVersion << ".c\n";
			}
			
			// copy best version to same directory as input .m file
			// and report location
			handleBestVersion(fileName,path,tmpPath,bestVersion);
			break;
		}
            
        case thread:
		{
            int thread_search(graph &g, graph &baseGraph, 
                              vector<partitionTree_t*> part_forest,
                              compile_details_t &cDet,
                              build_details_t &bDet);
			std::cout << "entering thread search " << endl;
			thread_search(g, *baseGraph, partitionForest,
                                           cDetails, bDetails);

			break;
		}
	}
	// }}}
    
    deleteForest(partitionForest);
}
