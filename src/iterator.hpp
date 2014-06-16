//
//  iterator.hpp
//  btoblas
//
/* iterator descriptions
 structure which is attached to each subgraph to describe the bounds
 */

#ifndef btoblas_iterator_hpp
#define btoblas_iterator_hpp

#include "enums.hpp"
#include <string.h>
using std::string;
using std::vector;

struct type;
struct subgraph;


// iterators centrally defined in iterator.cpp
extern string var_name;

// format specific attributes.
struct triangular_itr { 
    uplo_t uplo;
};

struct general_itr {
    // no special information required for now.
};

struct iterOp_t {
    // left OP right
    // examples
    // i < A_nrows
    // i += 1
    // i = 0
    string left;
    string oper;
    string right;
    
    // constructor
    iterOp_t(string l, string o, string r) : left(l), oper(o),
    right(r) {}
    // copy
    iterOp_t(const iterOp_t &oi) : left(oi.left), oper(oi.oper),
    right(oi.right) {}
};

struct sg_iterator_t {
    
    // for (initializations; conditions; updates)
    vector<iterOp_t> initializations;
    vector<iterOp_t> conditions;
    vector<iterOp_t> updates;
    
    storage format;     // general, triangular, compressed, ...
    // this is to point to format specific descriptors
    // i.e. general, triangular, compressed.
    void *attributes;
    
    // constructor
    sg_iterator_t(type &t, unsigned int v, subgraph *sg, void *g);
    // copy
    sg_iterator_t(const sg_iterator_t &oi);
    // assignment
    sg_iterator_t& operator=(const sg_iterator_t &ot);
    // destructor
    ~sg_iterator_t();
    
    // return serial C loop.
    string getSerialCLoop(subgraph *sg, bool useOneStep=false);
    // get iterator
    string getIterator(subgraph *sg);
    
    // compare fusability of two iterators
    bool fusable(sg_iterator_t &itr, bool allowPartitionUnion);
    // if partitioned return true, else false
    bool partitioned();
    
    // utilities
    string printConditions() {
        vector<iterOp_t>::iterator i;
        string ret = "";
        for (i = conditions.begin(); i != conditions.end(); ++i)
            ret += i->left + i->oper + i->right + ";";
        return ret;
    }
    
    string printUpdates() {
        vector<iterOp_t>::iterator i;
        string ret = "";
        for (i = updates.begin(); i != updates.end(); ++i)
            ret += i->left + i->oper + i->right + ";";
        return ret;
    }
};



#endif
