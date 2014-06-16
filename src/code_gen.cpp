//
//  code_gen.cpp
//  btoblas
//

#include <iostream>
#include "code_gen.hpp"
#include <deque>
#include "syntax.hpp"
#include "translate_utils.hpp"
#include "translate_to_code.hpp"
#include "partition.hpp"
#include <boost/algorithm/string.hpp>

using namespace std;
extern std::string precision_type;

bto_program_t *build_program(bto_function_t *func, graph &g) {
    // reorganize the ast

    // 1) move allocations/zeros as high as possible
    //      also match allocations with free's
    // 2) move message structures out to program level
    // 3) move pthread/mpi functions out to program level
    
    // reorganize access, zero, allocs, etc
    func->hoist_pointer_updates();
    func->hoist_zeros();
    func->hoist_allocations();
    func->insert_frees();
    
    // get main function and parallel function partition sizes 
    // and create tile sizes and number of thread information
    partition_t *pInfo = (partition_t*)g.get_partition_information();
    if (!pInfo) {
        map<string,int> empty;
        map<string,string> empty2;
        func->get_partitions_used(empty, empty2);
    }
    else {
        func->get_partitions_used(pInfo->sizeMap, pInfo->partitionDeps);
    }
    
    // reorganize for parallel code generation
    statement_list_t *pMesg = new statement_list_t();
    func->allocate_pthread_and_message();
    func->make_parallel_messages(*pMesg);
    statement_list_t *pFunc = new statement_list_t();
    func->hoist_parallel_functions(*pFunc);
    
    
    // put together final program.
    bto_program_t *p = new bto_program_t();
    p->func = func;
    p->parallel_mesgs = pMesg;
    p->parallel_funcs = pFunc;
    
    return p;
}

void build_allocations_r(graph &g, vertex v, vector<vertex> &outs) {
    
    if (g.info(v).op & (OP_ANY_ARITHMATIC | OP_GET))
        return;
    
    if (g.info(v).op & (temporary | sumto | OP_STORE)) {
        outs.push_back(v);
    }
    else if (g.info(v).op & output) {
        outs.push_back(v);
        return;
    }
    
    // if not adjacent edges do not continue.  if more than one
    // adjacent edge, then if the graph manipulation is working
    // these edges should be gets or arithmatic operations.
    if (g.adj(v).size() < 1)
        return;
    
    vertex l = g.adj(v)[0];
    
    build_allocations_r(g, l, outs);
}

void build_zero_data(graph &g, vertex v, map<vertex,zero_data_t*> &zeroMap) {
    
    string size = container_size(v, g);
    if (g.find_parent(v) == NULL) {
        // __m* does not exist yet, so force a __s*
        while (size.find("$$") != string::npos) {
            size.replace(size.find("$$"),2,"__s");
        }
    }
    // if nparts__s#*$$#, then $$ should be forced to
    // __s.  everything else will default to __m later on.
    size_t loc0 = size.find("nparts__");
    size_t loc1 = size.find("*");
    if (loc0 != string::npos && loc1 != string::npos && loc0 < loc1) {
        size_t loc2 = size.find("$$");
        if ((loc1+1) == loc2)
            size.replace(loc2,2,"__s");
    }
    
    
    string itr = string(1,var_name[depth(g.find_parent(v))]);
    bool sclr = g.info(v).t.k == scalar;
    
    bool element = false;
    if (sclr && g.adj(v).size() > 0)
        element = g.info(g.adj(v)[0]).t.k != scalar;
    
    string name = "t" + boost::lexical_cast<string>(v);
    if (g.info(v).op & (output | input))
        name = g.info(v).label;
    
    if (element) {
        // special case element accesses.
        vertex aj = g.adj(v)[0];
        if (!(g.info(aj).t.k == parallel_reduce)) {
            name = "t" + boost::lexical_cast<string>(aj);
            if (g.info(aj).op & (output | input))
                name = g.info(aj).label;
        } else {
            itr = "0";
        }
    }
    
    zeroMap[v] = new zero_data_t(name, size, itr, sclr, element);
}

void build_parallelReduce(graph &g, vertex v,
                           map<vertex,container_reduction_t*> &reduceMap) {
    // create parallel reduction
    
    // fromName is simply the vertex number of v
    string name = "t" + boost::lexical_cast<string>(v);

    // toName is more complicated, need to determine if this reduction
    // is to its self
    // *) if adj to v is output or temporary then reduce into that structure
    //      else reduce into self
    // then need to handle reducing to a scalar, this can be either to a
    //      a scalar value, or a scalar element.
    // 1) if not a self reduction and adj to adj is not a scalar then treat
    //      as element
    // 2) is reduction to self and adj is scalar get or store, then treat
    //      as element
    
    vertex aj = g.adj(v)[0];
    bool reduceToSelf = !(g.info(aj).op & (output | temporary | OP_STORE
                                           | partition_cast));
    bool element = false;
    bool isScalar = g.info(aj).t.k == scalar;
    string toName;
    if (reduceToSelf) {
        toName = name;
        
        if (isScalar && g.info(aj).op & (get_element | store_element))
            element = true;
    }
    else {
        toName = "t" + boost::lexical_cast<string>(aj);
        if (g.info(aj).op & (output | input))
            toName = g.info(aj).label;
        
        if (isScalar && g.adj(aj).size() > 0 
            && g.info(g.adj(aj)[0]).t.k != scalar)
            element = true;
    }

    //cout << v << " : " << toName << " = " << name << "\t" << 
    //cout << isScalar << "\t" << element << "\n";
    
    reduceMap[v] = new container_reduction_t(v, g, toName, name, isScalar,
                                             element);
}

void build_allocate(graph &g, vertex v,
                    map<vertex,allocate_t*> &allocMap) {
        
    // allocate
    string name = "t" + boost::lexical_cast<string>(v);
    string size = container_size(v, g);
    if (size.find("$$") == 0)
        size.replace(0,2,"__s");
    bool sclr = g.info(v).t.k == scalar;
    string itr = string(1,var_name[depth(g.find_parent(v))]);
    allocMap[v] =  new allocate_t(precision_type, name, 
                                  size, sclr);  
}

void shift_casts(graph &g, vector<vertex> &outs) {
    // given an output from op -> largest alloc or output,
    // swap any outputs | temp | sumto at the end with
    // casts.  casts on the read side are easier to deal with
    // that cast on the store side.
    
    vertex ia = outs.back();
    while (g.info(ia).op & (output | temporary | sumto) 
           && find(outs.begin(),outs.end(),ia) != outs.end()) {
        
        vertex nia = g.inv_adj(ia)[0];
        //cout << ia << "\t" << nia << "\n";
        
        if (g.info(nia).op == partition_cast) {
            op_code op = g.info(ia).op;
            g.info(ia).op = g.info(nia).op;
            g.info(nia).op = op;
            g.info(nia).label = g.info(ia).label;
            if (g.adj(ia).size() == 0) {
                g.clear_vertex(ia);
            }
            vector<vertex>::iterator i;
            i = find(outs.begin(),outs.end(),ia);
            *i = nia;
        }
        else
            break;
        ia = nia;
    }
}

void build_allocations(graph &g, map<vertex, allocate_t*> &allocMap,
                        map<vertex, zero_data_t*> &zeroMap,
                        map<vertex,bool> &reductionMap,
                        map<vertex,container_reduction_t*> &parallelReduce,
                        map<subgraph*, enum code_gen_t> &genMap) {
    // find all of the allocations and build that part of the ast.
    
    // allocMap is the output mapping temporary or sumto vertices to 
    // the particular ast built for them.
    // genMap maps subgraphs to the type of code generation.  this is
    // required so that allocations can happen locally in threads when
    // needed.
    
    // for now, just allocate.  when genMap is filled out, going to need
    // to determine when something is local to a thread.

    for (vertex v = 0; v < g.num_vertices(); ++v) {
        if (g.info(v).op & OP_ANY_ARITHMATIC) {
            
            int path = 0;
            if (g.adj(v).size() > 1 && g.adj(v)[0] != g.adj(v)[1]) {
                // try to find anything significant
                for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                    if (g.info(g.adj(v)[i]).op & (output | temporary |
                                                  sumto | OP_STORE)) {
                        path = i;
                        break;
                    }
                }
            }
            
            vector<vertex> outs;
            build_allocations_r(g,g.adj(v)[path],outs);
            
            if (outs.size() == 0)
                continue;
            
            outs.insert(outs.begin(),v);
            
            shift_casts(g, outs);
            
            vector<vertex>::iterator oi;
            bool change;
            vector<vertex> zero;
            
            do {
                change = false;
                oi = outs.begin()+1;
                for (; oi < outs.end(); ++oi) {
                    
                    bool atEnd = !((oi+1) < outs.end());
                    subgraph *sg = g.find_parent(*oi);
                    code_gen_t gt = genMap[sg];
                    vertex a;
                    if (g.adj(*oi).size() > 0)
                        a = g.adj(*oi)[0];
                    vertex ia;
                    if (g.inv_adj(*oi).size() > 0)
                        ia = g.inv_adj(*oi)[0];
                    subgraph *asg = g.find_parent(a);
                    code_gen_t agt = genMap[asg];
                    subgraph *iasg = g.find_parent(ia);
                    
                    if (g.info(*oi).op == temporary) {
                        // delete? delete when
                        // 1) temp->sumto in serial case, optimizes away to
                        //      just += into sumto
                        // 2) temp->output in top level case where total
                        //      type match
                        if (!atEnd && g.info(a).op == sumto
                            && sg->parent == asg
                            && agt == gen_c_loop
                            && total_type_match(g.info(*oi).t, 
                                                g.info(a).t)) {
                                // temp->sumto; delete
                                
                                g.remove_edges(ia,*oi);
                                g.move_out_edges_to_new_vertex(*oi, ia);
                                g.clear_vertex(*oi);
                                change = true;
                                continue;
                            }
                        else if (!atEnd && g.info(a).op == output
                                 && sg == asg
                                 && agt == gen_top_level
                                 && total_type_match(g.info(*oi).t, 
                                                     g.info(a).t)) {
                                     
                                     // temp->output; delete
                                     g.remove_edges(ia,*oi);
                                     g.move_out_edges_to_new_vertex(*oi, ia);
                                     g.clear_vertex(*oi);
                                     change = true;
                                     continue;
                                 }
                        
                        // make pointer or cast
                        // 1) sumto->tmp in same subgraph, this occurs with
                        //      back to back parallel reduction or top level 
                        //      reduction after a parallel loop
                        // 2) temp->cast*->output in top level
                        // 3) temp->output in top level
                        if (g.info(ia).op == sumto &&
                            sg == iasg) {
                            
                            // if pointing into a parent subgraph, then make
                            // pointer, otherwise this is some high level
                            // temporary that should turn into transparent
                            // cast.
                            bool pointer = false;
                            subgraph *sg0 = sg;
                            while (sg0 != asg && sg0) {
                                sg0 = sg0->parent;
                                if (sg0 == asg)
                                    pointer = true;
                            }
                            
                            if (pointer) {
                                if (g.info(*oi).t.k == row)
                                    g.info(*oi).op = store_row;
                                else if (g.info(*oi).t.k == column)
                                    g.info(*oi).op = store_column;
                                else
                                    g.info(*oi).op = store_element;
                                
                                change = true;
                            }
                            else if (g.info(*oi).t.k != scalar){
                                g.info(*oi).op = partition_cast;
                                change = true;
                            }
                        }
                        
                        vector<vertex>::iterator j = oi+1;
                        bool haveCastChainToOutput = 
                                (g.info(*oi).op & (temporary | partition_cast));
                        while (j < outs.end() && haveCastChainToOutput) {
                            if (g.info(*j).op == output) 
                                break;
                            if (g.find_parent(*j) != sg) {
                                haveCastChainToOutput = false;
                                break;
                            }
                            if (g.info(*j).op != partition_cast) {
                                haveCastChainToOutput = false;
                                break;
                            }
                            j++;
                        }
                        if (j < outs.end() &&
                            g.info(*j).op == output && 
                            haveCastChainToOutput) {
                            g.info(*oi).op = partition_cast;
                        }
                        
                    }
                    
                    if (g.info(*oi).op == sumto) { 
                        reductionMap[v] = true;
                        zero.push_back(*oi);
                        
                        // delete? delete when
                        // 1) sumto->output in the serail case, optimizes 
                        //      away to just += into the output
                        if (!atEnd && g.info(a).op == output
                            && sg == asg
                            && gt == gen_top_level
                            && total_type_match(g.info(*oi).t, 
                                                g.info(a).t)) {
                                
                                g.remove_edges(ia,*oi);
                                g.move_out_edges_to_new_vertex(*oi, ia);
                                g.clear_vertex(*oi);
                                change = true;
                                continue;
                            }

                        // make pointer
                        if (!atEnd && gt == gen_c_loop /*&&
                            agt & (gen_c_loop | gen_top_level)*/) {

                            if (g.info(*oi).t.k == scalar) {
                                g.info(*oi).op = store_element;
                                change = true;
                            } 
                            else if (1) {//(g.info(a).op & (output | temporary)) {
                                if (g.info(*oi).t.k == row)
                                    g.info(*oi).op = store_row;
                                else
                                    g.info(*oi).op = store_column;
                                
                                change = true;
                            }
                        } 
                        
                        if (!atEnd && gt == gen_pthread_loop) {
                            // find case of inner most pthread loop.  in this
                            // case the vertices actually belong to the
                            // thread body, so this sumto can become 
                            // just a pointer.
            
                            bool haveParallel = false;
                            subgraph *sg0 = iasg;
                            while (sg0 != sg) {
                                if (genMap[sg0] == gen_pthread_loop) {
                                    haveParallel = true;
                                    break;
                                }
                                sg0 = sg0->parent;
                            }
                            
                            if (!haveParallel) {
                                if (g.info(*oi).t.k == row)
                                    g.info(*oi).op = store_row;
                                else if (g.info(*oi).t.k == column)
                                    g.info(*oi).op = store_column;
                                else
                                    g.info(*oi).op = store_element; 
                            }
                        }
                    }
                }
                
                shift_casts(g, outs);
            } while (change);
            
            vector<vertex> pReduce;
            
            // now that sumto/temps are either deleted or made
            // into pointer/casts allocate what is left and build
            // parallel reductions where appropriate
            oi = outs.begin()+1;
            for (; oi < outs.end(); ++oi) {
                vertex ia = g.inv_adj(*oi)[0];
                
                if (g.info(*oi).op == temporary) {
                   build_allocate(g, *oi, allocMap); 
                }
                
                if (g.info(*oi).op == sumto) { 
                    reductionMap[v] = true;
                                        
                    build_allocate(g, *oi, allocMap);
                    
                    // parallel reduce?
                    if (genMap[g.find_parent(ia)] == gen_pthread_loop) {
                        pReduce.push_back(*oi);
                    }         
                }
            }

            // and finally zero the data, just zero the smallest portion
            vector<vertex>::iterator zr;
            for (zr = zero.begin(); zr < zero.end(); ++zr) {
                oi = find(outs.begin(),outs.end(),*zr);
                while (oi < outs.end() && g.info(*oi).op == deleted) {
                    ++oi;
                }
                if (oi < outs.end())
                    build_zero_data(g, *oi, zeroMap);
                break;
            }
            
            for (oi = pReduce.begin(); oi < pReduce.end(); ++oi) {
                
            }
            
            for (oi = pReduce.begin(); oi < pReduce.end(); ++oi) {
                // inserting this parallel reduction has created 
                // a temporary data structure which may be of
                // different size than the original data structure.
                // this means that all of the vertices from this 
                // vertex back up-stream to either the operation
                // or the another parallel reduction potentially
                // have incorrect dimension information.

                bool haveRow = false;
                bool haveCol = false;
                string br, bc;
                type *t = &g.info(*oi).t;
                while (t->k != scalar) {
                    // in a matrix a row describes the number of columns
                    // and a column describes tbe number or row.s
                    if (!haveCol && t->k == row) {
                        bc = t->dim.dim;
                        haveCol = true;
                    }
                    if (!haveRow && t->k == column) {
                        br = t->dim.dim;
                        haveRow = true;
                    }
                    if (haveRow && haveCol) 
                        break;
                    t = t->t;
                }
                string ld = "1";
                if (haveRow && haveCol) {
                    // matrix, get leading dimension
                    ld = g.info(*oi).t.get_lowest_ns()->dim.dim;
                }
                
                // update this type
                t = &g.info(*oi).t;
                while (t->k != scalar) {
                    if (t->k == row || t->k == column) {
                        t->dim.base_rows = br;
                        t->dim.base_cols = bc;
                        t->dim.lead_dim = ld;
                    }
                    t = t->t;
                }
                //cout << *oi << "\n" << prnt_detail(&g.info(*oi).t) << "\n";
                //cout << br << "\t" << bc << "\t" << ld << "\n";
                
                // push this information back up the outs chain
                vector<vertex>::iterator o;
                o = find(outs.begin(),outs.end(),*oi)-1;
                while (o >= outs.begin()) {
                    if (g.info(*o).t.k == parallel_reduce)
                        break;
                    
                    t = &g.info(*o).t;
                    while (t->k != scalar) {
                        if (t->k == row || t->k == column) {
                            t->dim.base_rows = br;
                            t->dim.base_cols = bc;
                            t->dim.lead_dim = ld;
                        }
                        t = t->t;
                    }
                    //cout << *o << "\n" << prnt_detail(&g.info(*o).t) << "\n";
                    o--;
                }
                
                // with the types updated, build the reduce
                build_parallelReduce(g, *oi, parallelReduce);
            }
        }
    }
    //std::ofstream out4("lower7.dot");
    //print_graph(out4, g);
    //out4.close();
}

