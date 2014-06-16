//
//  iterator.cpp
//  btoblas
//

#include "syntax.hpp"
#include "iterator.hpp"
#include "analyze_graph.hpp"
#include "translate_utils.hpp"
#include "partition.hpp"

std::string var_name = "aijklpqrstabcdefgh";

// constructor
sg_iterator_t::sg_iterator_t(type &t, vertex v, subgraph* sg, void *vg) {
    graph &g = *(graph*)vg;
    
    string itrVar = string(1,var_name[depth(sg)+1]);
    
    format = t.s;
    
    attributes = NULL;

    bool colStartConstraint = false;
    bool colEndConstraint = false;
    bool rowStartConstraint = false;
    bool rowEndContraint = false;
    
    // ammount of offset from diagonal
    // for triangular, this is 0 for non-unit, and maybe 1
    // for unit?
    // for banded this will be the upper and lower bandwidths.
    string colStart = "";
    string colEnd = "";
    string rowStart = "";
    string rowEnd = "";
    
    kind orien;
    switch (format) {
        case general: {
            attributes = (void*) new general_itr();
            break;
        }
        case triangular: {
            attributes = (void*) new triangular_itr();
            triangular_itr *attrib = (triangular_itr*)attributes;

            // if this is multiplying the transpose of a triangular matrix
            // the upper and lower need to be swapped.
            vertex pred = v;
            bool foundTranspose = false;
            while (g.inv_adj(pred).size() == 1) {
                if (g.info(pred).op == trans) {
                    foundTranspose = true;
                    break;
                }
                pred = g.inv_adj(pred)[0];
            }
            if (foundTranspose)
                attrib->uplo = t.uplo == upper ? lower : upper;
            else
                attrib->uplo = t.uplo;
            orien = t.k;
            
            if (attrib->uplo == upper) {
                if (t.k == row) {
                    rowStartConstraint = true;      // diag
                    rowStart = "0";
                }
                else if (t.k == column) {
                    colEndConstraint = true;        // diag
                    colEndConstraint = "0";
                }
            }
            else {
                if (t.k == row) {
                    rowEndContraint = true;         // diag
                    rowEnd = "0";
                }
                else if (t.k == column) {
                    colStartConstraint = true;      // diag
                    colStart = "0";
                }
            }
            break;
        }
        default: {
            std::cout << "error: iterator.cpp: constructor struct sg_iterator:";
            std::cout << " unsupported format\n";
        }
    }
    
    if (!rowEndContraint && !rowStartConstraint && !colEndConstraint &&
        !colStartConstraint) {
        
        iterOp_t init(itrVar,"=","0");
        iterOp_t condition(itrVar,"<",t.dim.dim);
        iterOp_t update(itrVar,"+=",getLoopStep(&t));
        initializations.push_back(init);
        conditions.push_back(condition);
        updates.push_back(update);
        return;
    }
    //std::cout << "------------------\n";
    // determine any additional constraints
    vector<string> rowConstraints;
    vector<string> colConstraints;
    
    string lastRowStep = "";
    string lastColStep = "";
    
    subgraph *p = sg;
    while (p != NULL) {
        if (orien == row) {
            rowConstraints.push_back(p->sg_iterator.initializations.begin()->left);
            if (lastRowStep.compare("") == 0)
                lastRowStep = p->sg_iterator.updates.begin()->right;
        }
        else if (orien == column) {
            colConstraints.push_back(p->sg_iterator.initializations.begin()->left);
            if (lastColStep.compare("") == 0)
                lastColStep = p->sg_iterator.updates.begin()->right;
        }
        p = p->parent;
    }
    
    //std::cout << lastColStep << "\t" << lastRowStep << "\n";
    
    // always done
    iterOp_t update(itrVar,"+=",getLoopStep(&t));
    updates.push_back(update);
    
    vector<string>::iterator i;
    string rS = "";
    string cS = "";
    for (i = rowConstraints.begin(); i != rowConstraints.end(); ++i) {
        if (rS.compare("") == 0)
            rS = *i;
        else
            rS += "+" + *i;
    }
    for (i = colConstraints.begin(); i != colConstraints.end(); ++i) {
        if (cS.compare("") == 0)
            cS = *i;
        else
            cS += "+" + *i;
    }
    
    if (rowStartConstraint || colStartConstraint) {
        // find current over all location
        string initV;
        if (cS.compare("") == 0 && rS.compare("") == 0) {
            initV = "0";
        } else if (cS.compare("") == 0) {
            if (t.k == row)
                initV = "0";
            else
                initV = rS;
        } else if (rS.compare("") == 0) {
            if (t.k == row)
                initV = cS;
            else
                initV = "0";
        } else {
            if (t.k == row)
                initV = cS + "-(" + rS + ")";
            else
                initV = rS + "-(" + cS + ")";
            initV = "max(0," + initV + ")";
        }
        
        iterOp_t init(itrVar,"=",initV);
        initializations.push_back(init);
    } else {
        iterOp_t init(itrVar,"=","0");
        initializations.push_back(init);
    }
    
    if (rowEndContraint || colEndConstraint) {
        // alway need to be smaller than current size
        iterOp_t condition(itrVar,"<",t.dim.dim);
        conditions.push_back(condition);
        
        // find extra constraint
        string lhs = itrVar;
        string rhs;
        if (t.k == row) {
            rhs = cS;
            if (lastColStep.compare("1") != 0 && lastColStep.compare("") != 0)
                rhs += "+" + lastColStep;
            if (rS.compare("") != 0)
                lhs += "+"+rS;
        }
        else if (t.k == column) {
            rhs = rS;
            if (lastRowStep.compare("1") != 0 && lastRowStep.compare("") != 0)
                rhs += "+" + lastRowStep;
            if (cS.compare("") != 0)
                lhs += "+"+cS;
        }
        if (rhs.compare("") != 0) {
            iterOp_t condition2(lhs,"<=",rhs);
            conditions.push_back(condition2);
        }
    } else {
        iterOp_t condition(itrVar,"<",t.dim.dim);
        conditions.push_back(condition);
    }
}

