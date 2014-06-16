#ifndef SYNTAX_HPP
#define SYNTAX_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include "boost/function.hpp"
#include "boost/lexical_cast.hpp"
#include <utility>

using std::vector;
using std::map;
using std::string;
using std::pair;
using std::set;

#define NUM_THREAD 4
#define STR_NUM_THREAD "NUM_THREADS"


#include "enums.hpp"


/* GRAPH.HPP USES SOME OF THE ABOVE ENUMERATIONS !! */
#include "graph.hpp"
#include "iterator.hpp"

struct dim_info {
	dim_info() : dim("?"), step("1"), base_rows("?"), base_cols("?"),
		lead_dim("?") {}

	dim_info(string br, string bc, string dim) : dim(dim), step("1"),
	base_rows(br), base_cols(bc), lead_dim("1") {}

	dim_info(string br, string bc, string dim, string blks, string lda) :
		dim(dim), step(blks), base_rows(br), base_cols(bc), lead_dim(lda) {}

	dim_info(const dim_info &info) : dim(info.dim), step(info.step),
	base_rows(info.base_rows), base_cols(info.base_cols),
	lead_dim(info.lead_dim) {}

	string dim;
	string step;

	//describe unpartitioned base structure (these should not change during partitioning)
	string base_rows;
	string base_cols;
	string lead_dim;

};

struct type {
	//for compiler
	// std::pair will not work without a type().  stupid.  this
	// opens up the potential for format erros all over the place
	// allowing this.  ideally the minimun would be type(storage).
	type() : k(unknown), s(any), uplo(uplo_null), diag(diag_null), dim(), t(0),
		height(0) {}

	type(storage s) : k(unknown), s(s), uplo(uplo_null), diag(diag_null),
		dim(), t(0), height(0) {}

	type(kind k, dim_info d, storage s) : k(k), s(s), uplo(uplo_null),
		diag(diag_null), dim(d), t(0), height(0) { }

	type(kind k, dim_info d, storage s, int height) : k(k), s(s),
		uplo(uplo_null), diag(diag_null), dim(d), t(0), height(height) { }
	
	type(kind k, dim_info d, storage s, int height, uplo_t uplo, diag_t diag) :
		k(k), s(s), uplo(uplo), diag(diag), dim(d), t(0), height(height) { }

	//for parsing only
	//matrix & vector
	type(string data, std::map<std::string,std::string> &attrib_list) :
		dim(), height(0) { 

		// supported attrubutes
		// orientation = [row, column]; defaults to column
		// format = [general, triangular]; defaults to general
		// uplo = [upper, lower]; must be specified when triangular is specified.
		//                  can only be specified when triangular is present

		if (data.compare("matrix") != 0 && data.compare("vector") != 0 && data.compare("tensor") != 0) {
			std::cout << "ERROR: syntax.hpp: type() constructor: expecting matrix\n";
			exit(-1);
		}

		// common attributes 
		string orien = "column";
		string uplo_s = "";
		string diag_s = "";
		string format = "general";
		string container = "";
		s = general;

		// iterate over attribute list and set corresponding values.
		map<string,string>::iterator itr = attrib_list.begin();
		for (; itr != attrib_list.end(); ++itr) {

			if (itr->first.compare("orientation") == 0) {
				orien = itr->second;
			}
			else if (itr->first.compare("format") == 0) {
				format = itr->second;
			}
			else if (itr->first.compare("uplo") == 0) {
				uplo_s = itr->second;
			}
			else if (itr->first.compare("diag") == 0) {
				diag_s = itr->second;
			} else if (itr->first.compare("container") == 0) {
				container = itr->second;
			}
			else {
				std::cout << "The attribute pair " << itr->first << ", "
					<< itr->second << " is not currently supported\n";
				exit(1);
			}
		}

		// perform checks on attributes.
		if (format.compare("general") == 0) {
			// general
			s = general;
			// uplo cannot be set
			if (uplo_s.compare("") != 0) {
				std::cout << "error: uplo cannot be set for a general matrix\n";
				exit(1);
			}
			// diag cannot be set
			if (diag_s.compare("") != 0) {
				std::cout << "error: diag cannot be set for a general matrix\n";
				exit(1);
			}
			uplo = uplo_null;
			diag = diag_null;
		}
		else if (format.compare("triangular") == 0) {
			// triangular
			s = triangular;
			// uplo must be set
			if (uplo_s.compare("") == 0) {
				std::cout << "error: uplo must be set for a triangular matrix\n";
				exit(1);
			}
			// diag must be set
			if (diag_s.compare("") == 0) {
				std::cout << "error: diag must be set for a triangular matrix\n";
				exit(1);
			}
			if (uplo_s.compare("upper") == 0)
				uplo = upper;
			else
				uplo = lower;
			if (diag_s.compare("unit") == 0)
				diag = unit;
			else
				diag = nonunit;
		}
		else {
			std::cout << "parse error: unsuported format: " << format << "\n";
			exit(1);
		}


		// build type based on attributes.
		// tensor type: takes a list like "RCCRC"
		// builds recursive type appropriately
		if (data.compare("tensor") == 0) {
			int sub_h = 0;
			type *sub_t = new type(scalar, dim_info("???","???","1"), general, 0,
							uplo_null, diag_null);
			// stopping before outermost container
			for (int i = container.size() -1 ; i > 0; --i) {
				++sub_h;
				kind subk = container[i] == 'R' ? row : column;
				type *outer_t = new type(subk, dim_info(), general, sub_h, uplo_null,
						diag_null);
				outer_t->t = sub_t;
				sub_t = outer_t;
			}
			// Now handle myself as outermost container
			k = (container[0] == 'R' ? row : column);
			t = sub_t;
			height = sub_h + 1;
		}
		// matrix
		else if (data.compare("matrix") == 0) {	
			if (orien.compare("column") == 0) {
				k = row;
				height = 2;

				//column
				type *clm = new type(column, dim_info(), s, 1, uplo, diag);
				t = clm; 

				//scalar
				type *scl = new type(scalar, dim_info("???","???","1"), s, 0,
						uplo_null, diag_null);
				clm->t = scl;			
			}
			else if (orien.compare("row") == 0) {
				k = column;
				height = 2;

				//row
				type *rw = new type(row, dim_info(), s, 1, uplo, diag);
				t = rw; 

				//scalar
				type *scl = new type(scalar, dim_info("???","???","1"), s, 0,
						uplo_null, diag_null);
				rw->t = scl;
			}
			else {
				std::cout << "ERROR: syntax.hpp: type structure constructor: unknown orientation\n";
			}
		}
		//vector
		else {
			if (orien.compare("row") == 0) {
				k = row;
				height = 1;
				dim.base_rows = "1";

				//scalar
				type *scl = new type(scalar, dim_info("1","???","1"), general, 0);
				t = scl;			
			}
			else if (orien.compare("column") == 0) {
				k = column;
				height = 1;
				dim.base_cols = "1";

				//scalar
				type *scl = new type(scalar, dim_info("???","1","1"), general, 0);
				t = scl;
			}
			else {
				std::cout << "ERROR: syntax.hpp: type structure constructor: unknown orientation\n";
			}

		}
	}