void reduce_expressions(graph &g, map<vertex, assign_t*> &assignMap,
                        map<vertex,bool> &reductionMap,
                        map<subgraph*, code_gen_t> &genMap) {
    // find all of the operations
    // lOp    rOp
    //   \   /
    //     *
    //     |
    //    possibly an assign
    //
    // create some of the ast for that.
    
    // assign map will be filled with all of the operations.  each operation
    // that produces a assign will have an entry and will have the 
    // associated right hand side wrapped up with it.  operations that
    // are only operands to other operations will be part of some assign
    // tree in assignMap.
    
    // reductionMap says if an operation should use +=
    
    // keep track of all of the oepration that are not assigns so that 
    // a complete ast of all operations can be made.
    map<vertex, operation_t*> opMap;
    
    for (vertex u = 0; u < g.num_vertices(); ++u) {
        if (g.info(u).op & OP_2OP_ARITHMATIC) {
            
            struct operation_t *oper = new operation_t(g.info(u).op,false);

            // left operand
            vertex lop = g.inv_adj(u)[0];
            op_code op = g.info(lop).op;
            // if the left operand is an access of some sort, create that
            // access, otherwise it must be another operation and that will
            // get filled in later.
            if (op & (sumto | input | output | temporary | get_element)) {
                // if get_element, name[index], else just name
                bool sclr = op != get_element;
                
                string index = string(1,var_name[depth(g.find_parent(lop))]);
                
                if (op & get_element) {
                    //g.info(lop).op = deleted;
                    lop = g.inv_adj(lop)[0];
                    op = g.info(lop).op;
                }
                
                string name;
                if (op & (input | output))
                    name = g.info(lop).label;
                else
                    name = "t" + boost::lexical_cast<string>(lop);
                
                access_t *lAccess = new access_t(name, index, sclr);
                
                operand_t *loperand = new operand_t(lAccess);
                
                oper->left = loperand;
            }
            
            // right operand
            vertex rop = g.inv_adj(u)[1];
            op = g.info(rop).op;
            // if the right operand is an access of some sort, create that
            // access, otherwise it must be another operation and that will
            // get filled in later.
            if (op & (sumto | input | output | temporary | get_element)) {
                // if get_element, name[index], else just name
                bool sclr = op != get_element;
                
                string index = string(1,var_name[depth(g.find_parent(rop))]);
                
                if (op & get_element) {
                    //g.info(rop).op = deleted;
                    rop = g.inv_adj(rop)[0];
                    op = g.info(rop).op;
                }
                
                string name;
                if (op & (input | output))
                    name = g.info(rop).label;
                else
                    name = "t" + boost::lexical_cast<string>(rop);
                
                access_t *rAccess = new access_t(name, index, sclr);
                
                operand_t *roperand = new operand_t(rAccess);
                
                oper->right = roperand;
            }
            else if ((g.inv_adj(u)[0] == g.inv_adj(u)[1])
                     && (oper->left != NULL)) {
                // special case when left and right operand are the same
                // and they are both an access.
                access_t *rAccess = new access_t(oper->left->op.access->name, 
                                            oper->left->op.access->index, 
                                            oper->left->op.access->sclr);
                
                operand_t *roperand = new operand_t(rAccess);
                
                oper->right = roperand;
            }
            
            // result
            vertex resultop = g.adj(u)[0];
            op = g.info(resultop).op;
            // if the result is a assign of some sort, create that
            // assign, otherwise it must be another operation and that will
            // get filled in later.
            if (op & (sumto | store_element | store_add_element | output
                      | temporary)) {
                
                bool issum = ((op & (store_add_element | sumto)) 
                              || reductionMap[u]);
                
                string index = string(1,var_name[depth(
                                    g.find_parent(resultop))]);
                
                // if we cross from serial to parallel, 
                // then we actually want name[0] for a scalar value
                subgraph *sg = g.find_parent(u);
                bool haveParallel = false;
                while (sg && sg != g.find_parent(resultop)) {
                    if (genMap[sg] & GEN_PARALLEL) {
                        haveParallel = true;
                        break;
                    }
                    sg = sg->parent;
                }
                haveParallel |= (genMap[g.find_parent(resultop)] & GEN_PARALLEL);
                
                if (!haveParallel) { 
                    if (op & (store_element | store_add_element)) {
                        // if this is just a store, then the actual 
                        // data im storing to is what I want.
                        g.info(resultop).op = deleted;
                        //cout << "deleting " << resultop << endl;
                        resultop = g.adj(resultop)[0];
                        op = g.info(resultop).op;
                    }
                    else if (op & (sumto)) {
                        // if this is reduction to a scalar, but that
                        // scalar is an element of a container, then
                        // the store should take place at cont[index]
                        // and not to a scalar.
                        if (g.info(g.adj(resultop)[0]).t.k != scalar) {
                            resultop = g.adj(resultop)[0];
                            op = g.info(resultop).op;
                        }
                    }
                }
                
                string name;
                if (op & output)
                    name = g.info(resultop).label;
                else
                    name = "t" + boost::lexical_cast<string>(resultop);

                // if store_element, name[index], else just name
                bool sclr = g.info(resultop).t.k == scalar;
                
                if (sclr && haveParallel) {
                    index = "0";
                    sclr = false;
                }
                
                access_t *resultAccess = new access_t(name, index, sclr);
                
                rhs_union rhs;
                rhs.operation = oper;
                assign_t *assgn = new assign_t(resultAccess, rhs, 
                                               _rhs_operation, issum);
                
                assignMap[u] = assgn;
            }
            else {
                // delete all of the operations except for the ones
                // that are associated with a store.
                g.info(u).op = deleted;
                opMap[u] = oper;
            }
        }
        
        else if (g.info(u).op & OP_1OP_ARITHMATIC) {
            struct operation_t *oper = new operation_t(g.info(u).op,true);
            
            // operand
            vertex lop = g.inv_adj(u)[0];
            op_code op = g.info(lop).op;
            // if the left operand is an access of some sort, create that
            // access, otherwise it must be another operation and that will
            // get filled in later.
            if (op & (sumto | input | output | temporary | get_element)) {
                // if get_element, name[index], else just name
                bool sclr = op != get_element;
                
                string index = string(1,var_name[depth(g.find_parent(lop))]);
                
                if (op & get_element) {
                    lop = g.inv_adj(lop)[0];
                    op = g.info(lop).op;
                }
                
                string name;
                if (op & (input | output))
                    name = g.info(lop).label;
                else
                    name = "t" + boost::lexical_cast<string>(lop);
                
                access_t *lAccess = new access_t(name, index, sclr);
                
                operand_t *loperand = new operand_t(lAccess);
                
                oper->left = loperand;
            }
            
            // result
            vertex resultop = g.adj(u)[0];
            op = g.info(resultop).op;
            // if the result is a assign of some sort, create that
            // assign, otherwise it must be another operation and that will
            // get filled in later.
            if (op & (sumto | store_element | store_add_element | output
                      | temporary)) {
                
                bool issum = ((op & (store_add_element | sumto)) 
                              || reductionMap[u]);
                
                string index = string(1,var_name[
                        depth(g.find_parent(resultop))]);
                
                if (op & (store_element | store_add_element)) {
                    // if this is just a store, then the actual 
                    // data im storing to is what I want.
                    g.info(resultop).op = deleted;
                    //cout << "deleting " << resultop << endl;
                    resultop = g.adj(resultop)[0];
                    op = g.info(resultop).op;
                }
                
                string name;
                if (op & output)
                    name = g.info(resultop).label;
                else
                    name = "t" + boost::lexical_cast<string>(resultop);
                
                // if store_element, name[index], else just name
                bool sclr = g.info(resultop).t.k == scalar;
                access_t *resultAccess = new access_t(name, index, sclr);
                
                rhs_union rhs;
                rhs.operation = oper;
                assign_t *assgn = new assign_t(resultAccess, rhs,
                                               _rhs_operation, issum);
                
                assignMap[u] = assgn;
            }
            else {
                // delete all of the operations except for the ones
                // that are associated with a store.
                g.info(u).op = deleted;
                opMap[u] = oper;
            }
        }
    }
    
    // assign and non-assign operations are all now collected.  
    // build out each assign to completion
    map<vertex,operation_t*>::iterator oitr;
    for (oitr=opMap.begin(); oitr!=opMap.end(); oitr++) {
        operation_t *op = oitr->second;
        
        if (op->left == NULL) {
            vertex lop = g.inv_adj(oitr->first)[0];
            if (opMap.find(lop) == opMap.end())
                std::cout << "code_gen.cpp; reduce_expressions(); "
                            << "unexpected graph; left op\n";
            op->left = new operand_t(opMap[lop],true);
        }
        if (!(op->unary) && op->right == NULL) {
            vertex rop = g.inv_adj(oitr->first)[1];
            if (opMap.find(rop) == opMap.end())
                std::cout << "code_gen.cpp; reduce_expressions(); "
                << "unexpected graph; right op\n";
            op->right = new operand_t(opMap[rop],true);
        }
    }
    
    // right hand side at this point are only operations.
    map<vertex,assign_t*>::iterator itr;
    for (itr=assignMap.begin(); itr!=assignMap.end(); itr++) {
        operation_t *op = itr->second->rightHandSide.operation;
        
        if (op->left == NULL) {
            vertex lop = g.inv_adj(itr->first)[0];
            if (opMap.find(lop) == opMap.end()) {
                std::cout << "code_gen.cpp; reduce_expressions(); "
                << "unexpected graph\n";
                cout << itr->first << "; left\n";
            }
            op->left = new operand_t(opMap[lop],true);
        }
        if (!(op->unary) && op->right == NULL) {
            vertex rop = g.inv_adj(itr->first)[1];
            if (opMap.find(rop) == opMap.end()) {
                std::cout << "code_gen.cpp; reduce_expressions(); "
                << "unexpected graph\n";
                cout << itr->first << "; right\n";
            }
            op->right = new operand_t(opMap[rop],true);
        }
    }
    
    // clean up
    for (oitr=opMap.begin(); oitr!=opMap.end(); oitr++) {
        delete oitr->second;
    }
    
    /*
    for (itr=assignMap.begin(); itr!=assignMap.end(); itr++) {
        std::cout << itr->second->to_string() << "\n";
    }
    */
}

