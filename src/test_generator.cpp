#include "syntax.hpp"
#include "translate_to_code.hpp"
#include "timer.h"
#include <unistd.h>
#include <stdio.h>
#include <list>
#include "modelInfo.hpp"
#include <math.h>
#include <string.h>

using std::list;

extern std::string precision_type; 					 
						 
void generateData(std::ostream &out, std::string data, std::string s) {
	// create code to fill data of size s
	out << "for (i=0; i < " << s << "; ++i)\n";
	out << data << "[i] = val();\n";
}

void graphToBLAS(graph &pg, std::string routine_name, string functionSig,
				 std::ofstream &out, std::map<vertex, string> &error) {
	
	string suff = "cblas_d";
	string freeList;
	
	map<vertex, string> noOutError;
	
	out << "void blas_" << routine_name << functionSig << " {\n";
	out << "int i;\n";
	vertex i;
	
	/*
	 handle the case that an operation has more than one out edge
	 and one of the out edges points to an output.  Make the only
	 out edge point to the output, and make the output have out
	 edges to the rest
	    +          +
	   / \     ->  |
	 out  t4		out
					 |
					t4
	*/
	
	graph g = graph(pg);
	
	for (i = 0; i < g.num_vertices(); i++) {
		
		if (!(g.info(i).op & OP_ANY_ARITHMATIC)) 
			continue;
		
		vertex outVert;
		bool foundOut = false;
		if (g.adj(i).size() > 1) {
			for (size_t j = 0; j < g.adj(i).size(); ++j) {
				if (g.info(g.adj(i)[j]).op == output) {
					outVert = g.adj(i)[j];
					foundOut = true;
					break;
				}
			}
		}
		
		if (foundOut) {
            g.remove_edges(i,outVert);
            g.move_out_edges_to_new_vertex(i, outVert);
            g.add_edge(i,outVert);
		}
	}
	
#if 0
	std::ofstream outt("lower10.dot");
    print_graph(outt, g);
#endif
	
	// handle scaler outputs
    vector<string> outPuts;
    // scalar inouts can get named incorrectly if just looking at
    // the input portion.  create a list of the scalar outputs
    // to catch this
	for (i = 0; i < g.num_vertices(); i++) {
		if (g.info(i).op == output && g.info(i).t.k == scalar) {
			out << precision_type << " __l" << g.info(i).label << " = *"
				<< g.info(i).label << "_ptr;\n";
            outPuts.push_back(g.info(i).label);
        }
	}
	
	// generate the blas calls with the modified graph.
	for (i = 0; i < g.num_vertices(); i++) {
		
		if (g.info(i).op & OP_2OP_ARITHMATIC) {
		
            // c in a little bit tricky. if the output of i
            // is into another operation, then we actually
            // want the type information from i.  if however
            // the output of i is into a temporary or an output, 
            // we want that because the types should be the same,
            // but we wants the data structures name.
            // I think because we flatten multiple outedges with outputs
            // above, just checking the first out edge will work.
            vertex c = g.adj(i)[0];
            if (g.info(c).op == add || g.info(c).op == subtract
                || g.info(c).op == multiply || g.info(c).op == trans) {
                c = i;
            }
            vertex a = g.inv_adj(i)[0];
            vertex b = g.inv_adj(i)[1];
            
            bool transA = false;
            bool transB = false;
            vertex ta = a;
            vertex tb = b;
            
            if (g.info(a).op == trans) {
                ta = a;
                a = g.inv_adj(ta)[0];
                transA = true;
            }
            if (g.info(b).op == trans) {
                tb = b;
                b = g.inv_adj(tb)[0];
                transB = true;
            }
            
            string nc = g.info(c).label;
            string na = g.info(a).label;
            string nb = g.info(b).label;
            
            if (g.info(c).t.k == scalar && (g.info(c).op == output ||
                find(outPuts.begin(),outPuts.end(),nc) != outPuts.end())) {
                nc = "__l"+nc;
            }
            else if (nc.compare("") == 0 || nc.compare(0,3,"tmp") == 0) {
                nc = "t" + boost::lexical_cast<string>(i);
                if (g.info(i).t.k == scalar) {
                    out << precision_type << " " << nc << ";\n";
                }
                else {
                    out << precision_type << " *" << nc << "=("
                    << precision_type << "*)malloc(sizeof("
                    << precision_type << ")*" 
                    << container_size_type(g.info(i).t)
                    << ");\n";
                    freeList += "free(" + nc + ");\n";
                }
            }
            
            if (g.info(a).t.k == scalar && (g.info(a).op == output ||
                find(outPuts.begin(),outPuts.end(),na) != outPuts.end())) {
                na = "__l"+na;
            }
            else if (na.compare("") == 0 || na.compare(0,3,"tmp") == 0) {
                na = "t" + boost::lexical_cast<string>(a);
            }
            
            if (g.info(b).t.k == scalar && (g.info(b).op == output ||
                                            find(outPuts.begin(),outPuts.end(),nb) != outPuts.end())) {
                nb = "__l"+nb;
            }
            else if (nb.compare("") == 0 || nb.compare(0,3,"tmp") == 0) {
                nb = "t" + boost::lexical_cast<string>(b);
            }
            
            if (g.info(i).op == add) {
                
                out << "// " << nc << " = " << na
				<< " + " << nb << "\n";
                
                
                switch (g.info(c).t.height) {
                    case 0:
                        out << nc << " = " << na
                        << " + " << nb << ";\n";
                        break;
                    case 1: 
                    {
                        string s = g.info(c).t.dim.dim;
                        
                        // if na || nb == nc special case
                        if (nc.compare(na) == 0) {
                            // x = x + y or x = x + x
                            out << suff << "axpy(" << s << ",1.0," << nb 
                                << ",1," << nc << ",1);\n"; 
                        }
                        else if (nc.compare(nb) == 0) {
                            // x = y + x 
                            out << suff << "axpy(" << s << ",1.0," << na 
                                << ",1," << nc << ",1);\n";
                        }
                        else {
                            // x = y + z
                            out << suff << "copy(" << s << "," << nb
                            << ",1," << nc << ",1);\n"; 
                            out << suff << "axpy(" << s << ",1.0," << na 
                                << ",1," << nc << ",1);\n";
                        }
                        
                        break;
                    }
                    case 2:
                    {
                        string s = g.info(c).t.t->dim.dim;
                        string aOff = na + "+i*" + s;
                        string bOff = nb + "+i*" + s;
                        string cOff = nc + "+i*" + s;
                        
                        // if na || nb == nc special case
                        if (nc.compare(na) == 0) {
                            // x = x + y or x = x + x
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "axpy(" << s << ",1.0," << bOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        else if (nc.compare(nb) == 0) {
                            // x = y + x
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "axpy(" << s << ",1.0," << aOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        else {
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "copy(" << s << "," << bOff
                            << ",1," << cOff << ",1);\n";
                            out << suff << "axpy(" << s << ",1.0," << aOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        
                        break;
                    }
                    default:
                        std::cout << "test_generator.cpp; graphToBLAS(); addition - types not handled\n";
                }
                
                if (g.info(c).op == output)
                    error[c] = "1";
                noOutError[i] = "1";
            }
            
            else if (g.info(i).op == subtract) {
                
                out << "// " << nc << " = " << na
                << " - " << nb << "\n";
                
                
                switch (g.info(c).t.height) {
                    case 0:
                        out << "// " << nc << " = " << na
                        << " - " << nb << ";\n";
                        break;
                    case 1: 
                    {
                        string s = g.info(c).t.dim.dim;
                        
                        // if na || nb == nc special case
                        if (nc.compare(na) == 0) {
                            // x = x - y or x = x - x
                            out << suff << "axpy(" << s << ",-1.0," << nb 
                            << ",1," << nc << ",1);\n";
                        }
                        else if (nc.compare(nb) == 0) {
                            // x = y - x 
                            out << suff << "scal(" << s << ",-1.0," << nc
                            << "1);\n";
                            // x = y + (-x)
                            out << suff << "axpy(" << s << ",1.0," << na 
                            << ",1," << nc << ",1);\n";
                        }
                        else {
                            // x = y + z
                            out << suff << "copy(" << s << "," << na
                            << ",1," << nc << ",1);\n"; 
                            out << suff << "axpy(" << s << ",-1.0," << nb 
                            << ",1," << nc << ",1);\n";
                        }
                        
                        break;
                    }
                    case 2:
                    {
                        string s = g.info(c).t.t->dim.dim;
                        string aOff = na + "+i*" + s;
                        string bOff = nb + "+i*" + s;
                        string cOff = nc + "+i*" + s;
                        
                        // if na || nb == nc special case
                        if (nc.compare(na) == 0) {
                            // x = x - y or x = x - x
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "axpy(" << s << ",-1.0," << bOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        else if (nc.compare(nb) == 0) {
                            // x = y - x
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "scal(" << s << ",-1.0," << cOff
                            << "1);\n";
                            out << suff << "axpy(" << s << ",-1.0," << aOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        else {
                            out << "for (i = 0; i < " << g.info(c).t.dim.dim 
                            << "; ++i) {\n";
                            out << suff << "copy(" << s << "," << aOff
                            << ",1," << cOff << ",1);\n";
                            out << suff << "axpy(" << s << ",-1.0," << bOff 
                            << ",1," << cOff << ",1);\n";
                            out << "}\n";
                        }
                        
                        break;
                    }
                    default:
                        std::cout << "test_generator.cpp; graphToBLAS(); subtraction - types not handled\n";
                }
                
                if (g.info(c).op == output)
                    error[c] = "1";
                noOutError[i] = "1";
            }
            
            else if (g.info(i).op == multiply) {
                
                out << "// " << nc << " = " << na
				<< " * " << nb << "\n";
                
                int ha = g.info(a).t.height;
                int hb = g.info(b).t.height;
                
                if (ha == 0 && hb == 0) {
                    // scalar multiply
                    out << nc << " = " << na
                    << " * " << nb << ";\n";
                    
                    if (g.info(c).op == output)
                        error[c] = "1";
                    noOutError[i] = "1";
                }
                else if ((ha == 0 && hb == 1) || (ha == 1 && hb == 0)) {
                    // scale vector
                    string scl;
                    string vec;
                    if (g.info(a).t.k == scalar) {
                        scl = na;
                        vec = nb;
                    }
                    else {
                        scl = nb;
                        vec = na;
                    }
                    string s = g.info(i).t.dim.dim;
                    
                    // special case x = x * alpha
                    if (nc.compare(vec) == 0) {
                        // x = x * alpha
                        out << suff << "scal(" << s << ","
                        << scl << "," << nc << ",1);\n";
                    }
                    else {
                        // x = y * alpha
                        out << suff << "copy(" << s << "," << vec
                        << ",1," << nc << ",1);\n"; 
                        out << suff << "scal(" << s << ","
                        << scl << "," << nc << ",1);\n";
                    }
                    
                    if (g.info(c).op == output)
                        error[c] = "1";
                    noOutError[i] = "1";
					
                }
                else if ((ha == 0 && hb == 2) || (ha == 2 && hb == 0)) {
                    // scale matrix
                    out << "test_generator.cpp; graphToBLAS(); finish scale matrix\n";
                    if (g.info(c).op == output)
                        error[c] = "1";
                    noOutError[i] = "1";
                }
                else if ((ha == 1 && hb == 1)) {
                    if (g.info(ta).t.k == row && g.info(tb).t.k == column) {
                        // dot
                        string s = g.info(a).t.dim.dim;
                        
                        out << nc << " = " << suff << "dot(" << s << "," 
						<< na << ",1," << nb << ",1);\n";
                        
                        if (g.info(c).op == output)
                            error[c] = s + "-1+" + s ;
                        noOutError[i] = s + "-1+" + s ;
                    }
                    else {
                        // outer product
                        string order;
                        string m, n;
                        string ldc;
                        
                        if (g.info(c).t.get_lowest_ns()->k == row) {
                            // row major
                            order = "CblasRowMajor";	
                            m = g.info(c).t.dim.dim;
                            n = g.info(c).t.t->dim.dim;
                            ldc = n;
                        }
                        else {
                            // column major
                            order = "CblasColMajor";
                            n = g.info(c).t.dim.dim;
                            m = g.info(c).t.t->dim.dim;
                            ldc = m;
                        }
                        
                        out << "memset(" << nc << ",0,sizeof(" << precision_type 
						<< ")*" << m << "*" << n << ");\n";
                        out << suff << "ger(" << order << "," << m << "," 
						<< n << ",1.0," << na << ",1," << nb << ",1," << nc 
                            << "," << ldc << ");\n";
                        
                        if (g.info(c).op == output)
                            error[c] = "1";
                        noOutError[i] = "1";
                    }
                }
                else if ((ha == 1 && hb == 2) || (ha == 2 && hb == 1)) {
                    // gemv
                    // trmv
                    
                    bool ge = false, tr = false;
                    
                    string uplo;
                    string diag = "CblasNonUnit";
                    
                    string m,n,lda;
                    string order;
                    string trans;
                    
                    if (ha == 1) {
                        // b is matrix
                        
                        if (g.info(b).t.s == general)
                            ge = true;
                        else if (g.info(b).t.s == triangular) {
                            tr = true;
                            if (g.info(b).t.uplo == upper) {
                                uplo = "CblasUpper";
                            }
                            else {
                                uplo = "CblasLower";
                            }
                        }
                        
                        if (transA) 
                            trans = transB ? "CblasNoTrans" : "CblasTrans";					
                        else 
                            trans = transB ? "CblasTrans" : "CblasNoTrans";
                        
                        if (g.info(b).t.get_lowest_ns()->k == row) {
                            // row major
                            order = "CblasRowMajor";	
                            m = g.info(b).t.dim.dim;
                            n = g.info(b).t.t->dim.dim;
                            lda = n;
                        }
                        else {
                            // column major
                            order = "CblasColMajor";
                            n = g.info(b).t.dim.dim;
                            m = g.info(b).t.t->dim.dim;
                            lda = m;
                        }
                        if (ge) {
                            // this will break give x = A*x
                            out << suff << "gemv(" << order << "," << trans
                            << ","<< m << "," << n << ",1.0," << nb << "," 
                            << lda << "," << na << ",1,0.0," << nc 
                            << ",1);\n";
                        }
                        else if (tr) {
                            // trmv is x = A*x, so a copy is needed when we have
                            // y = A*x
                            if (nc.compare(na) == 0) {
                                // x = A*x
                                // call trmv
                                out << suff << "trmv(" << order << ","  << uplo 
                                << "," << trans << "," << diag << "," << n 
                                << "," << nb << "," << lda << "," << nc 
                                << ",1" << ");\n";
                            }
                            else {
                                // y = A*x
                                // copy vector a to vector c before start.
                                out << suff << "copy(" << n << "," << na << ",1,"
                                << nc << ",1);\n";
                                // call trmv
                                out << suff << "trmv(" << order << ","  << uplo 
                                << "," << trans << "," << diag << "," << n 
                                << "," << nb << "," << lda << "," << nc 
                                << ",1" << ");\n";
                            }
                        }
                        else {
                            std::cout << "test_generator.cpp; graphToBLAS(); multiply - unexpected format\n";
                        }
                    }
                    else {
                        // a is matrix
                        
                        if (g.info(a).t.s == general)
                            ge = true;
                        else if (g.info(a).t.s == triangular) {
                            tr = true;
                            if (g.info(a).t.uplo == upper) {
                                uplo = "CblasUpper";
                            }
                            else {
                                uplo = "CblasLower";
                            }
                        }
                        
                        if (transB) 
                            trans = transA ? "CblasNoTrans" : "CblasTrans";				
                        else 
                            trans = transA ? "CblasTrans" : "CblasNoTrans";
                        
                        
                        if (g.info(a).t.get_lowest_ns()->k == row) {
                            // row major
                            order = "CblasRowMajor";	
                            m = g.info(a).t.dim.dim;
                            n = g.info(a).t.t->dim.dim;
                            lda = n;
                        }
                        else {
                            // column major
                            order = "CblasColMajor";
                            n = g.info(a).t.dim.dim;
                            m = g.info(a).t.t->dim.dim;
                            lda = m;
                        }
                        
                        if (ge) {
                            // this will break given x = A*x
                            out << suff << "gemv(" << order << "," << trans 
                            << "," << m << "," << n << ",1.0," << na << "," 
                            << lda << "," << nb << ",1,0.0," << nc 
                            << ",1);\n";
                        }
                        else if (tr) {
                            // trmv is x = A*x, so a copy is needed when we have
                            // y = A*x
                            if (nc.compare(nb) == 0) {
                                // x = A*x
                                // call trmv
                                out << suff << "trmv(" << order << ","  << uplo 
                                << "," << trans << "," << diag << "," 
                                << n << "," << na << "," << lda << "," 
                                << nc << ",1" << ");\n";
                            }
                            else {
                                // y = A*x
                                // copy vector b to vector c before start.
                                out << suff << "copy(" << n << "," << nb << ",1,"
                                << nc << ",1);\n";
                                // call trmv
                                out << suff << "trmv(" << order << ","  << uplo 
                                << "," << trans << "," << diag << "," 
                                << n << "," << na << "," << lda << "," 
                                << nc << ",1" << ");\n";
                            }
                        }
                        else {
                            std::cout << "test_generator.cpp; graphToBLAS(); multiply - unexpected format\n";
                        }
                    }
                    
                    if (g.info(c).op == output) {
                        if (trans.compare("CblasNoTrans") == 0)
                            error[c] = n + "-1+" + n;
                        else
                            error[c] = m + "-1+" + m;
                    }
                    if (trans.compare("CblasNoTrans") == 0)
                        noOutError[i] = n + "-1+" + n;
                    else
                        noOutError[i] = m + "-1+" + m;
                }
                else if ((ha == 2 && hb == 2) || (ha == 2 && hb == 2)) {
                    // gemm
                    string m,n,k,lda,ldb,ldc;
                    string order;
                    string tA;
                    string tB;
                    
                    if (g.info(c).t.get_lowest_ns()->k == column) {
                        // column major
                        order = "CblasColMajor";
                        n = g.info(c).t.dim.dim;
                        m = g.info(c).t.t->dim.dim;
                        ldc = m;
                        
                        if (g.info(a).t.get_lowest_ns()->k == column) {
                            // a notrans
                            if (transA) {
                                k = g.info(a).t.t->dim.dim;
                                tA = "CblasTrans";
                                lda = k;
                            }
                            else  {
                                k = g.info(a).t.dim.dim;
                                tA = "CblasNoTrans";
                                lda = m;
                            }
                        }
                        else {
                            if (transA) {
                                k = g.info(a).t.dim.dim;
                                tA = "CblasNoTrans";
                                lda = m;
                            }
                            else  {
                                k = g.info(a).t.t->dim.dim;
                                tA = "CblasTrans";
                                lda = k;
                            }
                        }
                        
                        if (g.info(b).t.get_lowest_ns()->k == column) {
                            if (transB) {
                                tB = "CblasTrans";
                                ldb = n;
                            }
                            else {
                                tB = "CblasNoTrans";
                                ldb = k;
                            }
                        }
                        else {
                            if (transB) {
                                tB = "CblasNoTrans";
                                ldb = k;
                            }
                            else {
                                tB =  "CblasTrans";
                                ldb = n;
                            }
                        }
                    }
                    else {
                        // row major
                        order = "CblasRowMajor";
                        n = g.info(c).t.t->dim.dim;
                        m = g.info(c).t.dim.dim;
                        ldc = n;
                        
                        if (g.info(a).t.get_lowest_ns()->k == row) {
                            // a notrans
                            if (transA) {
                                k = g.info(a).t.dim.dim;
                                tA = "CblasTrans";
                                lda = m;
                            }
                            else  {
                                k = g.info(a).t.t->dim.dim;
                                tA = "CblasNoTrans";
                                lda = k;
                            }
                        }
                        else {
                            if (transA) {
                                k = g.info(a).t.t->dim.dim;
                                tA = "CblasNoTrans";
                                lda = k;
                            }
                            else  {
                                k = g.info(a).t.dim.dim;
                                tA = "CblasTrans";
                                lda = m;
                            }
                        }
                        
                        if (g.info(b).t.get_lowest_ns()->k == row) {
                            if (transB) {
                                tB = "CblasTrans";
                                ldb = k;
                            }
                            else {
                                tB = "CblasNoTrans";
                                ldb = n;
                            }
                        }
                        else {
                            if (transB) {
                                tB = "CblasNoTrans";
                                ldb = n;
                            }
                            else {
                                tB =  "CblasTrans";
                                ldb = k;
                            }
                        }
                    }
                    // this will break if (na || nb) == nc
                    out << suff << "gemm(" << order << "," << tA 
					<< "," << tB << ","
					<< m << "," << n << "," << k << ",1.0," << na << ","
					<< lda << "," << nb << "," << ldb << ",0.0," << nc
					<< "," << ldc << ");\n";
                    
                    if (g.info(c).op == output) 
                        error[c] = k + "-1+" + k;
                    noOutError[i] = k + "-1+" + k;
                }
                else {
                    std::cout << "test_generator.cpp; graphToBLAS(); multiply - unhandled type\n";
                }
            }	
            
            else if (g.info(i).op == divide) {
                out << "// " << nc << " = " << na
                << " / " << nb << "\n";
                
                
                switch (g.info(c).t.height) {
                    case 0:
                        out << nc << " = " << na << "/" << nb << ";\n";
                        break;
                    case 1: 
                    case 2:
                        out << "test_generator.cpp; graphToBLAS(); finish container divide\n";
                        break;
                    default:
                        std::cout << "test_generator.cpp; graphToBLAS(); divide - types not handled\n";
                }
                
                if (g.info(c).op == output)
                    error[c] = "1";
                noOutError[i] = "1";
            }
            
            // propogate error
            string errA, errB;
            if (noOutError.find(a) != noOutError.end())
                errA = noOutError[a];
            else if (error.find(a) != error.end()) 
                errA = error[a];
            else
                errA = "";
            
            if (noOutError.find(b) != noOutError.end())
                errB = noOutError[b];
            else if (error.find(b) != error.end()) 
                errB = error[b];
            else
                errB = "";
            
            string propErr = "";
            if (errA.compare("") == 0) {
                if (errB.compare("") != 0)
                    propErr = " +("+errB + ")";
            }
            else {
                if (errB.compare("") == 0)
                    propErr = " +("+errA + ")";
                else {
                    errA = "("+errA+")";
                    errB = "("+errB+")";
                    propErr = "+((" + errA + ">" +errB + ")?" + errA +
					":" + errB + ")";
                }
            }
            if (error.find(c) != error.end()) { 
                error[c] += propErr;
                //std::cout << c << "\t" << error[c] << "\n";
            }
            if (noOutError.find(i) != noOutError.end()) {
                noOutError[i] += propErr;
                //std::cout << i << "\t" << noOutError[i] << "\n";
            }
        }
        else if (g.info(i).op & OP_1OP_ARITHMATIC) {
            // c in a little bit tricky. if the output of i
            // is into another operation, then we actually
            // want the type information from i.  if however
            // the output of i is into a temporary or an output, 
            // we want that because the types should be the same,
            // but we wants the data structures name.
            // I think because we flatten multiple outedges with outputs
            // above, just checking the first out edge will work.
            vertex c = g.adj(i)[0];
            if (g.info(c).op == add || g.info(c).op == subtract
                || g.info(c).op == multiply || g.info(c).op == trans) {
                c = i;
            }
            vertex a = g.inv_adj(i)[0];
            
            vertex ta = a;
            
            if (g.info(a).op == trans) {
                ta = a;
                a = g.inv_adj(ta)[0];
            }
            
            string nc = g.info(c).label;
            string na = g.info(a).label;
            
            if (g.info(c).t.k == scalar && (g.info(c).op == output ||
                find(outPuts.begin(),outPuts.end(),nc) != outPuts.end())) {
                nc = "__l"+nc;
            }
            else if (nc.compare("") == 0 || nc.compare(0,3,"tmp") == 0) {
                nc = "t" + boost::lexical_cast<string>(i);
                if (g.info(i).t.k == scalar) {
                    out << precision_type << " " << nc << ";\n";
                }
                else {
                    out << precision_type << " *" << nc << "=("
                    << precision_type << "*)malloc(sizeof("
                    << precision_type << ")*" 
                    << container_size_type(g.info(i).t)
                    << ");\n";
                    freeList += "free(" + nc + ");\n";
                }
            }
            
            if (g.info(a).t.k == scalar && (g.info(a).op == output ||
                find(outPuts.begin(),outPuts.end(),na) != outPuts.end())) {
                na = "__l"+na;
            }
            else if (na.compare("") == 0 || na.compare(0,3,"tmp") == 0) {
                na = "t" + boost::lexical_cast<string>(a);
            }

            if (g.info(i).op == squareroot) {
                
                out << "// " << nc << " = sqrt(" << na
                    << ")\n";
                
                
                switch (g.info(c).t.height) {
                    case 0:
                        out << nc << " = sqrt(" << na
                            << ");\n";
                        break;
                    case 1: 
                    case 2:
                    default:
                        std::cout << "test_generator.cpp; graphToBLAS(); squareroot - types not handled\n";
                }
                
                if (g.info(c).op == output)
                    error[c] = "1";
                noOutError[i] = "1";
            }
            
            // propogate error
            string errA;
            if (noOutError.find(a) != noOutError.end())
                errA = noOutError[a];
            else if (error.find(a) != error.end()) 
                errA = error[a];
            else
                errA = "";
            
            string propErr = "";
            if (errA.compare("") != 0) {
                propErr = " +("+errA + ")";
            }
            if (error.find(c) != error.end()) { 
                error[c] += propErr;
                //std::cout << c << "\t" << error[c] << "\n";
            }
            if (noOutError.find(i) != noOutError.end()) {
                noOutError[i] += propErr;
                //std::cout << i << "\t" << noOutError[i] << "\n";
            }
        }
	}
	
	// handle scaler outputs
	for (i = 0; i < g.num_vertices(); i++) {
		if (g.info(i).op == output && g.info(i).t.k == scalar)
			out << "*" << g.info(i).label << "_ptr = __l"
				<< g.info(i).label << ";\n";
	}
	
	out << freeList;
	out << "}\n\n\n\n";
}

string setUpOutPuts(string suff, std::map<vertex,string> &outputs, graph &g) {
	string cpy = "";
	
	std::map<vertex, string>::iterator it = outputs.begin();
	for (; it != outputs.end(); ++it) {
		string n = g.info(it->first).label;
		if (g.info(it->first).t.k == scalar) {
			cpy += suff + n + " = orig" + n + ";\n";
		}
		else {
			cpy += "memcpy(" + suff + n + ",orig" + n + ",sizeof(" 
				+ precision_type + ")*"  
				+ container_size_type(g.info(it->first).t) + ");\n";
		}
	}
	
	return cpy;
}

void checkOutput(std::ostream &out, std::map<vertex,string> &error, graph &g) {
	std::map<vertex, string>::iterator it = error.begin();
	
	for (; it != error.end(); ++it) {
		string n = g.info(it->first).label;
		type &t = g.info(it->first).t;

		switch (g.info(it->first).t.height) {
			case 0:
			{
				out << "if (checkError(" << n << ",ref" << n << ","
					<< it->second << "))";
				break;
			}
			case 1: 
			{
				out << "if (checkVector(" << n << ",ref" << n << ","
					<< t.dim.dim << ",1," << it->second << "))";
				break;
			}
			case 2: {
				string rows, cols, ld, order;
				
				if ((t.get_lowest_ns())->k == row) {
					order = "CblasRowMajor";
					rows = t.dim.dim;
					cols = t.t->dim.dim;
					ld = cols;
				}
				else {
					order = "CblasColMajor";
					rows = t.t->dim.dim;
					cols = t.dim.dim;
					ld = rows;
				}
				
				out << "if (checkMatrix(" << n << ",ref" << n << "," << rows
					<< "," << cols << "," << ld << "," << order << "," 
					<< it->second << "))";
				break;
			}
			default:
				std::cout << "WARNING: test_generator.cpp; checkOutput(); unhandled case\n"; 
		}
		out << "{\n";
		out << "++failedCnt;\n";
		out << "#ifdef END_EARLY\n";
		out << "goto END;\n";
		out << "#endif\n";
		out << "continue;\n";
		out << "}\n";
	}
}

void createCorrectnessTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 modelMsg &msg, bool noPtr) {

	//// Usefull data points
	int dSize = msg.stop;	// order of vectors and matrices created for test
	int N = dSize;			// largest size tested
	int start = msg.start;	// first size tested
	int step = msg.step;	// testing step size
	
	
	// map strings to types for all inputs and outputs
	std::map<string, std::pair<vertex,type> > data; 
	for (size_t i = 0; i != g.num_vertices(); i++) {
		switch (g.info(i).op) {
			case input:
			case output:
				data[g.info(i).label] = std::make_pair(i, g.info(i).t);
				break;
				
			default:
				break;
		}
	}	
	
	// find sizes that are actually used
	std::set<string> usedSizes;
	std::map<string, std::pair<vertex,type> >::iterator im = data.begin();
	for (; im != data.end(); ++im) {
		type &t = im->second.second;
		usedSizes.insert(t.dim.dim);
		type *tt = t.t;
		while (tt) {
			if (tt->k == scalar)
				break;
			if (tt->k != t.k) {
				usedSizes.insert(tt->dim.dim);
				break;
			}
			tt = tt->t;
		}
	}
	
	// create test file
	std::ofstream out((out_file_name + "CTester.c").c_str());
	
	out << "#include <stdlib.h>\n";
	out << "#include <stdio.h>\n";
	out << "#include \"testUtils.h\"\n";
	out << "\n";
	
	// function signature
	// while creating this signature, also create the call string
	// function signature
    // while creating this signature, also create the call string
    string callStringBTO = routine_name + 
			function_args(inputs,outputs,data,"",nameOnly,noPtr) + ";\n";
    string callStringBLAS = "blas_" + 
			routine_name + function_args(inputs,outputs,data,"ref",nameOnly, 
										 false) + ";\n";
    string functionSig = function_args(inputs,outputs,data,"",typeWithName,noPtr); 
    string blasSig = function_args(inputs,outputs,data,"",typeWithName,false);

	out << "extern void " << routine_name << functionSig << ";\n\n";
	
	// BLAS equivalent
	// error holds the tollerated error for a given vertex
	std::map<vertex, string> error;
	graphToBLAS(g, routine_name, functionSig, out, error);
	
	// begin main
	out << "int main (int argc, char *argv[]) {\n";
	out << "int i;\n";
	out << "int N = " << N << ";\n";
	out << "int start = " << start << ";\n";
	out << "int step = " << step << ";\n";
	out << "\n\n\n";
	out << "int cnt = 0;\n";
	out << "int failedCnt = 0;\n";
	
	// initialize variables
	std::set<string>::iterator is = usedSizes.begin();
	for (; is != usedSizes.end(); ++is) {
		if (is->compare("1") == 0)
			continue;
		out << "int " << *is << " = " << dSize << ";\n";
	}
	
	// create required data structures for inputs and outputs
	// fill with data	
	im = data.begin();
    vector<string> toFree;
	for (; im != data.end(); ++im) {
		if (im->second.second.k == scalar) {
			out << precision_type << " " << im->first << " = 2.2;\n";
			if (error.find(im->second.first) != error.end()) {
				out << precision_type << " ref" << im->first << " = 2.2;\n";
				out << precision_type << " orig" << im->first << " = 2.2;\n";
			}
		}
		else {
			string n = im->first;
			string s = container_size_type(im->second.second);
			string genName = n;

			if (error.find(im->second.first) != error.end()) {
				out << precision_type << "* " << n << " = (" << precision_type 
				<< "*) malloc(sizeof(" << precision_type << ")*3*"
				<< s << ");\n";
				
				out << precision_type << "* ref" << n << " = " + n << "+"
					<< s << ";\n";
				out << precision_type << "* orig" << n << " = " + n << "+"
					<< s << "*2;\n";
				genName = "orig" + genName;
                toFree.push_back("free(" + n + ");\n");
			}
			else {
				out << precision_type << "* " << n << " = (" << precision_type 
				<< "*) malloc(sizeof(" << precision_type << ")*"
				<< s << ");\n";
                toFree.push_back("free(" + n + ");\n");
			}
			
			generateData(out, genName, s);
		}
		
	}
	
	out << "\n\n";
	
	// call  routines
	is = usedSizes.begin();
	
	for (; is != usedSizes.end(); ++is) {
		if (is->compare("1") == 0)
			continue;
		out << "for (" << *is << "=start; " << *is << " <= N; " << *is << "+=step) {\n";
	}
		
	out << "++cnt;\n";
	
	// call bto
	out << setUpOutPuts("", error, g); 
	out << callStringBTO;

	// call blas
	out << setUpOutPuts("ref", error, g);
	out << callStringBLAS;
	
	checkOutput(out, error, g);
	
	for (is = usedSizes.begin(); is != usedSizes.end(); ++is) {
		if (is->compare("1") == 0)
			continue;
		out << "}\n";
	}
	
	out << "END:\n";
	out << "reportStatus(cnt, failedCnt, NULL);\n";
	
	// close main()
    for (size_t i = 0; i < toFree.size(); ++i) 
        out << toFree[i];
	out << "return 0;\n";
	out << "}\n";
	out.close();
}

void runCorrectnessTest(std::string &path, std::string &fileName, 
					 std::list<versionData*> &versionList) {

	//change to working directory
	char cwd[1024];
	char *p = getcwd(cwd, 1024);
	if (p == NULL)
		std::cout << "Failed to get current working directory\n";
	chdir(path.c_str());
	
	
	// run all test present
	string prec;	
	if (precision_type.compare("double") == 0)
		prec = "PREC=DREAL";
	else
		prec = "PREC=SREAL";
	
	const char mode[2] = {'r','\0'};
	char buffer[100];
	
	string base = "make correctness " + prec + " CTESTER=" 
			+ fileName + "CTester.c VER=";
			
	std::list<versionData*>::iterator itr;
	for (itr = versionList.begin(); itr != versionList.end(); ++itr) {
		int vid = (*itr)->vid;

		string test = fileName + "__" 
		+ boost::lexical_cast<string>(vid) + ".c";
		
		std::cout << "Testing: " << test << std::endl;
		
		// compile the test
		FILE *tf = popen((base+test+" 2>&1").c_str(), mode);
		if (tf == NULL) {
			std::cout << "Test failed to compile" << std::endl;
			continue;
		}
		// collect the compilation status
		buffer[0] = '\0';
		int fail = 0;
		while (fgets(buffer, 100, tf) != NULL) {
			if (buffer[0] != '\0') 
				fail = 1;
			
            std::cout << buffer << std::endl;
			buffer[0] = '\0';
		}
		if (pclose(tf) == -1)
            fail = 1;
        
		if (fail) {
			std::cout << "Test failed to compile" << std::endl;
			continue;
		}

		// run the test
		string cmd = "./a.out";
		cmd += " 2>&1";
		FILE *exe = popen(cmd.c_str(), mode);
		if (exe == NULL) {
			std::cout << "Test failed to execute" << std::endl;
			continue;
		}

		// collect the status
		while ( fgets(buffer, 100, exe) != NULL ) {
			if (strstr(buffer,"FAILED") != NULL
                || strstr(buffer,"Segmentation") != NULL) {
				// failed test
                std::cout << buffer << std::endl;
				exit(0);
			}
            std::cout << buffer << std::endl;
		}
        int err = pclose(exe);
        if (err) {
            std::cout << "Error with test: " << err << std::endl;
            exit(0);
        }	
	}
	
	chdir(cwd);
}

#define GLBL_SIZE 4000
void createMPIEmpiricalTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 bool noPtr, int reps) {

	//// Usefull data points
	int dSize = GLBL_SIZE;	// order of vectors and matrices created for test
	
	// map strings to types for all inputs and outputs
	std::map<string,pair<vertex,type> >data; 
	for (size_t i = 0; i != g.num_vertices(); i++) {
		switch (g.info(i).op) {
			case input:
			case output:
				data[g.info(i).label] = std::make_pair(i,g.info(i).t);
				break;
				
			default:
				break;
		}
	}	
	
	// find sizes that are actually used
	std::set<string> usedSizes;
	std::map<string,pair<vertex,type> >::iterator im = data.begin();
	for (; im != data.end(); ++im) {
		type &t = im->second.second;
		usedSizes.insert(t.dim.dim);
		type *tt = t.t;
		while (tt) {
			if (tt->k == scalar)
				break;
			if (tt->k != t.k) {
				usedSizes.insert(tt->dim.dim);
				break;
			}
			tt = tt->t;
		}
	}
	
	// create test file
	std::ofstream out((out_file_name + "ETester.c").c_str());
	
	out << "#include <stdlib.h>\n";
	out << "#include <stdio.h>\n";
	out << "#include \"mpi.h\"\n";
	out << "#include \"timer.h\"\n";
	out << "#include \"testUtils.h\"\n";
	out << "\n";
	
	// function signature
    // while creating this signature, also create the call string
    out << "extern void ";
    out << routine_name << function_args(inputs,outputs,data,"",typeWithName,noPtr);
    out << ";\n\n";
    string callString = routine_name + function_args(inputs,outputs,data,"",nameOnly,noPtr);
    callString += ";\n";
	
	// begin main
	out << "int main (int argc, char *argv[]) {\n";
	out << "int i;\n";
	out << "int reps = " << reps << ";\n";
	out << "int N = " << dSize << ";\n";
	out << "if (argc > 1) {\n";
	out << "char *end;\n";
	out << "N = strtol(argv[1],&end,10);\n";
	out << "if (end == argv[1]) printf(\"ERROR\\n\");\n";
	out << "}\n";
	
	// initialize variables
	std::set<string>::iterator is = usedSizes.begin();
	for (; is != usedSizes.end(); ++is) {
		if (is->compare("1") == 0)
			continue;
		out << "int " << *is << " = N;\n";
	}
	
	// create required data structures for inputs and outputs
	// fill with data	
	im = data.begin();
	for (; im != data.end(); ++im) {
		if (im->second.second.k == scalar) {
			out << precision_type << " " << im->first << " = 2.2;\n";
		}
		else {
			string n = im->first;
			string s = container_size_type(im->second.second);
			out << precision_type << "* " << n << " = (" << precision_type 
			<< "*) malloc(sizeof(" << precision_type << ") * "
			<< s << ");\n";
			generateData(out, n,  s);
		}
		
	}
	
	out << "MPI_Init(argc, argv);\n";
	out << "\n\n";
	
	// call  routine
	out << "INIT_TIMER;\n";
	out << "double bestTime = __DBL_MAX__;\n";
	out << "for (i=0; i < reps; ++i) {\n";
	out << "START_TIMER;\n";
	out << callString;
	out << "GET_TIMER;\n";
	out << "double curTime = DIFF_TIMER;\n";
	out << "bestTime = curTime < bestTime ? curTime : bestTime;\n";
	out << "}\n";
	out << "printf(\"%f\\n\",bestTime);\n";
	
	out << "MPI_Finalize();\n";
	// close main()
	out << "return 0;\n";
	out << "}\n";
	out.close();
}

void createEmpiricalTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 bool noPtr, int reps) {

	//// Usefull data points
	int dSize = GLBL_SIZE;	// order of vectors and matrices created for test
	
	// map strings to types for all inputs and outputs
	std::map<string,pair<vertex,type> >data; 
	for (size_t i = 0; i != g.num_vertices(); i++) {
		switch (g.info(i).op) {
			case input:
			case output:
				data[g.info(i).label] = std::make_pair(i,g.info(i).t);
				break;
				
			default:
				break;
		}
	}	
	
	// find sizes that are actually used
	std::set<string> usedSizes;
	std::map<string,pair<vertex,type> >::iterator im = data.begin();
	for (; im != data.end(); ++im) {
		type &t = im->second.second;
		usedSizes.insert(t.dim.dim);
		type *tt = t.t;
		while (tt) {
			if (tt->k == scalar)
				break;
			if (tt->k != t.k) {
				usedSizes.insert(tt->dim.dim);
				break;
			}
			tt = tt->t;
		}
	}
	
	// create test file
	std::ofstream out((out_file_name + "ETester.c").c_str());
	
	out << "#include <stdlib.h>\n";
	out << "#include <stdio.h>\n";
	out << "#include \"timer.h\"\n";
	out << "#include \"testUtils.h\"\n";
	out << "\n";
	
	// function signature
    // while creating this signature, also create the call string
    out << "extern void ";
    out << routine_name << function_args(inputs,outputs,data,"",typeWithName,noPtr);
    out << ";\n\n";
    string callString = routine_name + function_args(inputs,outputs,data,"",nameOnly,noPtr);
    callString += ";\n";
	
	// begin main
	out << "int main (int argc, char *argv[]) {\n";
	out << "int i,j;\n";
	out << "int reps = " << reps << ";\n";
	out << "int N = " << dSize << ";\n";
	out << "if (argc > 1) {\n";
	out << "char *end;\n";
	out << "N = strtol(argv[1],&end,10);\n";
	out << "if (end == argv[1]) printf(\"ERROR\\n\");\n";
	out << "}\n";
	
	// initialize variables
	std::set<string>::iterator is = usedSizes.begin();
	for (; is != usedSizes.end(); ++is) {
		if (is->compare("1") == 0)
			continue;
		out << "int " << *is << " = N;\n";
	}
	
	// create required data structures for inputs and outputs
	// fill with data	
	im = data.begin();
    vector<string> toFree;
	for (; im != data.end(); ++im) {
		if (im->second.second.k == scalar) {
			out << precision_type << " " << im->first << " = 2.2;\n";
		}
		else {
			string n = im->first;
			string s = container_size_type(im->second.second);
			out << precision_type << "* " << n << " = (" << precision_type 
			<< "*) malloc(sizeof(" << precision_type << ") * "
			<< s << ");\n";
			generateData(out, n,  s);
            toFree.push_back("free(" + n +");\n");
		}
		
	}
	
	out << "\n\n";
	
	// call  routine
	out << "struct timeval start, end;\n";
	out << "int num_in_epsilon=0;\n";
	out << "double epsilon = 0.1;\n";
	out << "gettimeofday(&start, NULL);\n";
	out << "double diff = 0;\n";
	out << "while(diff < epsilon){\n";
	out << callString;
	out << "gettimeofday(&end, NULL);\n";
	out << "diff= cal_sec(&start, &end);\n";
	out << "num_in_epsilon++;\n";
	out << "}\n";
	//out << "double bestTime = __DBL_MAX__;\n";
	//out << "for(j=0; j<reps; ++j){\n";
	out << "gettimeofday(&start, NULL);\n";
	out << "for (i=0; i < num_in_epsilon*reps; ++i) {\n";
	out << callString;
	out << "}\n";
	out << "gettimeofday(&end, NULL);\n";
	out << "double curTime = cal_sec(&start, &end);\n";
	//out << "bestTime = curTime < bestTime ? curTime : bestTime;\n";
	//out << "printf(\"%f\\n\",bestTime/num_in_epsilon);\n";
	out << "printf(\"%f\\n\",curTime/(num_in_epsilon*reps));\n";

	// close main()
    for (size_t i = 0; i < toFree.size(); ++i) {
        out << toFree[i];
    }
	out << "return 0;\n";
	out << "}\n";
	out.close();
}

int runEmpiricalTest(std::string &path, std::string &fileName, 
					 std::list<versionData*> &versionList,
					  int timeLimit, modelMsg &msg) {
	
	//change to working directory
	char cwd[1024];
	char *p = getcwd(cwd, 1024);
	if (p == NULL)
		std::cout << "Failed to get current working directory\n";
	chdir(path.c_str());

	
	// run all test present and find the best performing

	string prec;
	if (precision_type.compare("double") == 0)
		prec = "PREC=DREAL";
	else
		prec = "PREC=SREAL";
	
	const char mode[2] = {'r','\0'};
	char buffer[100];
	
	string defs = "DEFS=-DBTO_TILE";
	string base = "make empirical " + prec + " ETESTER=" 
			+ fileName + "ETester.c " + defs + " VER=";
	
	double bestTime = __DBL_MAX__;
	int bestVersion = -1;
	
	// keep track of testing time.  If a time limit is specified, check
	// between each empirical test and stop when limit is reached.
	struct timeval start, end;
	if (timeLimit > 0) {
		//std::cout << "Beginning empirical search with a time limit of "
				//<< timeLimit << " seconds\n\n";
		gettimeofday(&start,nullptr);
	}
	
	std::list<versionData*>::iterator itr;
	for (itr = versionList.begin(); itr != versionList.end(); ++itr) {
		int vid = (*itr)->vid;
		if (timeLimit > 0) {
			gettimeofday(&end, nullptr);
			if (cal_sec(&start, &end) > timeLimit) {
				std::cout << "\n\nEmpirical test time limit reached. "
					<< "Ending search\n\n";
				break;
			}
		}
		string test = fileName + "__" 
			+ boost::lexical_cast<string>(vid) + ".c";
		
		std::cout << "Testing: " << test << std::endl;
		//std::cout << base + test << "\n";
		
		// compile the test
		FILE *tf = popen((base+test+" 2>&1").c_str(), mode);
		if (tf == NULL) {
			std::cout << "Test failed to compile\n";
			continue;
		}
		// collect the compilation status
		buffer[0] = '\0';
		while (fgets(buffer, 100, tf) != NULL);
		pclose(tf);
#define USING_ICC
#ifndef USING_ICC
        if (buffer[0] != '\0') {
			std::cout << "Test failed to compile\n";
			printf("%s\n", buffer);
			continue;
		}
#endif
		
		// log empirical time
		modelData *newData = new modelData();
		double time = -1;
		double paramBestTime;
		
		// run for various sizes
		for (int start = msg.start; start <= msg.stop; start += msg.step) {
			// run for various parameters
			paramBestTime = HUGE_VAL;
			unsigned int pstart = 512;
			unsigned int pend = 512;
			unsigned int pstep = 16;
			for (unsigned int param = pstart; param <= pend; param+=pstep) {
				// run the test
				string cmd = string("./a.out") + string(" ") + boost::lexical_cast<string>(start);
				FILE *exe = popen(cmd.c_str(), mode);
				if (exe == NULL) {
					std::cout << "Test failed to execute\n";
					continue;
				}
				
				// collect the best run time
				while ( fgets(buffer, 100, exe) != NULL );
				pclose(exe);
				
				char *endPt;
				time = strtod(buffer, &endPt);
				if (endPt == buffer) {
					std::cout << "Failed to get timing\n";
					continue;
				}
				if (time < paramBestTime) {
					paramBestTime = time;
				}
				//std::cout << param << "\t" << time << "\n";
			}
			newData->add_data(start, paramBestTime);
		}
		
		if (paramBestTime < bestTime) {
			bestTime = paramBestTime;
			bestVersion = vid;
		}
		else if (paramBestTime == bestTime) {
			std::cout << "Warning: Two versions produced same time\n";
		}
		
		// display the empirical test time
		std::cout << paramBestTime << "\n";
		
		// log empirical time
		(*itr)->add_model(empirical,newData);
	}
	
	chdir(cwd);
	return bestVersion;
}

