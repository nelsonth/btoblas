
#include <string>
#include "generate_code.hpp"
#include <fstream>
#include <iostream>
#include "partition.hpp"
#include "modelInfo.hpp"
#include "translate_to_code.hpp"
#include <string>
#include <map>

#include "cost_estimate_common.hpp"
#include "memmodel_par/build_machine.h"
extern "C" {
#include "memmodel_par/parallel_machines.h"
}

using namespace std;

#include "code_gen.hpp"

bool build_genMap_1thread_1tile(graph &g, vector<subgraph*> &subs, 
                             map<subgraph*, code_gen_t> &genMap) {
    
    // map subgraph * to the correct code generator
    // c loop
    // pthread dispath
    
    bool pthread = true;
    
    bool lPar = false;
    for (size_t i = 0; i < subs.size(); ++i) {
        
        bool par = build_genMap_1thread_1tile(g,subs[i]->subs,genMap);
        if (par)
            lPar = true;
        
        bool partitioned = false;
        vector<iterOp_t>::iterator pi;
        for (pi = subs[i]->sg_iterator.updates.begin();
             pi != subs[i]->sg_iterator.updates.end(); ++pi) {
            if (pi->right.find("$$") == 0) {
                partitioned = true;
                break;
            }
        }
        
        if (partitioned) {
            // partitioned subgraph
            if (pthread) {
                // pthread dispatch
                if (par)
                    genMap[subs[i]] = gen_pthread_loop;
                else
                    genMap[subs[i]] = gen_c_loop;
                
                lPar = true;
            }
            else {
                // cache tile
                genMap[subs[i]] = gen_c_loop;
            }
        }
        else {
            // serial subgraph
            genMap[subs[i]] = gen_c_loop;
        }
    }
    
    return lPar;
}


void build_genMap_all_tile(graph &g, vector<subgraph*> &subs, 
                             map<subgraph*, code_gen_t> &genMap) {
    
    // map subgraph * to the correct code generator
    // c loop
    // pthread dispath
    
    for (size_t i = 0; i < subs.size(); ++i) {
        
        build_genMap_all_tile(g,subs[i]->subs,genMap);

        genMap[subs[i]] = gen_c_loop;
    }
}


void build_genMap_all_thread(graph &g, vector<subgraph*> &subs, 
                  map<subgraph*, code_gen_t> &genMap) {
 
    // map subgraph * to the correct code generator
    // c loop
    // pthread dispath

    for (size_t i = 0; i < subs.size(); ++i) {
        
        build_genMap_all_thread(g,subs[i]->subs,genMap);
        
        bool partitioned = false;
        vector<iterOp_t>::iterator pi;
        for (pi = subs[i]->sg_iterator.updates.begin();
             pi != subs[i]->sg_iterator.updates.end(); ++pi) {
            if (pi->right.find("$$") == 0) {
                partitioned = true;
                break;
            }
        }
        
        if (partitioned) {
            // partitioned subgraph
            genMap[subs[i]] = gen_pthread_loop;
        }
        else {
            // serial subgraph
            genMap[subs[i]] = gen_c_loop;
        }
    }
}

void generate_code(string out_file_name, int vid, graph &g, string message,
		int baseDepth, int threadDepth, string routine_name, bool noptr,
		map<string,type*>& inputs, map<string,type*>& outputs, 
    modelMsg &msg, bool mpi, code_generation codegen_type) {
	// generate code
    
	std::ofstream fout((out_file_name + "__" + boost::lexical_cast<string>(vid) 
						+ ".c").c_str());
//#define DEBUG
#ifdef DEBUG
	// print graph 
	std::ofstream out2(string("lower" + boost::lexical_cast<string>(vid+1000) 
                              + ".dot").c_str());
	print_graph(out2, g); 
	out2.close();
#endif
    
#ifdef DUMP_GRAPHS
    // various debuging...
	graphToQuery(&g,out_file_name,vid,threadDepth);
#endif
    
    bool par = false;

#define NEW_CODE_GEN
#ifndef NEW_CODE_GEN
	// determine the current depth
	// if curDepth > baseDepth
	//		we must have added partitions for parallel
	//		go parallel
	// else
	//		go to C
	int curDepth = 0;
	check_depth(1,curDepth, g.subgraphs);
	
	if (curDepth > baseDepth) {
		par = true;
		/*potentialPar = true;
		threadDepth = curDepth - baseDepth;
		switch (threadDepth) {
			case 1: break;
			case 2:
				// task level parallelism
				if (g.subgraphs.size() != 1 
					|| g.subgraphs[0]->subs.size() != 1)
					par = false;
				// parallel reductions
				//else if (g.subgraphs[0]->summations.size() != 0
				//		 || g.subgraphs[0]->subs[0]->summations.size() != 0)
				//	par = false;
				// partition same dimension twice (this needs a better partition
				// size generator)
				else if (g.subgraphs[0]->subs[0]->iterations.find("$$") == 0)
					par = false;
				break;
			case 3:
				// task level parallelism
				if (g.subgraphs.size() != 1 
					|| g.subgraphs[0]->subs.size() != 1
					|| g.subgraphs[0]->subs[0]->subs.size() != 1)
					par = false;
				// parallel reductions
				//else if (g.subgraphs[0]->summations.size() != 0
				//		 || g.subgraphs[0]->subs[0]->summations.size() != 0
				//		 || g.subgraphs[0]->subs[0]->subs[0]->summations.size() != 0)
				//	par = false;
				// partition same dimension twice (this needs a better partition
				// size generator)
				else if (g.subgraphs[0]->subs[0]->iterations.find("$$") == 0
						 || g.subgraphs[0]->subs[0]->subs[0]->iterations.find("$$")
						 == 0)
					par = false;
				break;
			default:
				std::cout << "ERROR: compile.cpp; compile(); unexpected"
				<< " graph in parallel code gen\n";
		}*/
	}

	if (mpi) {
		translate_to_mpi(fout, routine_name, inputs, outputs, g,
							  threadDepth,msg.parallelArch->threads);
	} 
	else if (par) {
		translate_to_pthreads(fout, routine_name, inputs, outputs, g,
							  threadDepth,msg.parallelArch->threads);
	}
	else if (noptr) {
		translate_to_noPtr(fout, routine_name, inputs, outputs, g);
	} else {
		translate_to_intrin(fout, routine_name, inputs, outputs, g);
	}

#else
    // new code gen
    map<subgraph*, enum code_gen_t> genMap;
    genMap[NULL] = gen_top_level;
    
	if (codegen_type == bothtile) {
		build_genMap_1thread_1tile(g, g.subgraphs, genMap);
	} else if (codegen_type == threadtile) {
		build_genMap_all_thread(g, g.subgraphs, genMap);
	} else { // cachetile
		build_genMap_all_tile(g, g.subgraphs, genMap);
	}
	bto_program_t* program = build_AST(g, routine_name, inputs, 
                                       outputs, genMap);
    fout << program->to_string();

    delete program;
    
#endif
    
	fout.close();
	
    
#ifdef DUMP_DOT_FILES
	// print graph 
	std::ofstream out(string("lower" + boost::lexical_cast<string>(vid+100) 
							 + ".dot").c_str());
	print_graph(out, g); 
	out.close();
#endif
    
#ifdef DUMP_VERSION_STRINGS
	std::cout << vid << " : " << message;
	if (par)
		std::cout << "\t<- parallel\n";
	else
		std::cout << "\n";
#endif
	
    
}