void fixup_out_edges(graph &g) {
    // look for multiple out edges from a store and reduce them.
    bool change = true;
    while (change) {
        change = false;
        for (vertex v = 0; v < g.num_vertices(); ++v) {
            
            if (g.adj(v).size() <= 1)
                continue;
            
            if ((g.info(v).op & OP_STORE)) {
                
                bool haveOut = false;
                vertex out;
                for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                    out = g.adj(v)[i];
                    while (g.info(out).op == partition_cast)
                        out = g.adj(out)[0];
                    
                    if (g.info(out).op & (output | temporary | sumto)) {
                        haveOut = true;
                        out = g.adj(v)[i];
                        break;
                    }
                }
                
                int stores = 0;
                vertex str;
                if (!haveOut) {
                    // dont have an output, so there should be just 1
                    // store, find that.
                    for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                        vertex a = g.adj(v)[i];
                        if (g.info(a).op & OP_STORE) {
                            stores++;
                            str = a;
                        }
                    }
                    
                    if (stores != 1) {
                        std::cout << "code_gen.cpp; fixup_out_edges(); "
                            << "unexpected graph\n" << stores << "\t" 
                            << v << "\n";
                        std::ofstream out2(string("lower1000.dot").c_str());
                        print_graph(out2, g); 
                        out2.close();
                        exit(0);
                    }
                    out = str;
                }
                
                op_code newOp;
                switch (g.info(v).op) {
                    case store_element:
                        newOp = get_element;
                        break;
                    case store_row:
                        newOp = get_row;
                        break;
                    case store_column:
                        newOp = get_column;
                        break;
                    default:
                        std::cout << "code_gen.cpp; fixup_out_edges(); "
                        << "unexpected op_code\n" << v << "\n";
                        exit(0); 
                }
                
                change = true;
                
                vertex_info newop(g.info(v).t, newOp, g.info(v).label);
                vertex newV = g.add_vertex(newop);
                if (g.find_parent(v))
                    g.find_parent(v)->vertices.push_back(newV);
                g.remove_edges(v,out);
                g.move_out_edges_to_new_vertex(v,newV);
                g.add_edge(v,out);
                
                vertex last = newV;
                while (g.info(out).op == partition_cast) {
                    vertex_info newc(g.info(out).t, partition_cast, 
                                     g.info(out).label);
                    newV = g.add_vertex(newc);
                    if (g.find_parent(out))
                        g.find_parent(out)->vertices.push_back(newV);
                    g.add_edge(newV, last);
                    last = newV;
                    out = g.adj(out)[0];
                }
                g.add_edge(out, newV);
            }
            
            if ((g.info(v).op & sumto)) {
                // certain sumto's behave as a store if there is some
                // adjacent edge that is larger than this sumto.
                bool as_store = false;
                vertex bigger;
                int height = g.info(v).t.height;
                for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                    vertex a = g.adj(v)[i];
                    if (g.info(a).op != partition_cast && 
                        g.info(a).t.height > height) {
                        bigger = a;
                        as_store = true;
                        break;
                    }
                }
                if (as_store) {
                    op_code newOp;
                    if (g.info(v).t.k == scalar) 
                        newOp = get_element;
                    else if (g.info(v).t.k == scalar) 
                        newOp = get_row;
                    else
                        newOp = get_column;
                    
                    change = true;
                    
                    vertex_info newop(g.info(v).t, newOp, g.info(v).label);
                    vertex newV = g.add_vertex(newop);
                    if (g.find_parent(v))
                        g.find_parent(v)->vertices.push_back(newV);
                    g.remove_edges(v,bigger);
                    g.move_out_edges_to_new_vertex(v,newV);
                    g.add_edge(v,bigger);
                    g.add_edge(bigger,newV);
                }
            }
            
            if ((g.info(v).op & sumto)) {
                // certain sumto's are followed by outputs.
                
                bool haveOut = false;
                vertex out;
                for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                    out = g.adj(v)[i];
                    if (g.info(out).op == output) {
                        haveOut = true;
                        break;
                    }
                }
                
                if (haveOut) {
                    change = true;
                    g.remove_edges(v,out);
                    g.move_out_edges_to_new_vertex(v,out);
                    g.add_edge(v,out);
                }
            }
            
            if ((g.info(v).op & partition_cast)) {
                // if cast has any out edges and one of those
                // is an output update the edges
                // 1) if the same type then remove the cast
                // 2) if not just fix edges
                bool haveOut = false;
                vertex out;
                for (unsigned int i = 0; i < g.adj(v).size(); ++i) {
                    out = g.adj(v)[i];
                    if (g.info(out).op == output) {
                        haveOut = true;
                        break;
                    }
                }
                
                if (haveOut) {
                    change = true;
                    
                    g.remove_edges(v,out);
                    g.move_out_edges_to_new_vertex(v,out);
                    if (total_type_match(g.info(v).t,g.info(out).t)) {
                        g.add_edge(g.inv_adj(v)[0],out);
                        g.clear_vertex(v);
                    }
                    else {
                        g.add_edge(v,out);
                    }
                }
            }
            
        }
    }
    //std::ofstream out4("lower7.dot");
    //print_graph(out4, g);
    //out4.close();
}

void add_reduction_iters_r(subgraph* sg, map<subgraph*, code_gen_t> &genMap) {
    // if any subgraph is parallel and it contains a sumto, them
    // add to the subgraph a reduction iterator.
    // if i = 0; i < size; i += $$3 is the current iterator
    // then add
    // ii = 0; ; ii += 1
    if (genMap[sg] == gen_pthread_loop) {
        iterOp_t &oInit = *sg->sg_iterator.initializations.begin();
        string nItr = oInit.left+oInit.left;
        iterOp_t nInit(nItr,"=","0");
        sg->sg_iterator.initializations.push_back(nInit);
        
        iterOp_t nUpdate(nItr,"+=","1");
        sg->sg_iterator.updates.push_back(nUpdate);
    }
    for (unsigned int i = 0; i < sg->subs.size(); ++i)
        add_reduction_iters_r(sg->subs[i], genMap);
}
    
void add_reduction_iters(graph &g, map<subgraph*, code_gen_t> &genMap) {
    // if any subgraph is parallel and it contains a sumto, them
    // add to the subgraph a reduction iterator.
    // if i = 0; i < size; i += $$3 is the current iterator
    // then add
    // ii = 0; ; ii += 1
    
    for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
        add_reduction_iters_r(g.subgraphs[i], genMap);
    }
}

void optimize_reductions(graph &g, map<subgraph*, code_gen_t> &genMap) {
    // temporary stuctures are placed in reductions as
    //  temp -> sumto
    // the parent of the parent of the temporary is equal to the parent of
    // the sumto.
    // the temporary can be optimized to nothing when 
    // the sumto is not performing a parallel reduction.
    
    for (vertex v = 0; v < g.num_vertices(); ++v) {
        if (!(g.info(v).op & (temporary)))
            continue;
        
        if (g.adj(v).size() > 1)
            continue;
        
        vertex adj = g.adj(v)[0];
        if (g.info(adj).op != sumto)
            continue;
        
        if (!total_type_match(g.info(v).t, g.info(adj).t))
            continue;
        
        subgraph *adjP = g.find_parent(adj);
        if ((g.find_parent(v) && g.find_parent(v)->parent != adjP)
            || (g.find_parent(v) == NULL))
            continue;

        if ((genMap[g.find_parent(v)] & GEN_PARALLEL)) {
            // in the parallel case make the temporary into
            // a store
            if (g.info(v).t.k == row)
                g.info(v).op = store_row;
            else if (g.info(v).t.k == column)
                g.info(v).op = store_column;
            else
                g.info(v).op = store_element;
        }
        else {
            // in the serial case optimize away the temporary
            vertex ia = g.inv_adj(v)[0];
            g.remove_edges(ia,v);
            g.move_out_edges_to_new_vertex(v, ia);
            g.clear_vertex(v);
        }
    }
    
    // if the sumto is performing a parallel reduction, then add another
    // level of container of size of number of iterations of the parent 
    // of the temporary.  (the subgraph of the temporary represents
    // a thread and so there will be thread number of sumto size container
    // created.  these are reduced eventually to size of sumto.
    for (vertex v = 0; v < g.num_vertices(); ++v) {
        if (g.info(v).op != sumto)
            continue;
                
        vertex ia = g.inv_adj(v)[0];
        bool parallel = false;
        while (!(g.info(ia).op & (OP_ANY_ARITHMATIC))) {
            if (genMap[g.find_parent(ia)] & GEN_PARALLEL) {
                parallel = true;
                break;
            }
            if (g.inv_adj(ia).size() < 1)
                break;
            ia = g.inv_adj(ia)[0];
        }

        if (!parallel) 
            continue;
        
        // have a parallel reduction, add a
        // temporary after sumto so input can be a vector of size(sumto)
        // which will reduce into a single container of size(sumto) 
        // which is the new temporary
        vertex_info newT = vertex_info(g.info(v));
        newT.op = temporary;
        vertex newT_v = g.add_vertex(newT);
        
        if (g.find_parent(v))
            g.find_parent(v)->vertices.push_back(newT_v);
        
        g.move_out_edges_to_new_vertex(v, newT_v);
        g.add_edge(v, newT_v);
        
        // make sumto larger by size of threads creating sumto
        subgraph *sg = g.find_parent(g.inv_adj(v)[0]);
        string threadCnt = sg->sg_iterator.updates.begin()->right;
        threadCnt.replace(0,2,"nparts__s");
        type *t = new type(g.info(v).t);
        delete g.info(v).t.t;
        g.info(v).t.t = t;
        g.info(v).t.height++;
        g.info(v).t.s = general;
        g.info(v).t.dim.step = "1";
        g.info(v).t.dim.dim = threadCnt;
        g.info(v).t.dim.lead_dim = container_size_type(*g.info(v).t.t);
        g.info(v).t.k = parallel_reduce;
    }
    
    //std::ofstream out4("lower7.dot");
    //print_graph(out4, g);
    //out4.close();
}

statement_list_t vertex_to_AST(graph &g, vertex u,
                           map<vertex,assign_t*> &assignMap,
                           map<vertex, allocate_t*> &allocMap,
                           map<vertex, zero_data_t*> &zeroMap,
                    map<vertex, container_reduction_t*> &parallelReduce,
                               map<subgraph*,code_gen_t> &genMap) {
    
    statement_list_t stmts;

    // for assignMap details see reduce_expressions()
    if (allocMap.find(u) != allocMap.end()) {
        statement_t *newStmt = new statement_t();
        newStmt->code = _allocate;
        newStmt->stmt.allocate = allocMap[u];
        stmts.push_back(newStmt);
    }
    
    if (zeroMap.find(u) != zeroMap.end()) {
        statement_t *newStmt = new statement_t();
        newStmt->code = _zero_data;
        newStmt->stmt.zero_data = zeroMap[u];
        stmts.push_back(newStmt);
    }
    
    if (parallelReduce.find(u) != parallelReduce.end()) {
        statement_t *newStmt = new statement_t();
        newStmt->code = _container_reduction;
        newStmt->stmt.container_reduction = parallelReduce[u];
        stmts.push_back(newStmt);
    }
    
    switch (g.info(u).op) {
        case get_row:
        case get_column: {
            statement_t *newStmt = new statement_t();
            newStmt->code = _pointer_update;
            
            string leftName = "t" + boost::lexical_cast<string>(u);
            
            vertex pred = g.inv_adj(u)[0];
            string rightName = "t" + boost::lexical_cast<string>(pred);
            if (g.info(pred).op & (input | output))
                rightName = g.info(pred).label;
            
            string index = string(1,var_name[depth(g.find_parent(u))]);
            string update = index + get_next_elem(g.info(pred).t);
            newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                 update);
            stmts.push_back(newStmt);
            break;
        }
        case get_row_from_column:
        case get_column_from_row: {
            statement_t *newStmt = new statement_t();
            newStmt->code = _pointer_update;
            
            string leftName = "t" + boost::lexical_cast<string>(u);
            
            vertex pred = g.inv_adj(u)[0];
            string rightName = "t" + boost::lexical_cast<string>(pred);
            if (g.info(pred).op & (input | output))
                rightName = g.info(pred).label;
            
            string index = string(1,var_name[depth(g.find_parent(u))]);
            string update = index + get_next_elem_stride(g.info(pred).t);
            newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                     update);
            stmts.push_back(newStmt);
            break;
        }
        case store_row:
        case store_column:
        case store_element: {
            if (genMap[g.find_parent(u)] & GEN_SERIAL) {
                statement_t *newStmt = new statement_t();
                newStmt->code = _pointer_update;
                
                string leftName = "t" + boost::lexical_cast<string>(u);
                
                vertex succ = g.adj(u)[0];
                string rightName = "t" + boost::lexical_cast<string>(succ);
                if (g.info(succ).op & (input | output))
                    rightName = g.info(succ).label;
                
                string index = string(1,var_name[depth(g.find_parent(u))]);
                string update = index + get_next_elem(g.info(succ).t);
                newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                     update);
                stmts.push_back(newStmt);
            }
            else {
                statement_t *newStmt = new statement_t();
                newStmt->code = _pointer_update;
                
                string leftName = "t" + boost::lexical_cast<string>(u);
                
                vertex succ = g.adj(u)[0];
                string rightName = "t" + boost::lexical_cast<string>(succ);
                if (g.info(succ).op & (input | output))
                    rightName = g.info(succ).label;
                
                string index, update;
                if (g.info(succ).op == sumto || g.info(succ).t.k == 
                    parallel_reduce) {
                    index = string(1,var_name[depth(g.find_parent(u))]);
                    index += index;
                    update = index + "*" + 
                            container_size_type(*g.info(succ).t.t);
                } else {
                    index = string(1,var_name[depth(g.find_parent(u))]);
                    update = index + get_next_elem(g.info(succ).t);
                }
                
                newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                     update);
                stmts.push_back(newStmt);
            }
            break;
        }
        case partition_cast: {
            // determine if this is an upcast or a down cast
            // if inv_adj is a store, then adj is the actual data structure
            // if adj is a get, then inv_adj is the actual data structure
            vertex ia = g.inv_adj(u)[0];
            while (g.info(ia).op == partition_cast)
                ia = g.inv_adj(ia)[0];
            vertex a = g.adj(u)[0];
            while (g.info(a).op == partition_cast)
                a = g.adj(a)[0];
            
            bool useIa = false;
            bool useA = false;
            
            if (g.info(ia).op & OP_STORE)
                useA = true;
            else if (g.info(a).op & OP_GET)
                useIa = true;
            else if (g.info(a).op & (output))
                useA = true;
            else if (g.info(ia).op & (input))
                useIa = true;
            
            if (useA) {
                statement_t *newStmt = new statement_t();
                newStmt->code = _pointer_update;
                
                string leftName = "t" + boost::lexical_cast<string>(u);
                
                string rightName = "t" + boost::lexical_cast<string>(a);
                if (g.info(a).op & (input | output))
                    rightName = g.info(a).label;
                
                string update = "";
                newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                     update, true);
                stmts.push_back(newStmt);
            }
            else if (useIa) {
                statement_t *newStmt = new statement_t();
                newStmt->code = _pointer_update;
                
                string leftName = "t" + boost::lexical_cast<string>(u);
                
                string rightName = "t" + boost::lexical_cast<string>(ia);
                if (g.info(ia).op & (input | output))
                    rightName = g.info(ia).label;
                
                string update = "";
                newStmt->stmt.pointer_update = 
                new pointer_update_t(precision_type, leftName, rightName,
                                     update, true);
                stmts.push_back(newStmt);
            }
            else {
                cout << "WARNING: code_gen.cpp; vertex_to_AST(); unexpected cast node"
                    << " : " << u << "\n";
            }
            break;
        }
        case get_element:
        case temporary: 
        case sumto: 
        case output:
        case input:
        case deleted:
            // nop
            break;
         
        case add:
        case subtract:
        case multiply:
        case divide: 
        case squareroot: {
            statement_t *newStmt = new statement_t();
            newStmt->code = _assign;
            newStmt->stmt.assign = assignMap[u];
            stmts.push_back(newStmt);
            break;
        }
        case store_add_row:
        case store_add_column:
        case store_add_element:
        
        default:
            std::cout << "WARNING: code_gen.cpp; vertex_to_AST();" 
                        << " unhandled or unexpected vertex\n";
            std::cout << ">> " << u << " <<\n";            
    }
    return stmts;
}