// destructor
sg_iterator_t::~sg_iterator_t() {
    if (attributes) {
        switch (format) {
            case general:
                delete (general_itr*)attributes;
                break;
            case triangular:
                delete (triangular_itr*)attributes;
                break;
            default: 
                std::cout << "error: iterator.hpp: destructor struct ";
                std::cout << "sg_iterator: unsupported format\n";
        }
    }
}

// assignment
sg_iterator_t& sg_iterator_t::operator=(const sg_iterator_t &ot) {
    
    if (attributes) {
        switch (format) {
            case general:
                delete (general_itr*)attributes;
                break;
            case triangular:
                delete (triangular_itr*)attributes;
                break;
            default:
                std::cout << "error: iterator.hpp: assignment struct ";
                std::cout << "sg_iterator: unsupported format\n";
        }
    }
    
    initializations = ot.initializations;
    conditions = ot.conditions;
    updates = ot.updates;
    format = ot.format;
    
    if (ot.attributes) {
        switch (format) {
            case general:
                attributes = (void*) new general_itr(*(general_itr*)ot.attributes);
                break;
            case triangular:
                attributes = (void*) new triangular_itr(*(triangular_itr*)ot.attributes);
                break;
            default:
                std::cout << "error: iterator.hpp: assignment struct ";
                std::cout << "sg_iterator: unsupported format\n";
        }
    }
    else {
        attributes = NULL;
    }
    
    return *this;
}

// copy
sg_iterator_t::sg_iterator_t(const sg_iterator_t &oi) : 
		initializations(oi.initializations), conditions(oi.conditions),
		updates(oi.updates), format(oi.format) {
    switch(format) {
        case general:
            attributes = (void*) new general_itr(*(general_itr*)oi.attributes);
            break;
        case triangular:
            attributes = (void*) new triangular_itr(*(triangular_itr*)oi.attributes);
            break;
        default:
            std::cout << "error: iterator.hpp: copy struct sg_iterator: ";
            std::cout << "unsupported format\n";
    }
}

// return iterator as string.
string sg_iterator_t::getIterator(subgraph *sg) {
    return string(1,var_name[depth(sg)]);
}