	//scalar
	type(kind k) : k(k), s(general), uplo(uplo_null), diag(diag_null), dim("1","1","1","1","1"), t(0), height(0) { }

	//copy
	type(const type &ot) : k(ot.k), s(ot.s), uplo(ot.uplo), diag(ot.diag), dim(ot.dim), height(ot.height) 
	{

		if (ot.t) {
			t = new type(*(ot.t));
		}
		else
			t = 0;
	}	

	// assignment operator
	type& operator=(const type &ot) {
		k = ot.k;
		s = ot.s;
		dim = ot.dim;
		height = ot.height;
		uplo = ot.uplo;
		diag = ot.diag;

		if (t)
			delete t;

		if (ot.t) {
			t = new type(*(ot.t));
		}
		else
			t = 0;

		return *this;
	}

	// destructor
	~type() {
		if (this->t)
			delete this->t;
		this->t = 0;
	}

	kind k;         // row/column/scalar
	storage s;      // general/triangular
	uplo_t uplo;    // upper/lower
	diag_t diag;    // unit/nonunit
	dim_info dim;
	type *t;
	int height;


	// util functions //
	//
	
	type *get_highest_row() {
		if (this->k == scalar)
			return this;

		type *t = this;
		while (t) {
			if (t->k == row || t->k == scalar)
				return t;
			t = t->t;
		}
		std::cout << "ERROR: syntax.hpp: get_highest_row(): unexpected type\n";
		return t;
	}

	type *get_highest_column() {
		if (this->k == scalar)
			return this;

		type *t = this;
		while (t) {
			if (t->k == column || t->k == scalar)
				return t;
			t = t->t;
		}
		std::cout << "ERROR: syntax.hpp: get_highest_column(): unexpected type\n";
		return t;
	}

	type *get_lowest_ns() {
		if (this->k == scalar)
			std::cout << "ERROR: syntax.hpp: get_lowest_ns(): unexpected type\n";

		type *t = this;
		while (t) {
			if (t->t->k == scalar)
				return t;
			t = t->t;
		}
		std::cout << "ERROR: syntax.hpp: get_lowest_ns(): unexpected type\n";
		return NULL;
	}

	int whichPartition(string dim, kind k) {
		// a container above will call this to
		// find which container below is its matching
		// partition and return the height of that
		// partition.
		if (dim.compare("1") == 0)
			return -1;

		type *t = this;
		while (t) {
			if (t->k == k && t->dim.dim.compare(dim) == 0)
				return t->height;
			t = t->t;
		}
		return -1;
	}

	bool beenPartitioned() {
		type *t = this;
		int r, c;
		r = c = 0;
		while (t) {
			if (t->k == row) 
				++r;
			else if (t->k == column)
				++c;

			if (r > 1 || c > 1) {
				return true;
			}
			t = t->t;
		}
		return false;
	}

	int num_rows() {
		int ret = 0;
		type *t = this;

		while (t) {
			if (t->k == row)
				ret++;
			if (t->k == scalar)
				break;
			t = t->t;			
		}
		return ret;
	}