int pthreadUID;
statement_list_t* build_AST_r(graph &g, string name,
                              vector<subgraph*> subgraphs, 
                        subgraph *current,
                        vector<vertex> &vertices,
                        map<subgraph*, enum code_gen_t> &genMap,
                        map<vertex, assign_t*> &assignMap,
                        map<vertex, allocate_t*> &allocMap,
                        map<vertex, zero_data_t*> &zeroMap,
                    map<vertex, container_reduction_t*> &parallelReduce) {
    
    // for assignMap details see reduce_expressions()
    
    // topologically sort the current subgraphs
    deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
  	order_subgraphs(order, new_sub, new_old, current, vertices, 
                    subgraphs, g);
    
  	// a new graph was made where subgraphs are collapsed to vertices
    // and given a number and original vertices at the current level
    // are renumbered.  Now going through order will give the overall
    // operation order.  
    
    statement_list_t *statements = new statement_list_t();

  	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,vertex>::iterator iter0 = new_old.find(order[i]);
    	if (iter0 != new_old.end()) {
            // found a vertex
      		vertex u = iter0->second;
            statement_list_t stmts = vertex_to_AST(g,u,assignMap,allocMap,
                                              zeroMap, parallelReduce,
                                                   genMap);
            for (unsigned int i = 0; i < stmts.size(); ++i)
                statements->push_back(stmts[i]);
    	}
        
        map<vertex,subgraph*>::iterator iter1 = new_sub.find(order[i]);
    	if (iter1 != new_sub.end()) {
            // found a subgraph
            subgraph* sg = iter1->second;
            //std::cout << sg->uid.first << "," << sg->uid.second << "\n";
            
            statement_list_t *stmts = build_AST_r(g, name, sg->subs, sg, 
                                        sg->vertices, genMap, assignMap,
                                        allocMap, zeroMap,
                                        parallelReduce);
            
            // build correct AST node based on genMap
            if (genMap[sg] == gen_c_loop) {
                statement_t *newStmt = new statement_t();
                c_loop_t *loop = new c_loop_t(stmts,&sg->sg_iterator, sg);
                newStmt->code = _c_loop;
                newStmt->stmt.c_loop = loop;

                statements->push_back(newStmt);
            }
            else if (genMap[sg] == gen_pthread_loop) {
                // a subgraph intended to dispatch a thread body must
                // create the dispatch loop out of the iterator and all
                // of the pointer updates, and then everything else 
                // (i.e. child subgraphs) belong to the function body.
                // so create the dispatch loop, and give it two statements
                // 1) function call
                // 2) function (moved later on)
                // and then put the dispatch loop into the current level
                // statements.
                statement_t *newStmt0 = new statement_t();
                statement_t *newStmt1 = new statement_t();
                statement_t *newStmt2 = new statement_t();
                
                // create the function, which holds all of the statements
                // just returned
                pthread_function_t *pthf = new pthread_function_t(stmts,
                            name, pthreadUID);
                newStmt0->code = _pthread_function;
                newStmt0->stmt.pthread_function = pthf;
                
                // create the function call
                // the call goes in the dispatch loops statement list
                pthread_function_call_t *pthfc = 
                new pthread_function_call_t(name,pthreadUID);
                newStmt1 = new statement_t();
                newStmt1->code = _pthread_function_call;
                newStmt1->stmt.pthread_function_call = pthfc;
                
                // create a statment list for the dispatch loop which 
                // will hold the function call and the function (to be
                // moved later on)
                statement_list_t *dispStatements = new statement_list_t();
                dispStatements->push_back(newStmt1);
                dispStatements->push_back(newStmt0);
                
                // create the dispatch loop with the statement list
                // of the function and function call
                // the dispatch loop goes in the list of the current
                // level statements
                pthread_loop_t *pthd = new pthread_loop_t(
                            dispStatements, &sg->sg_iterator, sg);
                newStmt2->code = _pthread_loop;
                newStmt2->stmt.pthread_loop = pthd;
                statements->push_back(newStmt2);
                
                ++pthreadUID;
            }
            else {
                cout << "code_gen.cpp; build_AST_r(); unhandled code generation type\n";
            }
        }
  	}
     
    return statements;
}

bto_program_t* build_AST(graph &g, string name,
                map<string,type*> &inputs,
                map<string,type*> &outputs,
                map<subgraph*, enum code_gen_t> &genMap) {
    

    pthreadUID = 0;
    // genMap maps each subgraph to a particular type of code generation
    // for example c loop, or pthreads.
    
    // move graph closer to code
    optimize_reductions(g, genMap);
    add_reduction_iters(g, genMap);
    fixup_out_edges(g);

    // subgraph -> sumto means that the subgraph is creating something
    // the size of sumto for each iteration and a reduction is to be
    // performed with those values.  in the absence of a parallel
    // reduction, the reduction can be optimized to a +=.
    // build optimize reduction map
    // vertex -> true if can optimize to +=
    map<vertex,bool> reductionMap;
    // build allocations, and structures that need zeroing
    map<vertex, allocate_t*> allocMap;
    map<vertex, zero_data_t*> zeroMap;
    map<vertex, container_reduction_t*> parallelReduce;
    build_allocations(g, allocMap, zeroMap, reductionMap, 
                      parallelReduce, genMap);

    // build expressions
    // for assignMap details see reduce_expressions()
    map<vertex, assign_t*> assignMap;
    reduce_expressions(g, assignMap, reductionMap, genMap);
    
    vector<vertex> verts;
    for (unsigned int i = 0; i < g.num_vertices(); ++i) {
        if (g.find_parent(i) == NULL)
            verts.push_back(i);
    }
  	statement_list_t *stmts = build_AST_r(g,name,g.subgraphs, NULL, verts, 
                                         genMap, assignMap, allocMap,
                                         zeroMap, parallelReduce);
    
    bto_function_t *fun = new bto_function_t(stmts,name,&inputs,&outputs);
    
    // build program
    bto_program_t *program = build_program(fun, g);
    
    return program;
}

////////////////////////// other ast member functions //////////////
statement_t::~statement_t() {
    switch (code) {
        case _c_loop:
            delete stmt.c_loop;
            break;
        case _pthread_loop:
            delete stmt.pthread_loop;
            break;
        case _pthread_function:
            delete stmt.pthread_function;
            break;
        case _pointer_update:
            delete stmt.pointer_update;
            break;
        case _allocate:
            delete stmt.allocate;
            break;
        case _free:
            delete stmt.free;
            break;
        case _zero_data:
            delete stmt.zero_data;
            break;
        case _assign:
            delete stmt.assign;
            break;
        case _declare:
            delete stmt.declare;
            break;
        case _pthread_function_call:
            delete stmt.pthread_function_call;
            break;
        case _pthread_message:
            delete stmt.pthread_message;
            break;
        case _pthread_message_pack:
            delete stmt.pthread_message_pack;
            break;
        case _pthread_message_unpack:
            delete stmt.pthread_message_unpack;
            break;
        case _declare_partition:
            delete stmt.declare_partition;
            break;
        case _pthread_message_alloc:
            delete stmt.pthread_message_alloc;
            break;
        case _pthread_alloc:
            delete stmt.pthread_alloc;
            break;
        case _pthread_join:
            delete stmt.pthread_join;
            break;
        case _container_reduction:
            delete stmt.container_reduction;
            break;
    }
}

operand_t::~operand_t() {
    switch(code) {
        case _operation:
            delete op.operation;
            break;
        case _access:
            delete op.access;
            break;
        case _constant:
            delete op.constant;
            break;
    }
}

operand_t::operand_t(operand_t *l) {
    code = l->code;
    switch(code) {
        case _operation:
            op.operation =  new operation_t(l->op.operation);
            break;
        case _access:
            op.access = new access_t (l->op.access);
            break;
        case _constant:
            op.constant = new constant_t(l->op.constant);
            break;
    }
}

operand_t::operand_t(access_t *acc, bool makeNew) : code(_access) {
    if (makeNew)
        op.access = new access_t(acc);
    else
        op.access = acc;
}

operand_t::operand_t(operation_t *opp, bool makeNew) : code(_operation) {
    if (makeNew)
        op.operation = new operation_t(opp);
    else
        op.operation = opp;
}

operand_t::operand_t(constant_t *con, bool makeNew) : code(_constant) {
    if (makeNew)
        op.constant = new constant_t(con);
    else
        op.constant = con;
}

container_reduction_t::container_reduction_t(vertex v, graph &g, string to,
                                            string from, bool sclr, 
                                             bool element) :
fromName(from), toName(to), isScalar(sclr), element(element) {

    fromIndex = "";
    toIndex = "";
    zeroCheck = "";
    decls = "";
    
    if (to.compare(from) == 0)
        reduceToSelf = true;
    else {
        reduceToSelf = false;
    }
    
    // find the actual data that we are reducing into.
    vertex ia = g.inv_adj(v)[0];
    vertex last = v;
    vertex a = g.adj(v)[0];
    while (g.info(a).t.k == parallel_reduce)
        a = g.adj(a)[0];
    
    // now build up the loop and indexes
    while (g.info(last).t.k == parallel_reduce) {
        sg_iterator_t &sItr = g.find_parent(ia)->sg_iterator;
        rLoops.push_back(sItr.getSerialCLoop(g.find_parent(ia),true));

        string index = string(1,var_name[depth(g.find_parent(ia))]);
        
        // from side indexing
        if (fromIndex.compare("") == 0)
            fromIndex = index+index + "*" + g.info(last).t.dim.lead_dim;
        else 
            fromIndex += "+" + index+index + "*" + 
                    g.info(last).t.dim.lead_dim;
        
        // to side indexing 
        // here we care about what is under the parallel_reduce 
        // levels of the container to see what was originally happening
        // if there is a difference in the underlying container we
        // need a pointer update, if they are the same, no update is
        // needed.
        // when the underlying containers are the same, this means
        // we want to zero the first access of that portion of the
        // to side of the reduction.
        type *t0 = &g.info(ia).t;
        while (t0->k == parallel_reduce)
            t0 = t0->t;
        type *t1 = &g.info(a).t;
        while (t1->k == parallel_reduce)
            t1 = t1->t;
        if (!total_type_match(*t0,*t1)) {
            // no type match, need a pointer update
            if (toIndex.compare("") == 0)
                toIndex = index + get_next_elem(g.info(a).t);
            else
                toIndex += "+" + index + get_next_elem(g.info(a).t);
        }
        else {
            // have a type match, so we want to add this index to the
            // zero check
            if (zeroCheck.compare("") == 0)
                zeroCheck = "(" + index + index + " == 0)";
            else 
                zeroCheck += "&&(" + index + index + " == 0)";
        }
        a = ia;
        last = ia;
        ia = g.inv_adj(ia)[0];
    }
    
    // determine the details of the container being reduced
    // so loops can be created
    // if this is not reducing to self, then the leading dimension
    // may be different, so track the reduce to type separately.
    type *lt = &g.info(v).t;
    while (lt->k == parallel_reduce) {
        lt = lt->t;
    }
    type *tlt;
    // if self reduction, just make types the same, else
    // separate them and find correct.
    if (reduceToSelf)
        tlt = lt;
    else {
        a = g.adj(v)[0];
        while (g.info(a).t.k == parallel_reduce)
            a = g.adj(a)[0];
        
        tlt = &g.info(a).t;
        while (tlt->k == parallel_reduce) {
            tlt = tlt->t;
        }
    }
    
    int d = depth(g.find_parent(ia));
    rLoops.push_back("ZERO");
    
    // to use the sg_iterator code need to build up subgraphs on the
    // way down through this type. complicated...
    while (lt->k != scalar) {
        
        string index = "__" + string(1,var_name[d++]);
        decls += "int " + index + ";\n";
        
        
        // build loops        
        string lSize = lt->dim.dim;
        while (lSize.find("$$") != string::npos) {
            lSize.replace(lSize.find("$$"),2,"__m");
        }
        
        string lStep = lt->dim.step;
        if (lStep.find("$$") != string::npos)
            lStep.replace(lStep.find("$$"),2,"__s");
        
        string cleanUp = "";
        if (lStep.compare("1") != 0) {
            // int __m20 = i + __s20 > A_ncols ? A_ncols - i : __s20;
            string lM = lt->dim.step;
            lM.replace(0,2,"__m");
            cleanUp = "int " + lM + " = " + index + " + " + lStep;
            cleanUp += " > " + lSize + " ? " + lSize + " - " + index;
            cleanUp += " : " + lStep + ";\n";
        }
        
        string loop = "for (" + index + " = 0; " + index + " < " + lSize;
        loop += "; " + index + "+=" + lStep + ") {\n";
        loop += cleanUp;
        rLoops.push_back(loop);
        
        // build indexing, keepsing reduce to type separate because
        // leading dimesions may be different
        lStep = lt->dim.lead_dim;
        string lStepTo = tlt->dim.lead_dim;
        if (lt->k == lt->get_lowest_ns()->k) {
            lStep = "";
            lStepTo = "";
        }
        else {
            lStep = "*" + lStep; 
            lStepTo = "*" + lStepTo;
        }
        while (lStep.find("$$") != string::npos) {
            lStep.replace(lStep.find("$$"),2,"__m");
        }
        while (lStepTo.find("$$") != string::npos) {
            lStepTo.replace(lStepTo.find("$$"),2,"__m");
        }
        
        if (toIndex.compare("") == 0)
            toIndex = index + lStepTo;
        else
            toIndex += "+" + index + lStepTo;
        
        if (fromIndex.compare("") == 0)
            fromIndex = index + lStep;
        else
            fromIndex += "+" + index + lStep;
        
        lt = lt->t;
        tlt = tlt->t;
    }
}

////////////////////////// ast manipulation ////////////////////////
void get_allocations(statement_list_t &allocs, statement_list_t *stmts) {
    // fill allocs with any allocate_t statements from the statement_list
    // and remove those pointers from the statement_list
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        if ((*i)->code == _allocate) {
            allocs.push_back(*i);
            // erase fires destructor, but need allocs to hold it now
            // so make destructor fire on NULL.
            *i = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void move_allocations_to_front(statement_list_t *stmts) {
    // move any allocations present in this statement list to the front
    // of the list
    statement_list_t::iterator i;
    
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        if ((*i)->code == _allocate) {
            rotate(stmts->begin(),i,i+1);
        }
    }
}

void bto_function_t::hoist_allocations() {
    // hoist allocations below me.
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_allocations();
    }
    
    // move my allocations to the front
    move_allocations_to_front(stmts);
    
    // now can any allocations be pulled up one level?
    // for now child c_loops.
    statement_list_t allocs;
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _c_loop 
            || (*stmts)[i]->code == _pthread_loop) 
            get_allocations(allocs, (*stmts)[i]->stmt.c_loop->stmts);
    }
    
    // insert any children that can hoist in the from of my list
    stmts->insert(stmts->begin(),allocs.begin(),allocs.end());
}

