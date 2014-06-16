//
//  optimize_analysis.hpp
//  btoblas
//
//

#ifndef btoblas_optimize_analysis_hpp
#define btoblas_optimize_analysis_hpp

#include <iostream>
#include <fstream>
#include <utility>
#include "syntax.hpp"
#include "graph.hpp"
#include "translate_utils.hpp"

// describe the types of a binary operation
struct partitionOp_t {
    // left and right operands
    type *left;
    type *right;
    // result
    type *result;
    
    // constructor
    partitionOp_t(type *left, type *right, type *result) 
    :left(left), right(right), result(result) {}
    
    // destructor
    ~partitionOp_t() {
        if (left) delete left;
        if (right) delete right;
        if (result) delete result;
        left = right = result = NULL;
    }
    
	/*
    void transposeOperands(bool transLeft, bool transRight) {
        // if true, transpose type, else do nothing
        if (transLeft) {
            // transpose
            type *t = left;
			while (t) {
				if (t->k == row)
					t->k = column;
				else if (t->k == column)
					t->k = row;
				else
					break;
				t = t->t;
			}
        }
        if (transRight) {
            // transpose
            type *t = right;
			while (t) {
				if (t->k == row)
					t->k = column;
				else if (t->k == column)
					t->k = row;
				else
					break;
				t = t->t;
			}
        }
    }
	*/
};

/*
each operation will contain a tree of partition possibilities.
this tree will be as deep as the number of partitions being
introduced.  the tree is shaped as follows.
         unpartitioned operation
        /            |           \
      m-way        n-way        k-way
     /  |  \      /  |  \      /  |  \
   m    n   k    m   n   k    m   n   k

where certain operations will not have a way described when that
partitioning is illegal.  For example, all add/sub operations will
never have a k partition.
*/

struct partitionBranch_t;

struct partitionBranch_t {
    // recursively decribe the possible partitions for a binary
    // operation
    
    int partitionLevel;          
    
    // point to next level of tree when present, NULL otherwise
    struct partitionBranch_t *nextM;
    struct partitionBranch_t *nextN;
    struct partitionBranch_t *nextK;
    
    // types for current partition level.  null when not valid
    partitionOp_t *m;
    partitionOp_t *n;
    partitionOp_t *k;
    
    // constructors
    partitionBranch_t(int partitionLevel) : partitionLevel(partitionLevel),
    nextM(NULL), nextN(NULL), nextK(NULL), m(NULL), n(NULL), k(NULL) {}
    
    // destructor
    ~partitionBranch_t() {
        if (nextM) delete nextM;
        if (nextN) delete nextN;
        if (nextK) delete nextK;
        nextM = nextN = nextK = NULL;
        
        if (m) delete m;
        if (n) delete n;
        if (k) delete k;
        m = n = k = NULL;
    }
    
	/*
    void transposeOperands(bool transLeft, bool transRight) {
        // if true, transpose operand
        if (transLeft || transRight) {
            if (m)
                m->transposeOperands(transLeft, transRight);
            if (n)
                n->transposeOperands(transLeft, transRight);
            if (k)
                k->transposeOperands(transLeft, transRight);
            if (nextM)
                nextM->transposeOperands(transLeft, transRight);
            if (nextN)
                nextN->transposeOperands(transLeft, transRight);
            if (nextK)
                nextK->transposeOperands(transLeft, transRight);
        }
    }
	*/
};

struct partitionTree_t {
    // each operation described by a tree with possible multiple
    // resursive branches, each branch describing up to 3 complete
    // partitionings
    // tree -> branch -> partitionOp
    
    vertex left;
    vertex right;
    vertex result;      // result is the operation itself
    
    partitionBranch_t *partitions;  // branches.
    partitionOp_t *baseTypes;       // base types
    
    op_code op;
    
    int maxPartitionDepth;
    
    // constructor
    partitionTree_t(vertex left, vertex right, vertex result, 
                    int maxPartitionDepth, op_code op) : left(left), 
                    right(right), result(result), partitions(NULL), 
                    baseTypes(NULL), op(op),
                    maxPartitionDepth(maxPartitionDepth) {}
    
    //destructor
    ~partitionTree_t() {
        if (partitions) delete partitions;
        if (baseTypes) delete baseTypes;
        partitions = NULL;
        baseTypes = NULL;
    }
    
	/*
    void transposeOperands(bool transLeft, bool transRight) {
        // if true, transpose operand
        if (transLeft || transRight) {
            if (baseTypes)
                baseTypes->transposeOperands(transLeft, transRight);
            if (partitions)
                partitions->transposeOperands(transLeft, transRight);
        }
    }
	*/
};

struct partitionChoices_t {
	vector<sg_iterator_t *> iterators; // null if type not restricted
	vector<Way> branch_paths;
	vector<bool> reductions;

	// constructor
	partitionChoices_t(int n) : iterators(n), branch_paths(n), reductions(n) {}
};

partitionTree_t* initializeTree(graph &g, vertex v, int maxDepth);
void buildForest(vector<partitionTree_t*> &forest, graph &g, int maxDepth);

void deleteForest(vector<partitionTree_t*> &forest);

vector<partitionChoices_t> queryFuseSet(graph &g, vector<partitionTree_t *>t,  set<vertex> fuse, int depth);

vector<vector<Way> > queryPair(partitionTree_t *t1, partitionTree_t *t2,
		int depth, int numVertices);

void printTree(std::ofstream &out, partitionTree_t *t);

#endif