	int num_cols() {
		int ret = 0;
		type *t = this;

		while (t) {
			if (t->k == column)
				ret++;
			if (t->k == scalar)
				break;
			t = t->t;
		}
		return ret;
	}	
}; 

inline bool operator==(const type &t1, const type &t2) {
	//this may need to be recursive now...
	bool result = (t1.k == t2.k);

	result &= (t1.s == t2.s || t1.s == any || t2.s == any);

	return result;
}


enum eval_choice { defer, evaluate };
enum param_access { once, many };
enum computation_kind { inseparable, decouple };

string type_to_string(const type &t);
string op_to_string(op_code op);
string op_to_name(op_code op);

typedef unsigned int vertex;

struct vertex_info
{
	vertex_info() : t(type(unknown)), eval(defer), trivial(true) { }

	vertex_info(op_code op) : t(type(unknown)), op(op), eval(defer),
		trivial(true) { }
	
	vertex_info(op_code op, string label) : t(type(unknown)), op(op),
		label(label), eval(defer), trivial(true) { }

	vertex_info(const type &t, op_code op, string label) : t(t), op(op),
		label(label), eval(defer), trivial(true) { }

	vertex_info(const type &t, op_code op, string label, double v) : t(t),
		op(op), val(v), label(label), eval(defer), trivial(true) { }

	type t;
	op_code op;
	double val;		// if a constant is provided store value
	string label;
	eval_choice eval;
	int algo;
	bool trivial; // some computation happens, not just I/O

	string to_string(vertex i) const;

};

typedef Graph<vertex_info> graph;
typedef boost::function<bool(vertex,graph&)> rewrite_fun;
typedef boost::function<bool(vertex,graph&,bool)> partition_fun;
typedef boost::function<subgraph_id(vertex,graph&,std::pair<subgraph_id,subgraph_id >&)> 
	optim_fun;
typedef boost::function<set<pair<subgraph_id,subgraph_id > >
	(vertex,graph&,std::set<subgraph_id>&)> optim_fun_chk;
typedef boost::function<type(vertex u, vector<vertex>, graph&)> return_type_fun;
typedef boost::function<vector<param_access>(vector<vertex>, vertex u, graph&)> param_access_fun;
typedef boost::function<bool(vertex, graph &)> constraint_fun;

struct algo {
	algo(op_code op, return_type_fun rt, computation_kind comp, param_access_fun pa, type arg, 
			rewrite_fun r,bool w, constraint_fun cf)
		: op(op), return_type(rt), comp(comp), param_accesses(pa), refiner(r), 
		does_work(w), additionalConstraints(cf) {
			param_types.push_back(arg);	
		}
	algo(op_code op, return_type_fun rt, computation_kind comp, param_access_fun pa, type arg1,  
			type arg2, rewrite_fun r, bool w, constraint_fun cf)
		: op(op), return_type(rt), comp(comp), param_accesses(pa), refiner(r), 
		does_work(w), additionalConstraints(cf) {
			param_types.push_back(arg1);
			param_types.push_back(arg2);
		}
	op_code op;
	return_type_fun return_type;
	computation_kind comp;
	vector<type> param_types;
	param_access_fun param_accesses;
	rewrite_fun refiner;
	bool does_work;
	constraint_fun additionalConstraints;
};

typedef std::pair<const std::string, type*> param;
typedef std::pair<std::string, std::string> attrib;

struct expr { 
	virtual void print() = 0;
	virtual vertex to_graph(map<string, vertex>& env, 
			map<string,type*>const& inputs, 
			map<string,type*>const& outputs, 
			graph& g) = 0;
	virtual ~expr() { }
};

struct operation : public expr {
	op_code op;
	vector<expr*> operands;

	operation(op_code op, expr* e) : op(op) {
		operands.push_back(e);
	}

	operation(op_code op, expr* e1, expr* e2) : op(op) {
		operands.push_back(e1);
		operands.push_back(e2);
	}

	virtual vertex to_graph(map<string, vertex>& env, 
			map<string,type*>const& inputs, 
			map<string,type*>const& outputs, 
			graph& g);

	virtual void print();

};

struct variable : public expr {
	variable(std::string* n) : name(*n) { }
	std::string name;
	void print();
	virtual vertex to_graph(map<string, vertex>& env, 
			map<string,type*>const& inputs, 
			map<string,type*>const& outputs, 
			graph& g);
};

struct scalar_in : public expr {
	scalar_in(double val) : val(val) { }
	double val;
	void print();
	virtual vertex to_graph(map<string, vertex>& env, 
			map<string,type*>const& inputs, 
			map<string,type*>const& outputs, 
			graph& g);
};


struct stmt {
	stmt(std::string* l, expr* rhs) : lhs(*l), rhs(rhs) { }
	std::string lhs;
	expr* rhs;
	void print();
};


void print_program(vector<stmt*>& p);

// partition stuff
void make_partitions(graph &g);
pair<std::string, std::string> partition_to_c(vertex u, graph &g, std::ostream& out);

#endif // SYNTAX_HPP