void statement_t::hoist_allocations() {
    switch (code) {
        case _c_loop:
            stmt.c_loop->hoist_allocations();
            break;
        case _pthread_loop:
            stmt.pthread_loop->hoist_allocations();
            break;
        case _pthread_function:
            stmt.pthread_function->hoist_allocations();
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::hoist_allocations(); "
                << "unhandled ast node\n";
    }
}

void c_loop_t::hoist_allocations() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_allocations();
    }
    
    // move my allocations to the front
    move_allocations_to_front(stmts);
    
    // now can any allocations be pulled up one level?
    // in this case that may just be child c_loops.
    statement_list_t allocs;
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _c_loop) 
            get_allocations(allocs, (*stmts)[i]->stmt.c_loop->stmts);
    }
    
    // insert any children that can hoist in the from of my list
    stmts->insert(stmts->begin(),allocs.begin(),allocs.end());
}

void pthread_loop_t::hoist_allocations() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_allocations();
    }
    
    // move my allocations to the front
    move_allocations_to_front(stmts);
}

void pthread_function_t::hoist_allocations() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_allocations();
    }
    
    // move my allocations to the front
    move_allocations_to_front(stmts);
}

void collect_frees(statement_list_t &frees, statement_list_t *stmts) {
    // build a free for each allocation in stmts and put that in frees
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _allocate) {
            // if this allocation is just a single value, then
            // no free is required.
            if ((*stmts)[i]->stmt.allocate->sclr)
                continue;
            
            string name = (*stmts)[i]->stmt.allocate->leftName;
            free_t *f = new free_t(name);
            
            statement_t *s = new statement_t();
            s->code = _free;
            s->stmt.free = f;
            
            frees.push_back(s);
        }
    }
}

void statement_t::insert_frees() {
    switch (code) {
        case _c_loop:
            stmt.c_loop->insert_frees();
            break;
        case _pthread_loop:
            stmt.pthread_loop->insert_frees();
            break;
        case _pthread_function:
            stmt.pthread_function->insert_frees();
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition: 
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::insert_frees(); "
                    << "unhandled ast node\n";
    }
}

void bto_function_t::insert_frees() {
    // find any allocations and insert corresponding free's
    // recurse while build this list
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->insert_frees();
    }
    
    statement_list_t frees;
    collect_frees(frees, stmts);
    stmts->insert(stmts->end(),frees.begin(),frees.end());
}

void c_loop_t::insert_frees() {
    // find any allocations and insert corresponding free's
    // recurse while build this list
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->insert_frees();
    }
    
    statement_list_t frees;
    collect_frees(frees, stmts);
    stmts->insert(stmts->end(),frees.begin(),frees.end());
}

void pthread_loop_t::insert_frees() {
    // find any allocations and insert corresponding free's
    // recurse while build this list
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->insert_frees();
    }
    
    statement_list_t frees;
    collect_frees(frees, stmts);
    stmts->insert(stmts->end(),frees.begin(),frees.end());
}

void pthread_function_t::insert_frees() {
    // find any allocations and insert corresponding free's
    // recurse while build this list
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->insert_frees();
    }
    
    statement_list_t frees;
    collect_frees(frees, stmts);
    stmts->insert(stmts->end(),frees.begin(),frees.end());
}

void move_zeros_to_front(statement_list_t *stmts) {
    // a zero was inserted by a reduction and typically in the graph
    // these vertices are after the loop (i.e. sumto vertex).  these
    // zero nodes need to move above the loop or operation that is
    // causing them.
    // pulling them all to the front of the list breaks inouts
    
    statement_list_t::iterator i;

    for (i = stmts->begin()+1; i < stmts->end(); ++i) {

        if ((*i)->code == _zero_data) {

            string name = (*i)->stmt.zero_data->name;
            
            // look for the first assign node before this zero in the
            // list and place zero immediately before this operation.
            statement_list_t::iterator b = i;
            for (; b >= stmts->begin(); --b) {
                vector<string> assigns;
                (*b)->assigns_to(assigns);
                
                if (find(assigns.begin(),assigns.end(),name) 
                        != assigns.end()) {
                    
                    rotate(b, i, i+1);
                    break;
                }
            }
        }
    }
}

void statement_t::hoist_zeros() {
    switch (code) {
        case _c_loop:
            stmt.c_loop->hoist_zeros();
            break;
        case _pthread_loop:
            stmt.pthread_loop->hoist_zeros();
            break;
        case _pthread_function:
            stmt.pthread_function->hoist_zeros();
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::hoist_zeros(); "
            << "unhandled ast node\n";
    }
}

void bto_function_t::hoist_zeros() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_zeros();
    }
    
    // move my zeros to the front
    move_zeros_to_front(stmts);
}

void c_loop_t::hoist_zeros() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_zeros();
    }
    
    // move my zeros to the front
    move_zeros_to_front(stmts);
}

void pthread_loop_t::hoist_zeros() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_zeros();
    }
    
    // move my zeros to the front
    move_zeros_to_front(stmts);
}

void pthread_function_t::hoist_zeros() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_zeros();
    }
    
    // move my allocations to the front
    move_zeros_to_front(stmts);
}

void move_pointer_updates_to_front(statement_list_t *stmts) {
    // move any pointer updates present in this statement list to the front
    // of the list
    statement_list_t::iterator i;
    
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        if ((*i)->code == _pointer_update) {
            rotate(stmts->begin(),i,i+1);
        }
    }
}

void statement_t::hoist_pointer_updates() {
    switch (code) {
        case _c_loop:
            stmt.c_loop->hoist_pointer_updates();
            break;
        case _pthread_loop:
            stmt.pthread_loop->hoist_pointer_updates();
            break;
        case _pthread_function:
            stmt.pthread_function->hoist_pointer_updates();
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::hoist_zeros(); "
            << "unhandled ast node\n";
    }
}

void bto_function_t::hoist_pointer_updates() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_pointer_updates();
    }
    
    // move my allocations to the front
    move_pointer_updates_to_front(stmts);
}

void c_loop_t::hoist_pointer_updates() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_pointer_updates();
    }
    
    // move my allocations to the front
    move_pointer_updates_to_front(stmts);
}

void pthread_loop_t::hoist_pointer_updates() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_pointer_updates();
    }
    
    // move my allocations to the front
    move_pointer_updates_to_front(stmts);
}

void pthread_function_t::hoist_pointer_updates() {
    // get everything below me hoisted
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->hoist_pointer_updates();
    }
    
    // move my allocations to the front
    move_pointer_updates_to_front(stmts);
}

void statement_t::get_iterators(set<string> &itrs) {
    switch (code) {
        case _c_loop:
            stmt.c_loop->get_iterators(itrs);
            break;
        case _pthread_loop:
            stmt.pthread_loop->get_iterators(itrs);
            break;
        case _pthread_function:
            stmt.pthread_function->get_iterators(itrs);
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::get_iterators(); "
            << "unhandled ast node\n";
    }
}

void bto_function_t::get_iterators(set<string> &itrs) {
    // get any iterators in use
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_iterators(itrs);
    }
}

void c_loop_t::get_iterators(set<string> &itrs) {
    // get any iterators in use
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_iterators(itrs);
    }
    itrs.insert(iterator->getIterator(sub));
}

void pthread_loop_t::get_iterators(set<string> &itrs) {
    // get any iterators in use
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_iterators(itrs);
    }
    string s = iterator->getIterator(sub);
    itrs.insert(s);
    itrs.insert(s+s);
}

void pthread_function_t::get_iterators(set<string> &itrs) {
    // get any iterators in use
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_iterators(itrs);
    }
}

void statement_t::make_parallel_messages(statement_list_t &pMesg,
                                         vector<declare_t> &decls) {
    // make message ast for any parallel functions.  leave a 
    // pack message at the dispatch level, leave a unpack message 
    // at the thread function level, and put a message structure in
    // pMesg.
    switch (code) {
        case _c_loop:
            stmt.c_loop->make_parallel_messages(pMesg, decls);
            break;
        case _pthread_loop:
            stmt.pthread_loop->make_parallel_messages(pMesg, decls);
            break;
        case _pthread_function:
            stmt.pthread_function->make_parallel_messages(pMesg, decls);
            break;
        case _pointer_update:
            stmt.pointer_update->make_parallel_messages(decls);
            break;
        case _zero_data:
            stmt.zero_data->make_parallel_messages(decls);
            break;
        case _assign:
            stmt.assign->make_parallel_messages(decls);
            break;
        case _declare:
            stmt.declare->make_parallel_messages(decls);
            break;
        case _allocate:
            stmt.allocate->make_parallel_messages(decls);
            break;
        case _declare_partition:
            stmt.declare_partition->make_parallel_messages(decls);
            break;
        case _container_reduction:
            stmt.container_reduction->make_parallel_messages(decls);
            break;
        case _pthread_message_alloc:
            stmt.pthread_message_alloc->make_parallel_messages(decls);
            break;
        case _pthread_alloc:
            stmt.pthread_alloc->make_parallel_messages(decls);
            break;
        case _free:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _pthread_join:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::make_parallel_messages(); "
            << "unhandled ast node\n";
    }
}

void declare_partition_t::make_parallel_messages(vector<declare_t> &decls) {
    // parse size and if not in decls, add values
    vector<declare_t>::iterator i;
    if (!fixedSize) {
        string ls = size;
        while (ls.find("*") != string::npos) {
            string t;
            t.assign(ls,0,ls.find("*"));        // pull off up to first *
            ls.replace(0,ls.find("*")+1,"");    // replace up to * with nothing
            
            if (t.find("$$") == 0)
                t.replace(0,2,"__s");
            
            bool insertName = true;
            for (i = decls.begin(); i < decls.end(); ++i) {
                if ((*i).name.compare(t) == 0) {
                    insertName = false;
                    break;
                }
            }
            
            if (insertName) {
                declare_t d("int",t);
                decls.push_back(d);
            }
        }
        
        if (ls.find("$$") == 0)
            ls.replace(0,2,"__s");
        
        // insert remaining portion of size
        bool insertName = true;
        for (i = decls.begin(); i < decls.end(); ++i) {
            if ((*i).name.compare(ls) == 0) {
                insertName = false;
                break;
            }
        }
        
        if (insertName) {
            declare_t d("int",ls);
            decls.push_back(d);
        }
    }

    // remove name from decls as
    // 1) __s*
    // 2) nparts__s*
    string npName = "nparts"+ name;
    for (i = decls.begin(); i < decls.end();) {
        if ((*i).name.compare(name) == 0) {
            i = decls.erase(i);
        }
        else if ((*i).name.compare(npName) == 0) {
            i = decls.erase(i);
        }
        else
            ++i;
    }
}

void pthread_alloc_t::make_parallel_messages(vector<declare_t> &decls) {
    vector<declare_t>::iterator i;
    // parse size and if not in decls, add values
    string ls = size;
    while (ls.find("*") != string::npos) {
        string t;
        t.assign(ls,0,ls.find("*"));        // pull off up to first *
        ls.replace(0,ls.find("*")+1,"");    // replace up to * with nothing
        
        if (t.find("$$") == 0)
            t.replace(0,2,"nparts__s");
        
        bool insertName = true;
        for (i = decls.begin(); i < decls.end(); ++i) {
            if ((*i).name.compare(t) == 0) {
                insertName = false;
                break;
            }
        }
        
        if (insertName) {
            declare_t d("int",t);
            decls.push_back(d);
        }
    }
    
    if (ls.find("$$") == 0)
        ls.replace(0,2,"nparts__s");
    
    // insert remaining portion of size
    bool insertName = true;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(ls) == 0) {
            insertName = false;
            break;
        }
    }
    
    if (insertName) {
        declare_t d("int",ls);
        decls.push_back(d);
    }
}

void pthread_message_alloc_t::make_parallel_messages(
                                    vector<declare_t> &decls) {
    vector<declare_t>::iterator i;
    // parse size and if not in decls, add values
    string ls = size;
    while (ls.find("*") != string::npos) {
        string t;
        t.assign(ls,0,ls.find("*"));        // pull off up to first *
        ls.replace(0,ls.find("*")+1,"");    // replace up to * with nothing
        
        if (t.find("$$") == 0)
            t.replace(0,2,"nparts__s");
        
        bool insertName = true;
        for (i = decls.begin(); i < decls.end(); ++i) {
            if ((*i).name.compare(t) == 0) {
                insertName = false;
                break;
            }
        }
        
        if (insertName) {
            declare_t d("int",t);
            decls.push_back(d);
        }
    }
    
    if (ls.find("$$") == 0)
        ls.replace(0,2,"nparts__s");

    // insert remaining portion of size
    bool insertName = true;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(ls) == 0) {
            insertName = false;
            break;
        }
    }
    
    if (insertName) {
        declare_t d("int",ls);
        decls.push_back(d);
    }
}

void allocate_t::make_parallel_messages(vector<declare_t> &decls) {
    vector<declare_t>::iterator i;
    // parse size and if not in decls, add values

    string ls = size;
    while (ls.find("*") != string::npos) {
        string t;
        t.assign(ls,0,ls.find("*"));        // pull off up to first *
        ls.replace(0,ls.find("*")+1,"");    // replace up to * with nothing
        
        if (t.find("$$") == 0)
            t.replace(0,2,"__s");
        
        bool insertName = true;
        for (i = decls.begin(); i < decls.end(); ++i) {
            if ((*i).name.compare(t) == 0) {
                insertName = false;
                break;
            }
        }
        
        if (insertName) {
            declare_t d("int",t);
            decls.push_back(d);
        }
    }
    
    if (ls.find("$$") == 0)
        ls.replace(0,2,"__s");
    
    // insert remaining portion of size
    bool insertName = true;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(ls) == 0) {
            insertName = false;
            break;
        }
    }
    
    if (insertName) {
        declare_t d("int",ls);
        decls.push_back(d);
    }

    // if we allocate then remove the leftName from decls.
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(leftName) == 0) {
            decls.erase(i);
            break;
        }
    }
}