// return serial C loop.
string sg_iterator_t::getSerialCLoop(subgraph *sg, bool useOneStep) {

    // when useOneStep is true
    // for (i = 0; ii = 0; i < iterations; i += step; ii++)
    // when false
    // for (i = 0; i < iterations. i += step) 
        
    string linitialize = "";
    vector<iterOp_t>::iterator i;
    for (i = initializations.begin(); i != initializations.end();
         ++i) {
        if (linitialize.compare("") == 0)
            linitialize += i->left + i->oper + i->right;
        else
            linitialize += "," + i->left + i->oper + i->right;
    }
    string lconditions = "";
    for (i = conditions.begin(); i != conditions.end();
         ++i) {
        if (lconditions.compare("") == 0)
            lconditions += i->left + i->oper + i->right;
        else
            lconditions += " && " + i->left + i->oper + i->right;
    }
    string lupdates = "";
    for (i = updates.begin(); i != updates.end();
         ++i) {
        if (lupdates.compare("") == 0)
            lupdates += i->left + i->oper + i->right;
        else
            lupdates += "," + i->left + i->oper + i->right;
    }
    // clean up names if partitions were introduced.
    linitialize = scrub_dollar(linitialize,"__m");
    lconditions = scrub_dollar(lconditions,"__m");
    lupdates = scrub_dollar(lupdates,"__s");

    string loop;
    loop = "for (" + linitialize + "; " + lconditions;
    loop += "; " + lupdates + ") {\n";
    
    // output subgraph id
	loop += "// " + sg->str_id + "\n";
    
    string partitionFixUp = "";
    // if a partition was introduced, handle uneven partitions
    for (i = updates.begin(); i != updates.end(); ++i) {
        if (i->right.find("$$") == 0) {
            string lstep = i->right;
            lstep.erase(0,2);
            bool haveMatch = false;
            vector<iterOp_t>::iterator j;
            for (j = conditions.begin(); j != conditions.end(); ++j) {
                if (j->left.compare(i->left) == 0) {
                    haveMatch = true;
                    break;
                }
            }
            if (!haveMatch) continue;
            string literations = j->right;
            literations = scrub_dollar(literations,"__m");
            
            partitionFixUp = "int __m" + lstep + " = " +
            i->left + " + __s" + lstep + " > " +
            literations + " ? " + literations + " - " +
            var_name[depth(sg)] + " : __s" + lstep + ";\n";
            
        }
    }

    return loop + partitionFixUp;
}


// compare fusability of two iterators
bool sg_iterator_t::fusable(sg_iterator_t &itr, bool allowPartitionUnion) {
    // determine if two iterators are fusable.  
    // allowPartitionUnion: when true if sizes do not match but both are
    // symbolic partitions sizes, assume these can be merged to be the same.
    // when false assume they cannot be merged.
	//std::cout << "testing fusable:" << std::endl;
	//std::cout << "\t" << iterations << "\t" << step << " fuse w/ " <<
		//itr.iterations << "\t" << itr.step << std::endl;
    
    bool iterationsMatch = false;
    bool stepsMatch = false;
    
    // NOTE: this is going to need some more attention, for
    // now just get it working for single condition case..
    string iterations0 = conditions.begin()->right;
    string iterations1 = itr.conditions.begin()->right;
    
    // is there a match of the iterations
    iterationsMatch = (iterations0.compare(iterations1) == 0);
    
    if (!iterationsMatch && allowPartitionUnion) {
        // if there is no match of iterations but partition sizes
        // are mergeable, then check to see if both sizes are partitions.
        // if so true, else false.
        iterationsMatch = (iterations0.find("$$") == 0 &&
                           iterations1.find("$$") == 0);
    }
    
    // return early if iterations do not match
    if (!iterationsMatch) {
		//std::cout << "fusable: iterations don't match" << std::endl;
        return false;
	}
    
    string step0 = updates.begin()->right;
    string step1 = itr.updates.begin()->right;
    
    // do the steps of each iterator match
    stepsMatch = (step0.compare(step1) == 0);
    
    if (!stepsMatch && allowPartitionUnion) {
        // if there is no match of steps but partition sizes
        // are mergeable, then check to see if both sizes are partitions.
        // if so true, else false.
        stepsMatch = (step0.find("$$") == 0 && step1.find("$$") == 0);
    }
    
    return stepsMatch;
}


// if partitioned return true, else false
bool sg_iterator_t::partitioned() {
    vector<iterOp_t>::iterator i;
    for (i = conditions.begin(); i != conditions.end(); ++i) {
        if (i->right.find("$$") == 0) return true;
    }
    for (i = updates.begin(); i != updates.end(); ++i) {
        if (i->right.find("$$") == 0) return true;
    }

    return false;
}