void pointer_update_t::make_parallel_messages(vector<declare_t> &decls) {
    // if leftName is in decls, remove it 
    // add rightName
    // if parsed update is not in decls, add it

    // parse update; expecting either x*VAL or VAL where we want VAL
    // and x is an index.
    
    string tmp = update;
    size_t loc = tmp.find("*");
    if (loc != string::npos)
        tmp.replace(0,loc+1,"");
    
    // VAL may have val*val...
    string ftmp;
    ftmp.assign(tmp);

    while (ftmp.size()) {
        if (ftmp.find("*") == string::npos) {
            tmp.assign(ftmp);
            ftmp.clear();
        }
        else {
            tmp.assign(ftmp,0,ftmp.find("*"));  // pull off up to first *
            ftmp.replace(0,ftmp.find("*")+1,"");// replace up to * with nothing
        }
        
        if (tmp.find("$$") == 0)
            tmp.replace(0,2,"__s");

        bool foundUpdate = false;
        
        vector<declare_t>::iterator i;
        for (i = decls.begin(); i < decls.end(); ++i) {
            // look for size portion of update
            if ((*i).name.compare(tmp) == 0) {
                foundUpdate = true;
                break;
            }
        }
        
        if (!foundUpdate && loc != string::npos) {
            declare_t t("int",tmp);
            decls.push_back(t);
        }
    }
    
    // check to see if the rightName or leftName are present
    bool foundRightName = false;
    vector<declare_t>::iterator i;
    for (i = decls.begin(); i < decls.end();) {
        
        // is this pointer already in the message
        if ((*i).name.compare(rightName) == 0)
            foundRightName = true;
        
        // does this pointer trump a lower pointer,
        // if so remove it
        if ((*i).name.compare(leftName) == 0)
            i = decls.erase(i);
        else {
            ++i;
        }
    }
    
    // insert if not present
    if (!foundRightName) {
        declare_t d(c_type,rightName,true);
        decls.push_back(d);
    }
}

void zero_data_t::make_parallel_messages(vector<declare_t> &decls) {
    
    // if 'name' is not in decls, then add it
    vector<declare_t>::iterator i;
    bool insertName = true;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(name) == 0) {
            insertName = false;
            break;
        }
    }
    
    if (insertName) {
        declare_t d(precision_type,name,true);
        decls.push_back(d);
    }
    
    // parse size and if not in decls, add values
    string ls = size;
    while (ls.find("*") != string::npos) {
        string t;
        t.assign(ls,0,ls.find("*"));    // pull of up to first *
        ls.replace(0,ls.find("*")+1,"");  // replace up to * with nothing
        
        if (t.find("$$") == 0)
            t.replace(0,2,"__m");
        
        insertName = true;
        for (i = decls.begin(); i < decls.end(); ++i) {
            if ((*i).name.compare(t) == 0) {
                insertName = false;
                break;
            }
        }
        
        if (insertName) {
            declare_t d("int",t);
            decls.push_back(d);
        }
    }
    
    if (ls.find("$$") == 0)
        ls.replace(0,2,"__m");
    
    // insert remaining portion of size
    insertName = true;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(ls) == 0) {
            insertName = false;
            break;
        }
    }
    
    if (insertName) {
        declare_t d("int",ls);
        decls.push_back(d);
    }
}

void access_t::make_parallel_messages(vector<declare_t> &decls) {
    
    // if 'name' is not in decls, then add it
    if (name.compare("disp") == 0)
        return;
    
    vector<declare_t>::iterator i;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(name) == 0)
            return;
    }
    
    bool pointer = !sclr;
    declare_t d(precision_type,name,pointer);
    decls.push_back(d);
}

void operand_t::make_parallel_messages(vector<declare_t> &decls) {
    if (code == _operation)
        op.operation->make_parallel_messages(decls);
    else if (code == _access)
        op.access->make_parallel_messages(decls);
}

void operation_t::make_parallel_messages(vector<declare_t> &decls) {
    
    left->make_parallel_messages(decls);
    if (!unary) {
        right->make_parallel_messages(decls);
    }
}

void assign_t::make_parallel_messages(vector<declare_t> &decls) {
    
    // add leftHandSide
    leftHandSide->make_parallel_messages(decls);
    // make sure that scalar values being assigned to are pointers
    // as long as they are not integeter sizes
    if (decls.back().c_type.compare("int") != 0)
        decls.back().ptr = true;
    
    // now the right hand side
    if (rhsCode == _rhs_operation)
        rightHandSide.operation->make_parallel_messages(decls);
    else if (rhsCode == _rhs_access)
        rightHandSide.access->make_parallel_messages(decls);
}

void declare_t::make_parallel_messages(vector<declare_t> &decls) {
    // if 'name' is not in decls, then add it    
    vector<declare_t>::iterator i;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(name) == 0)
            return;
    }
    
    declare_t d(c_type,name,ptr);
    decls.push_back(d);
}

void container_reduction_t::make_parallel_messages(
                                    vector<declare_t> &decls) {
    // if 'name' is not in decls, then add it
    vector<declare_t>::iterator i;
    bool foundFrom = false;
    bool foundTo = false;
    for (i = decls.begin(); i < decls.end(); ++i) {
        if ((*i).name.compare(fromName) == 0)
            foundFrom = true;
        if ((*i).name.compare(toName) == 0)
            foundTo = true;
    }
    
    if (!foundFrom) {
        declare_t d(precision_type,fromName,true);
        decls.push_back(d);
    }
    
    // determine if toName is scalar
    bool ptr = true;
    if (isScalar && !element)
        ptr = false;
    
    if (!foundTo && fromName.compare(toName) != 0) {
        declare_t d(precision_type,toName,ptr);
        decls.push_back(d);
    }
}

string scrub_max(string s) {
    while (s.find("max") != string::npos)
        s = s.erase(s.find("max"),3);
 
    while (s.find(")") != string::npos)
        s = s.erase(s.find(")"),1);

    while (s.find("(") != string::npos)
        s = s.erase(s.find("("),1);
    
    return s;
}

void remove_mSize_decls(vector<declare_t> &decls, vector<iterOp_t> &itr) {
    // if this loop is partitioned, then it will introduce
    // __m* of step.  that should be removed from decls if present
    
    vector<iterOp_t>::iterator i;
    vector<declare_t>::iterator j;
    for (i = itr.begin(); i != itr.end(); ++i) {
        string complete = scrub_dollar(i->right, "__m");
        complete = scrub_max(complete);
        
        vector<string> separate;
        boost::split(separate, complete, boost::is_any_of("+-,"));
        
        for (unsigned int k = 0; k < separate.size(); ++k) {
            string rVal = separate[k];
            
            for (j = decls.begin(); j < decls.end(); ++j) {
                if ((*j).name.compare(rVal) == 0) {
                    decls.erase(j);
                    break;
                }
            }
            
        }
    }
    //for (int i = 0; i < decls.size(); ++i) {
    //    std::cout << decls[i].to_string() << "\n";
    //}
}

void update_decls(vector<declare_t> &decls, vector<iterOp_t> &itr) {
    // first add anything in the right hand side to the decls
    vector<iterOp_t>::iterator i;
    vector<declare_t>::iterator j;
    for (i = itr.begin(); i != itr.end(); ++i) {
        string complete = scrub_dollar(i->right,
                    i->oper.find("+") != string::npos ? "__s" : "__m");
        complete = scrub_max(complete);
        
        vector<string> separate;
        boost::split(separate, complete, boost::is_any_of("+-,"));
        
        for (unsigned int k = 0; k < separate.size(); ++k) {
            string rVal = separate[k];
            
            if (rVal.compare("0") == 0 || rVal.compare("1") == 0) continue;
            
            bool foundVal = false;
            for (j = decls.begin(); j < decls.end(); ++j) {
                if ((*j).name.compare(rVal) == 0) {
                    foundVal = true;
                    break;
                }
            }
            
            if (!foundVal) {
                //std::cout << "inserting " << rVal;
                declare_t t("int",rVal);
                decls.push_back(t);
                
            }
        }
    }
    
    //for (int i = 0; i < decls.size(); ++i) {
    //    std::cout << decls[i].to_string() << "\n";
    //}
    
    for (i = itr.begin(); i != itr.end(); ++i) {
        if (i->oper.compare("=") == 0) continue;
        
        string complete = scrub_dollar(i->left, "__m");
        
        vector<string> separate;
        boost::split(separate, complete, boost::is_any_of("+-,"));
        
        for (unsigned int k = 0; k < separate.size(); ++k) {
            string rVal = separate[k];
            
            if (rVal.compare("0") == 0 || rVal.compare("1") == 0) continue;
            
            bool foundVal = false;
            for (j = decls.begin(); j < decls.end(); ++j) {
                if ((*j).name.compare(rVal) == 0) {
                    foundVal = true;
                    break;
                }
            }
            
            if (!foundVal) {
                //std::cout << "inserting " << rVal;
                declare_t t("int",rVal);
                decls.push_back(t);
            }
        }
    }
    
    // and remove anything in the left hand side from decls when oper == "="
    for (i = itr.begin(); i != itr.end(); ++i) {
        if (i->oper.compare("=") != 0) continue;
        
        string rVal = i->left;
    
        for (j = decls.begin(); j < decls.end(); ++j) {
            if ((*j).name.compare(rVal) == 0) {
                //std::cout << "erasing " << rVal << "\n";
                j = decls.erase(j);
                continue;
            }
        }
    }
}

void c_loop_t::make_parallel_messages(statement_list_t &pMesg,
                                      vector<declare_t> &decls) {
    // make message ast for any parallel functions.  leave a 
    // pack message at the dispatch level, leave a unpack message 
    // at the thread function level, and put a message structure in
    // pMesg.
    // backwards because things should be in serial order now
    statement_list_t::reverse_iterator i;
    for (i = stmts->rbegin(); i < stmts->rend(); ++i) {
        (*i)->make_parallel_messages(pMesg, decls);
    }

    update_decls(decls, iterator->updates);
    update_decls(decls, iterator->conditions);
    update_decls(decls, iterator->initializations);
    remove_mSize_decls(decls, iterator->updates);
    
    //for (int i = 0; i < decls.size(); ++i) {
    //    std::cout << decls[i].to_string() << "\n";
    //}
}

void pthread_loop_t::make_parallel_messages(statement_list_t &pMesg,
                                            vector<declare_t> &decls) {
    // make message ast for any parallel functions.  leave a 
    // pack message at the dispatch level, leave a unpack message 
    // at the thread function level, and put a message structure in
    // pMesg.
    
    // the pointer updates at the top level of the function this
    // is dispatching to need to be pulled up to this level
    statement_list_t::iterator f;
    statement_list_t ls;
    for (f = stmts->begin(); f < stmts->end(); ++f) {
        if ((*f)->code == _pthread_function) {
            (*f)->stmt.pthread_function->handle_top_level_accesses(ls);
        }
    }
    stmts->insert(stmts->begin(),ls.begin(),ls.end());
    
    // now recursively find the values that need to be put in the
    // message.
    // backwards because things should be in serial order now
    statement_list_t::reverse_iterator i;
    statement_list_t packers;
    for (i = stmts->rbegin(); i < stmts->rend(); ++i) {
        (*i)->make_parallel_messages(pMesg, decls);
        
        // if statement is a pthread_function then it just created
        // a pthread_message, and we need the packer here.
        // the pthread_message will be the last value placed on
        // pMesg
        if ((*i)->code == _pthread_function) {
            statement_t *pck = new statement_t();
            pck->code = _pthread_message_pack;
            pck->stmt.pthread_message_pack = pMesg.back()->
                stmt.pthread_message->make_message_packer();
            packers.push_back(pck);
        }
    }
    
    // now place any packers immediately before their corresponding
    // function calls
    statement_list_t::iterator f0,f1;
    for (f0 = packers.begin(); f0 < packers.end(); ++f0) {
        int uid = (*f0)->stmt.pthread_message_pack->uid;
        for (f1 = stmts->begin(); f1 < stmts->end(); ++f1) {
            if ((*f1)->code == _pthread_function_call && 
                uid == (*f1)->stmt.pthread_function_call->uid) {
                stmts->insert(f1,*f0);
                break;
            }
        }
    }
    
    // now the loop details.
    update_decls(decls, iterator->updates);
    update_decls(decls, iterator->conditions);
    update_decls(decls, iterator->initializations);
    remove_mSize_decls(decls, iterator->updates);
}

void pthread_function_t::handle_top_level_accesses(
                        statement_list_t &access) {
    // all of the accesses in the thread function need to be 
    // pulled up into the dispatch level
    // just above the function.  
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        if ((*i)->code == _pointer_update) {
            if ((*i)->stmt.pointer_update->cast) {
                ++i;
                continue;
            }
            access.push_back(*i);
            *i = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void pthread_function_t::make_parallel_messages(statement_list_t &pMesg,
                                                vector<declare_t> &decls) {
    // make message ast for any parallel functions.  leave a 
    // pack message at the dispatch level, leave a unpack message 
    // at the thread function level, and put a message structure in
    // pMesg.
    
    vector<declare_t> ldecls;
    // backwards because things should be in serial order now
    statement_list_t::reverse_iterator i;
    for (i = stmts->rbegin(); i < stmts->rend(); ++i) {
        (*i)->make_parallel_messages(pMesg, ldecls);
    }
    
    statement_list_t *mesgStmts = new statement_list_t();
    int cnt = 0;
    for (unsigned int i = 0; i < ldecls.size(); ++i) {
        if (ldecls[i].name.compare("1") == 0)
            continue;
        
        statement_t *s = new statement_t();
        s->code = _declare;
        s->stmt.declare = new declare_t(ldecls[i]);
        mesgStmts->push_back(s);
        ++cnt;
    }
    
    if (cnt) {
        statement_t *mesg = new statement_t();
        mesg->code = _pthread_message;
        mesg->stmt.pthread_message =  new pthread_message_t(mesgStmts, 
                                                name, uid);
        pMesg.push_back(mesg);
        
        // get the unpacker
        statement_t *unpck = new statement_t();
        unpck->code = _pthread_message_unpack;
        unpck->stmt.pthread_message_unpack = mesg->stmt.pthread_message->
            make_message_unpacker();
        stmts->insert(stmts->begin(),unpck);
    }
    else {
        delete mesgStmts;
    }
    
    // after using ldecls (declarations recursively deeper) append
    // these to decls to build complete set and ensure all values
    // are passed through in a nested pthread scenario
    vector<declare_t>::iterator j;
    for (j = ldecls.begin(); j != ldecls.end(); ++j) {
        if (find(decls.begin(),decls.end(),*j) == decls.end())
            decls.push_back(*j);
    }
}

void bto_function_t::make_parallel_messages(statement_list_t &pMesg) {
    // make message ast for any parallel functions.  leave a 
    // pack message at the dispatch level, leave a unpack message 
    // at the thread function level, and put a message structure in
    // pMesg.
    
    vector<declare_t> decls;
    // backwards because things should be in serial order now
    statement_list_t::reverse_iterator i;
    for (i = stmts->rbegin(); i < stmts->rend(); ++i) {
        (*i)->make_parallel_messages(pMesg, decls);
    }
}

pthread_message_pack_t *pthread_message_t::make_message_packer() {
    pthread_message_pack_t *pack = 
            new pthread_message_pack_t(stmts,name,uid);
    return pack;
}

pthread_message_unpack_t *pthread_message_t::make_message_unpacker() {
    pthread_message_unpack_t *unpack = 
            new pthread_message_unpack_t(stmts,name,uid);
    return unpack;
}

void pthread_loop_t::allocate_pthread_and_message(
                            map<int, vector<string> > &sizeMap,
                            vector<string> &sizes) {
    
    // add step to sizes
    sizes.push_back(iterator->updates.begin()->right);
    
    // continue recursing
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        (*i)->allocate_pthread_and_message(sizeMap, sizes);
    }
}

void pthread_function_t::allocate_pthread_and_message(
                                map<int, vector<string> > &sizeMap,
                                vector<string> &sizes) {
    // sized used up to this point.
    sizeMap[uid] = sizes;
    
    // traverse statements and insert thread and message allocations
    // after recursing down to find out the set of partitions used
    // in the loop nests so we know how many threads are needed.
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        // gather all of the steps from the loops down to each
        // thread call and map them to a thread unique id.
        map<int, vector<string> > lSizeMap;
        vector<string> lSizes;
        (*i)->allocate_pthread_and_message(lSizeMap, lSizes);
        
        map<int, vector<string> >::iterator j;
        for (j = lSizeMap.begin(); j != lSizeMap.end(); ++j) {
            
            string size = "";
            for (unsigned int l = 0; l < j->second.size(); ++l) {
                string s = j->second[l];
                s.replace(0,2,"nparts__s");
                size += s + "*";
            }
            size.replace(size.size()-1,1,"");
            
            // message and thread allocation
            statement_t *ns0 = new statement_t();
            ns0->code = _pthread_alloc;
            ns0->stmt.pthread_alloc = new pthread_alloc_t(j->first,size);
            statement_t *ns1 = new statement_t();
            ns1->code = _pthread_message_alloc;
            ns1->stmt.pthread_message_alloc = new pthread_message_alloc_t(
                                j->first, size, name);
            
            i = stmts->insert(i,ns0);
            i = stmts->insert(i,ns1);
            i += 2;
            
            // disp = 0
            ns0 = new statement_t();
            ns0->code = _assign;
            access_t *lhs = new access_t("disp","",true);
            rhs_union rhs;
            rhs.constant = new constant_t("int",(int)0);
            ns0->stmt.assign = new assign_t(lhs,rhs,_rhs_constant, false);
            i = stmts->insert(i,ns0);
            i += 1;
            
            // thread join
            ns0 = new statement_t();
            ns0->code = _pthread_join;
            ns0->stmt.pthread_join = new pthread_join_t(j->first,size);
            i = stmts->insert(i+1,ns0);
            
            // message and thread free's
            ns0 = new statement_t();
            ns0->code = _free;
            ns0->stmt.free = new free_t("threads_" + 
                                boost::lexical_cast<string>(j->first));
            ns1 = new statement_t();
            ns1->code = _free;
            ns1->stmt.free = new free_t(name + "_" + 
                                boost::lexical_cast<string>(j->first));
            
            i = stmts->insert(i+1,ns0);
            i = stmts->insert(i+1,ns1);
        } 
    }
}

void statement_t::allocate_pthread_and_message(
                            map<int, vector<string> > &sizeMap,
                            vector<string> &sizes) {
    switch (code) {
        case _pthread_loop:
            stmt.pthread_loop->allocate_pthread_and_message(
                                    sizeMap, sizes);
            break;
        case _pthread_function:
            stmt.pthread_function->allocate_pthread_and_message(
                                    sizeMap,sizes);
            break;
        case _c_loop:
            // only applies to pthread loop at this point.
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::allocate_pthread_and_message(); "
            << "unhandled ast node\n";
    }
}

void bto_function_t::allocate_pthread_and_message() {
    // traverse statements and insert thread and message allocations
    // after recursing down to find out the set of partitions used
    // in the loop nests so we know how many threads are needed.
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        // gather all of the steps from the loops down to each
        // thread call and map them to a thread unique id.
        map<int, vector<string> > sizeMap;
        vector<string> sizes;
        (*i)->allocate_pthread_and_message(sizeMap, sizes);
        
        map<int, vector<string> >::iterator j;
        for (j = sizeMap.begin(); j != sizeMap.end(); ++j) {
            
            string size = "";
            for (unsigned int l = 0; l < j->second.size(); ++l) {
                string s = j->second[l];
                s.replace(0,2,"nparts__s");
                size += s + "*";
            }
            size.replace(size.size()-1,1,"");
            
            // message and thread allocation
            statement_t *ns0 = new statement_t();
            ns0->code = _pthread_alloc;
            ns0->stmt.pthread_alloc = new pthread_alloc_t(j->first,size);
            statement_t *ns1 = new statement_t();
            ns1->code = _pthread_message_alloc;
            ns1->stmt.pthread_message_alloc = new pthread_message_alloc_t(
                         j->first, size, name);

            i = stmts->insert(i,ns0);
            i = stmts->insert(i,ns1);
            i += 2;
            
            // disp = 0
            ns0 = new statement_t();
            ns0->code = _assign;
            access_t *lhs = new access_t("disp","",true);
            rhs_union rhs;
            rhs.constant = new constant_t("int",(int)0);
            ns0->stmt.assign = new assign_t(lhs,rhs,_rhs_constant, false);
            i = stmts->insert(i,ns0);
            i += 1;
            
            // thread join
            ns0 = new statement_t();
            ns0->code = _pthread_join;
            ns0->stmt.pthread_join = new pthread_join_t(j->first,size);
            i = stmts->insert(i+1,ns0);
            
            // message and thread free's
            ns0 = new statement_t();
            ns0->code = _free;
            ns0->stmt.free = new free_t("threads_" + 
                            boost::lexical_cast<string>(j->first));
            ns1 = new statement_t();
            ns1->code = _free;
            ns1->stmt.free = new free_t(name + "_" + 
                                boost::lexical_cast<string>(j->first));
            
            i = stmts->insert(i+1,ns0);
            i = stmts->insert(i+1,ns1);
        } 
    }
}

void statement_t::hoist_parallel_functions(statement_list_t &pFunc) {
    switch (code) {
        case _c_loop:
            stmt.c_loop->hoist_parallel_functions(pFunc);
            break;
        case _pthread_loop:
            stmt.pthread_loop->hoist_parallel_functions(pFunc);
            break;
        case _pthread_function:
            stmt.pthread_function->hoist_parallel_functions(pFunc);
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::hoist_parallel_functions(); "
            << "unhandled ast node\n";
    }
}

void c_loop_t::hoist_parallel_functions(statement_list_t &pFunc) {
    // not sure if we will ever have functions inside another c_loop
    // but not disallowing it. (this would be creating a thread from
    // a tiled loop or some such)
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        (*i)->hoist_parallel_functions(pFunc);
        if ((*i)->code == _pthread_function) {
            pFunc.push_back(*i);
            // trick the destructore which fires with erase
            (*i) = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void pthread_loop_t::hoist_parallel_functions(statement_list_t &pFunc) {
    // find any functions in this statement list
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        (*i)->hoist_parallel_functions(pFunc);
        if ((*i)->code == _pthread_function) {
            pFunc.push_back(*i);
            // trick the destructore which fires with erase
            (*i) = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void pthread_function_t::hoist_parallel_functions(statement_list_t &pFunc) {
    // not sure if we will ever have functions inside another function
    // but not disallowing it. (this would be creating a thread from
    // a thread that was already created)
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        (*i)->hoist_parallel_functions(pFunc);
        if ((*i)->code == _pthread_function) {
            pFunc.push_back(*i);
            // trick the destructore which fires with erase
            (*i) = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void bto_function_t::hoist_parallel_functions(statement_list_t &pFunc) {
    // parallel message should already be handled, now move the actual
    // parallel functions into pFunc
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end();) {
        (*i)->hoist_parallel_functions(pFunc);
        if ((*i)->code == _pthread_function) {
            pFunc.push_back(*i);
            // trick the destructore which fires with erase
            (*i) = NULL;
            i = stmts->erase(i);
        }
        else {
            ++i;
        }
    }
}

void statement_t::get_partitions_used(map<string,int> &sizeMap,
                                      map<string,string> &partDeps,
                                      set<string> &parts) {
    switch (code) {
        case _c_loop:
            stmt.c_loop->get_partitions_used(sizeMap,partDeps,parts);
            break;
        case _pthread_loop:
            stmt.pthread_loop->get_partitions_used(sizeMap,partDeps,parts);
            break;
        case _pthread_function:
            stmt.pthread_function->get_partitions_used(sizeMap,partDeps,
                                                       parts);
            break;
        case _pointer_update:
        case _allocate:
        case _free:
        case _zero_data:
        case _assign:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
        case _container_reduction:
            // terminal's no allocations here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::get_iterators(); "
            << "unhandled ast node\n";
    }
}

void insert_ordered_part_decl(statement_list_t *stmts, statement_t *stmt,
                              map<string,string> &partDeps) {
    // partition declarations can depend on one another when they
    // are a number of partions of a given size.  so we mush put them in 
    // the correct order
    // this assumes that the first statements of stmts are the decls
    // and anything beyond is of no interest.

    // any partition that is an absolute size has just
    // been inserted in the front of stmts so this function only 
    // takes stmt with a size component
    
    // partDeps maps dependencies between partitions such that given
    // a map iterator
    // first > second which also means that first should be
    // declared before second
    
    // for up to two levels of partitioning, if a partition is not in
    // partDeps just stick at end of decls.  if it is in partDeps, stick
    // in front.  this will need to be revisited to handle more than
    // 2 levels.
    
    string nameM = stmt->stmt.declare_partition->name;
    nameM.replace(0,3,"$$");
    if (partDeps.find(nameM) != partDeps.end()) {
        stmts->insert(stmts->begin(),stmt);
        return;
    }
    
    // otherwise stick at end of decls
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        statement_t &s = *(*i);
        // if hit the end of the declare_partition types then
        // insert here
        if (s.code != _declare_partition)
            break;
    }
    
    stmts->insert(i,stmt);
}

void build_partition_decl(string part, map<string,int> &sizeMap,
                          map<string,string> &partDeps,
                          statement_list_t *stmts) {
    // build declaration for partitions and insert into front of
    // stmts.
    // supporting two idoms
    // 1) $$4  means partition 4 is an absolute size:
    //          int __s4 = sizeMap[part];
    // 2) ##4@SIZE  means partition 4 is a number of partitions desired
    //              from SIZE
    //          int nparts__s4 = sizeMap[part];
    //          int __s4 = SIZE/nparts__s4; (adjusted to handle cleanup)
    
    if (part.find("$$") == 0) {
        string s = part;
        s.replace(0,2,"__s");
        declare_partition_t *decl = new declare_partition_t(s,sizeMap[part]);
        statement_t *stmt1 = new statement_t();
        stmt1->code = _declare_partition;
        stmt1->stmt.declare_partition = decl;
        stmts->insert(stmts->begin(),stmt1);
    }
    else if (part.find("##") == 0) {
        string s = part;
        s.replace(0,2,"__s");
        s.replace(s.find("@"),s.size(),"");
        
        string size = part;
        size.replace(0,size.find("@")+1,"");
        
        part.replace(part.find("@"),part.size(),"");
        part.replace(0,2,"$$");
        declare_partition_t *decl = new declare_partition_t(s,sizeMap[part],
                                                            size);
        statement_t *stmt1 = new statement_t();
        stmt1->code = _declare_partition;
        stmt1->stmt.declare_partition = decl;
        insert_ordered_part_decl(stmts,stmt1, partDeps);
    }
}

void bto_function_t::get_partitions_used(map<string,int> &sizeMap,
                                         map<string,string> &partDeps) {
    // find any loops in the function and see if they are partitioned
    // if so get details.
    set<string> partitions;
    
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_partitions_used(sizeMap, partDeps, partitions);
    }
    
    set<string>::iterator j;
    for (j = partitions.begin(); j != partitions.end(); ++j) {
        build_partition_decl(*j, sizeMap, partDeps, stmts);
    }
}


void pthread_function_t::get_partitions_used(map<string,int> &sizeMap,
                                             map<string,string> &partDeps,
                                             set<string> &parts) {
    // find any loops in the function and see if they are partitioned
    // if so get details.    
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_partitions_used(sizeMap, partDeps, parts);
    }
}

void c_loop_t::get_partitions_used(map<string,int> &sizeMap,
                                   map<string,string> &partDeps,
                                   set<string> &parts) {
    // find any loops in the function and see if they are partitioned
    // if so get details.
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_partitions_used(sizeMap, partDeps, parts);
    }
    // a step means an introduction of this partition and therefore
    // a definition.  an iteration mean some parent subgraph is
    // controlling this iteration and the partition should have been
    // defined by that controlling subgraph.
    vector<iterOp_t>::iterator j;
    for (j = iterator->updates.begin(); j != iterator->updates.end();
         ++j) {
        if (j->right.find("$$") == 0)
            parts.insert(j->right);
    }
}

void pthread_loop_t::get_partitions_used(map<string,int> &sizeMap,
                                         map<string,string> &partDeps,
                                         set<string> &parts) {
    // find any loops in the function and see if they are partitioned
    // if so get details.
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->get_partitions_used(sizeMap, partDeps, parts);
    }
    // a step means an introduction of this partition and therefore
    // a definition.  an iteration mean some parent subgraph is
    // controlling this iteration and the partition should have been
    // defined by that controlling subgraph.
    vector<iterOp_t>::iterator j;
    for (j = iterator->updates.begin(); j != iterator->updates.end();
         ++j) {
        vector<iterOp_t>::iterator k;
        for (k = iterator->conditions.begin();
             k != iterator->conditions.end(); ++k) {
            if (j->left.compare(k->left) == 0)
                break;
        }
        if (k == iterator->conditions.end()) continue;
        string ls = j->right;
        ls.replace(0,2,"##");
        ls += "@" + k->right;
        parts.insert(ls);
    }
}

void statement_t::assigns_to(vector<string> &assgns) {
    // find all values assigned to
    switch (code) {
        case _c_loop:
            stmt.c_loop->assigns_to(assgns);
            break;
        case _pthread_loop:
            stmt.pthread_loop->assigns_to(assgns);
            break;
        case _pthread_function:
            stmt.pthread_function->assigns_to(assgns);
            break;
        case _assign:
            stmt.assign->assigns_to(assgns);
            break;
        case _pointer_update:
            stmt.pointer_update->assigns_to(assgns);
            break;
        case _container_reduction:
            stmt.container_reduction->assigns_to(assgns);
            break;
        case _allocate:
        case _free:
        case _zero_data:
        case _declare:
        case _pthread_function_call:
        case _pthread_message:
        case _pthread_message_pack:
        case _pthread_message_unpack:
        case _declare_partition:
        case _pthread_message_alloc:
        case _pthread_alloc:
        case _pthread_join:
            // terminal's no assigns here
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::assigns_to(); "
            << "unhandled ast node\n";
    }
}

void pointer_update_t::assigns_to(vector<string> &assgns) {
    // if updating a pointer of some value that is being assigned
    // to (in assgns already), add rightName to assngs
    if (find(assgns.begin(),assgns.end(),leftName) != assgns.end())
        assgns.push_back(rightName);
}

void container_reduction_t::assigns_to(vector<string> &assgns) {
    // if fromName is assigned to, add toName
    if (find(assgns.begin(),assgns.end(),fromName) != assgns.end())
        assgns.push_back(toName);
}

void access_t::assigns_to(vector<string> &assgns) {
    // only getting here from assign_t leftHandSide, so
    // just add value accessed
    assgns.push_back(name);
}
                
void assign_t::assigns_to(vector<string> &assgns) {
    // add value assigned to
    leftHandSide->assigns_to(assgns);
}

void c_loop_t::assigns_to(vector<string> &assgns) {
    // find all nested assignments
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->assigns_to(assgns);
    }
    // then make sure pointer updates are handled
    // i.e.  pointer_update followed by loop that assigns
    //      if pointer_update is done first, and then the following
    //      loop adds something to assgns, we may miss the update
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _pointer_update) 
             (*stmts)[i]->assigns_to(assgns);
    }
}

void pthread_loop_t::assigns_to(vector<string> &assgns) {
    // find all nested assignments
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->assigns_to(assgns);
    }
    // then make sure pointer updates are handled
    // i.e.  pointer_update followed by loop that assigns
    //      if pointer_update is done first, and then the following
    //      loop adds something to assgns, we may miss the update
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _pointer_update) 
            (*stmts)[i]->assigns_to(assgns);
    }
}

void pthread_function_t::assigns_to(vector<string> &assgns) {
    // find all nested assignments
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        (*stmts)[i]->assigns_to(assgns);
    }
    // then make sure pointer updates are handled
    // i.e.  pointer_update followed by loop that assigns
    //      if pointer_update is done first, and then the following
    //      loop adds something to assgns, we may miss the update
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        if ((*stmts)[i]->code == _pointer_update) 
            (*stmts)[i]->assigns_to(assgns);
    }
}

////////////////////////// code gen ///////////////////////

string operand_t::to_string() {
    switch (code) {
        case _operation:
            return op.operation->to_string();
        case _access:
            return op.access->to_string();
        case _constant:
            return op.constant->to_string();
        default:
            std::cout << "code_gen.cpp: operant_t::to_string(); unexpected "
                << "operand type\n";
    }
    return "";
}

string constant_t::to_string() {
    if (c_type.compare("int") == 0)
        return boost::lexical_cast<string>(val.i);
    else if (c_type.compare("float") == 0)
        return boost::lexical_cast<string>(val.f);
    else if (c_type.compare("double") == 0)
        return boost::lexical_cast<string>(val.d);
    
    std::cout << "code_gen.cpp: constant_t::to_string(); unexpected "
            << "constant type: " << c_type << "\n";
    
    return "";
}

string operation_t::to_string() {
    if (unary) 
        return "(" + op_to_string(op) + "(" + left->to_string() + "))";
    
    return "(" + left->to_string() + op_to_string(op) + 
    right->to_string() + ")";
}

string assign_t::to_string() {
    string assign = sum ? " += " : " = ";
    string f = leftHandSide->to_string() + assign;
    
    switch (rhsCode) {
        case _rhs_operation:
            f += rightHandSide.operation->to_string();
            break;
        case _rhs_access:
            f += rightHandSide.access->to_string();
            break;
        case _rhs_constant:
            f += rightHandSide.constant->to_string();
            break;
        default:
            cout << "code_gen.cpp; assign_t::to_string(); unexpected type\n";
    }
    
    return f + ";";
}

string access_t::to_string() {
    if (sclr)
        return name;
    return name + "[" + index + "]";
}

string allocate_t::to_string() {
    string s = size;
    
    size_t loc = s.find("$$");
    while (loc != string::npos) {
        s.replace(loc,2,"__s");
        loc = s.find("$$");
    }
    
    if (sclr)
        return c_type + " " + leftName + ";";
    else
        return c_type + " *" + leftName + " = malloc(sizeof(" + c_type
        + ")*" + s + ");";
}

string free_t::to_string() {
    return "free(" + name + ");";
}

string declare_t::to_string() {
    if (ptr)
        return c_type + " *" + name + ";";
    else 
        return c_type + " " + name + ";";
}

string zero_data_t::to_string() {
    if (sclr) {
        if (element)
            return name + "[" + itr + "] = 0.0;";
        else
            return name + " = 0.0;";
    }
    string s = size;
    while (s.find("$$") != string::npos)
        s.replace(s.find("$$"),2,"__m");
    
    return "for (__zr__ = 0; __zr__ < " + s + "; ++__zr__) " + 
            name + "[__zr__] = 0.0;";
}

string pointer_update_t::to_string() {
    string lupdate = update;
    while (lupdate.find("$$") != string::npos) {
        lupdate.replace(lupdate.find("$$"),2,"__s");
    }
    string s = "";
    s = c_type + " *" + leftName + " = " + rightName;
    if (update.compare("") != 0)
        s += " + " + lupdate;
    
    s += ";";
    return s;
}

string statement_t::to_string() {
    string s = "";
    switch (code) {
        case _c_loop:
            s += stmt.c_loop->to_string();
            break;
        case _pthread_loop:
            s += stmt.pthread_loop->to_string();
            break;
        case _pthread_function:
            s += stmt.pthread_function->to_string();
            break;
        case _pthread_function_call:
            s += stmt.pthread_function_call->to_string();
            break;
        case _pointer_update:
            s += stmt.pointer_update->to_string();
            break;
        case _allocate:
            s += stmt.allocate->to_string();
            break;
        case _free:
            s += stmt.free->to_string();
            break;
        case _zero_data:
            s += stmt.zero_data->to_string();
            break;
        case _assign:
            s += stmt.assign->to_string();
            break;
        case _declare:
            s += stmt.declare->to_string();
            break;
        case _pthread_message:
            s += stmt.pthread_message->to_string();
            break;
        case _pthread_message_pack:
            s += stmt.pthread_message_pack->to_string();
            break;
        case _pthread_message_unpack:
            s += stmt.pthread_message_unpack->to_string();
            break;
        case _declare_partition:
            s += stmt.declare_partition->to_string();
            break;
        case _pthread_message_alloc:
            s += stmt.pthread_message_alloc->to_string();
            break;
        case _pthread_alloc:
            s += stmt.pthread_alloc->to_string();
            break;
        case _pthread_join:
            s += stmt.pthread_join->to_string();
            break;
        case _container_reduction:
            s += stmt.container_reduction->to_string();
            break;
        default:
            std::cout << "code_gen.cpp; statement_t::to_string(); unhandled "
                << "ast node\n";
    }
    return s;
}

string bto_program_t::to_string() {
    string p = "";
    
    p += "#include <stdlib.h>\n";
    p += "#include <math.h>\n";
    p += "#include <pthread.h>\n";
    p += "#define max(__a__,__b__) (__a__ > __b__ ? __a__ : __b__)\n";
    p += "\n\n";
    
    // parallel mesg structures
    for (unsigned int i = 0; i < parallel_mesgs->size(); ++i) {
        p += (*parallel_mesgs)[i]->to_string() + "\n";
    }
    
    // parallel functions
    for (unsigned int i = 0; i < parallel_funcs->size(); ++i) {
        p += (*parallel_funcs)[i]->to_string() + "\n\n";
    }
    
    // main function
    p += func->to_string();
    
    return p;
}

string bto_function_t::to_string() {
    string f = "";
    
    // string of pointers for scalar output
    string ptrOutputLcl;
    string ptrOutputEnd;
    for (map<string,type*>::const_iterator i = outputs->begin(); 
         i != outputs->end(); ++i) {
        if (i->second->k == scalar) {
            // create local working scalar value
            ptrOutputLcl += type_to_c(*i->second) + " " + i->first + " = ";
            ptrOutputLcl += "*" + i->first + "_ptr;\n";
            // create store back to argument
            ptrOutputEnd +="*" + i->first + "_ptr = " + i->first + ";\n";
        }
    }
    
    // map for function (required by noPtr)
    // not sure how to deal with that exactly?
	map<string, pair<vertex, type> > data;
    
    f += make_function_signature(name,*inputs,*outputs,data,"",
                                 typeWithName,false);
    
    set<string> itrs;
    get_iterators(itrs);
    set<string>::iterator it;
    f += "int disp, __zr__";
    for (it = itrs.begin(); it != itrs.end(); ++it)
        f += "," + *it;
    f += ";\n";
    
    f += ptrOutputLcl + "\n";
    
    
    // statements
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        f += (*stmts)[i]->to_string() + "\n";
    }
    
    f += ptrOutputEnd;
    
    return f + "}\n";
}

string c_loop_t::to_string() {
    string l = iterator->getSerialCLoop(sub);
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        l += (*stmts)[i]->to_string() + "\n";
    }
    l += "}";
    
    return l;
}

string pthread_loop_t::to_string() {
    string l = iterator->getSerialCLoop(sub,true);
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        l += (*stmts)[i]->to_string() + "\n";
    }
    l += "}";
    
    return l;
}

string pthread_function_t::to_string() {
    string s = "void *" + name + "_body_" + boost::lexical_cast<string>(uid);
    s += "(void *mesg) {\n";
    
    set<string> itrs;
    get_iterators(itrs);
    set<string>::iterator it;
    s += "int disp, __zr__";
    for (it = itrs.begin(); it != itrs.end(); ++it)
        s += "," + *it;
    s += ";\n";
    
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        s += (*stmts)[i]->to_string() + "\n";
    }
    
    s += "return NULL;\n";
    s += "}\n";
    
    return s;
}

string pthread_function_call_t::to_string() {
    
    string s = "pthread_create(&threads_";
    s += boost::lexical_cast<string>(uid) + "[disp], NULL, ";
    s +=  name + "_body_" + boost::lexical_cast<string>(uid);
    s += ", (void *)(" + name + "_" + boost::lexical_cast<string>(uid);
    s += "+disp));\n";
    s += "++disp;";
    return s;
}

string pthread_message_t::to_string() {
    string s = "typedef struct { \n";
    
    for (unsigned int i = 0; i < stmts->size(); ++i) {
        s += (*stmts)[i]->to_string() + "\n";
    }
    
    s += "} " + name + "_" + boost::lexical_cast<string>(uid);
    s += "_msg_t;\n";
    return s;
}

string pthread_message_pack_t::to_string() {
    string s;
    
    string msg = name + "_" + boost::lexical_cast<string>(uid) + "[disp].";
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        if ((*i)->code == _declare) {
            string n = (*i)->stmt.declare->name;
            s += msg + n + " = " + n + ";\n";
        }
    }
    
    return s;
}

string pthread_message_unpack_t::to_string() {
    string s;
    
    string t = name + "_" + boost::lexical_cast<string>(uid) + "_msg_t";
    
    s += t + " *msg = (" + t + "*)mesg;\n";
    
    statement_list_t::iterator i;
    for (i = stmts->begin(); i < stmts->end(); ++i) {
        if ((*i)->code == _declare) {
            string t = (*i)->to_string();
            
            size_t loc = t.find(";");
            t.replace(loc,1,"");
            
            t += " = msg->" + (*i)->stmt.declare->name + ";\n";
            
            s += t;
        }
    }
    
    return s;
}

string pthread_message_alloc_t::to_string() {

    string s = "";
    string sid = boost::lexical_cast<string>(uid);
    string t = name + "_" + sid + "_msg_t";
    
    s += t + " *" + name + "_" + sid + " = malloc(sizeof(" + t; 
    s += ")*" + size + ");";
    
    return s;
}

string pthread_alloc_t::to_string() {
    string s = "";
    s += "pthread_t *threads_" + boost::lexical_cast<string>(uid);
    s += " = malloc(sizeof(pthread_t)*" + size + ");";
    
    return s;
}

string pthread_join_t::to_string() {
    string s = "for (disp = 0; disp < " + size + "; ++disp) {\n";
    s += "pthread_join(threads_" + boost::lexical_cast<string>(uid);
    s += "[disp],NULL);\n";
    s += "}\n";
    
    return s;
}

string container_reduction_t::to_string() {
    
    while (fromIndex.find("$$") != string::npos) {
        fromIndex.replace(fromIndex.find("$$"),2,"__s");
    }
    while (toIndex.find("$$") != string::npos) {
        toIndex.replace(toIndex.find("$$"),2,"__m");
    }
    
    string lToIndex;
    if (isScalar) {
        // scalar
        if (element)
            lToIndex = "[0]";
        else
            lToIndex = "";
    }
    else {
        lToIndex = "[" + toIndex + "]";
    }
    
    string r = "";
    string e = "";
    unsigned int i;
    for (i = 0; i < rLoops.size(); ++i) {
        if (rLoops[i].compare("ZERO") == 0)
            break;
        r += rLoops[i];
        e += "}\n";
    }
    ++i;
    
    string c;
    string cE = "";
    if (reduceToSelf) {
        c = "if (!(" + zeroCheck + ")) {\n";
        for (unsigned int j = i; j < rLoops.size(); ++j) {
            c += rLoops[j];
            cE += "}\n";
        }
        c += toName + lToIndex + " += " + fromName + "[" + fromIndex;
        c += "];\n";
        c += cE;
        c += "}\n";
    }
    else {
        c = "if (" + zeroCheck + ") {\n";
        for (unsigned int j = i; j < rLoops.size(); ++j) {
            c += rLoops[j];
            cE += "}\n";
        }
        c += toName + lToIndex + " = " + fromName + "[" + fromIndex;
        c += "];\n";
        c += cE;
        c += "} else {\n";
        for (unsigned int j = i; j < rLoops.size(); ++j) {
            c += rLoops[j];
        }
        c += toName + lToIndex + " += " + fromName + "[" + fromIndex;
        c += "];\n";
        c += cE;
        c += "}\n";
    }
    
    return "{\n" + decls + r + c + e + "}\n";
}

string declare_partition_t::to_string() {
    string s = "";
    string pSize = boost::lexical_cast<string>(partSize);
    
    string lSize = size;
    if (lSize.find("$$") == 0)
        lSize.replace(0,2,"__s");
    
    if (fixedSize) {
        s += "int " + name + " = " + pSize + ";\n";
    }
    else {
        string npName = "nparts" + name;
        s += "int " + npName + " = " + pSize + ";\n";
        s += "int " + name + ";\n";
        s += "if (" + npName + " > 1 && " + lSize + " > " + npName + ") {\n";
        s += name + " = " + lSize + "/" + npName + ";\n";
        s += "// this will fail if " + lSize + "%" + npName + "+" + 
                name + " > "  + npName + "\n";
        s += "// which primarily effects small sizes\n";
        s += "if (" + lSize + "%" + npName + ")\n";
        s += "++" + name + ";\n";
        s += "}\n";
        
        s += "else {\n";
        s += name + " = " + lSize + ";\n";
        s += npName + " = 1;\n";
        s += "}\n";
    }
    return s;
}



