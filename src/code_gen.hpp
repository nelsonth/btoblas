//
//  code_gen.hpp
//  btoblas


#ifndef btoblas_code_gen_hpp
#define btoblas_code_gen_hpp

#include "syntax.hpp"
#include "iterator.hpp"


struct c_loop_t;
struct pthread_loop_t;
struct pthread_function_t;
struct pthread_function_call_t;
struct pthread_message_t;
struct pthread_message_pack_t;
struct pthread_message_unpack_t;
struct pthread_message_alloc_t;
struct pthread_alloc_t;
struct pthread_join_t;
struct container_reduction_t;
struct pointer_update_t;
struct allocate_t;
struct free_t;
struct zero_data_t;
struct assign_t;
struct declare_t;
struct declare_partition_t;

enum code_gen_t {
	gen_top_level = 0x1,      // used to differential the vertices in top level
	// i.e. inputs and outputs.
	gen_c_loop =    0x2,
	gen_pthread_loop = 0x4
};
#define GEN_PARALLEL (gen_pthread_loop)
#define GEN_SERIAL (gen_c_loop)

// build vector for statement_t which can hold a wide range of
// things.
typedef union statementUnion {
	c_loop_t *c_loop;
	pthread_loop_t *pthread_loop;
	pthread_function_t *pthread_function;
	pthread_function_call_t *pthread_function_call;
	pthread_message_t *pthread_message;
	pthread_message_pack_t *pthread_message_pack;
	pthread_message_unpack_t *pthread_message_unpack;
	pthread_message_alloc_t *pthread_message_alloc;
	pthread_alloc_t *pthread_alloc;
	pthread_join_t *pthread_join;
	container_reduction_t *container_reduction;
	pointer_update_t *pointer_update;
	allocate_t *allocate;
	free_t *free;
	zero_data_t *zero_data;
	assign_t *assign;
	declare_t *declare;
	declare_partition_t *declare_partition;
} statement_union;

enum stmt_code {
	_c_loop,
	_pthread_loop,
	_pthread_function,
	_pthread_function_call,
	_pthread_message,
	_pthread_message_pack,
	_pthread_message_unpack,
	_pthread_message_alloc,
	_pthread_alloc,
	_pthread_join,
	_container_reduction,
	_pointer_update,
	_allocate,
	_free,
	_zero_data,
	_assign,
	_declare,
	_declare_partition
};

struct statement_t;
typedef vector<statement_t*> statement_list_t;

struct statement_t {
	// wrapper around pretty much any ast node.
	stmt_code code;
	statement_union stmt;

	~statement_t();
	string to_string();
	void hoist_allocations();
	void insert_frees();
	void hoist_zeros();
	void hoist_pointer_updates();
	void get_iterators(set<string> &itrs); 
	void get_partitions_used(map<string,int> &sizeMap,
			map<string,string> &partDeps,
			set<string> &funcParts);
	void assigns_to(vector<string> &assgns);
	void hoist_parallel_functions(statement_list_t &pFunc);
	void make_parallel_messages(statement_list_t &pMesg,
			vector<declare_t> &decls);
	void allocate_pthread_and_message(map<int, vector<string> > &sizeMap,
			vector<string> &sizes);
};

struct bto_function_t {
	// ordered set of statements in function body
	statement_list_t *stmts;

	// for generating the entry point function signature
	string name;                // function name
	map<string,type*> *inputs;  // input values
	map<string,type*> *outputs; // output values

	bto_function_t(statement_list_t *stmts, string name, 
			map<string,type*> *in,
			map<string,type*> *out) : stmts(stmts),name(name),
	inputs(in), outputs(out) {}

	~bto_function_t() {
		// input and outputs belong to main, do not delete

		for (size_t i = 0; i < stmts->size(); ++i)
			delete (*stmts)[i];

		delete stmts;
	}

	string to_string();
	void hoist_allocations();
	void insert_frees();
	void hoist_zeros();
	void hoist_pointer_updates();
	void hoist_parallel_functions(statement_list_t &pFunc);
	void get_iterators(set<string> &itrs);
	void get_partitions_used(map<string,int> &sizeMap,
			map<string,string> &partDeps);
	void make_parallel_messages(statement_list_t &pMesg);
	void allocate_pthread_and_message();
};

struct c_loop_t {
	// ordered set of statements in c loop
	statement_list_t *stmts;
	sg_iterator_t *iterator;
	subgraph *sub;

	c_loop_t(statement_list_t *stmts, sg_iterator_t *itr, subgraph *sg) : 
		stmts(stmts), iterator(itr), sub(sg) {}

	~c_loop_t() {
		for (size_t i = 0; i < stmts->size(); ++i)
			delete (*stmts)[i];

		delete stmts;

		// iterator and sub belong to the graph and will get cleaned
		// up when the graph is deleted.
	}

	string to_string();
	void hoist_allocations();
	void insert_frees();
	void hoist_zeros();
	void hoist_pointer_updates();
	void get_iterators(set<string> &itrs);
	void get_partitions_used(map<string,int> &sizeMap,
			map<string,string> &partDeps,
			set<string> &funcParts);
	void assigns_to(vector<string> &assgns);
	void hoist_parallel_functions(statement_list_t &pFunc);
	void make_parallel_messages(statement_list_t &pMesg,
			vector<declare_t> &decls);
};

struct pthread_loop_t {
	// ordered set of statement in pthread body
	statement_list_t *stmts;
	sg_iterator_t *iterator;
	subgraph *sub;

	pthread_loop_t(statement_list_t *stmts, sg_iterator_t *itr,
			subgraph *sg) : stmts(stmts), iterator(itr),
	sub(sg) {}

	~pthread_loop_t() {
		for (size_t i = 0; i < stmts->size(); ++i)
			delete (*stmts)[i];

		delete stmts;

		// iterator and sub belong to the graph and will get cleaned
		// up when the graph is deleted.   
	}

	string to_string();
	void hoist_allocations();
	void insert_frees();
	void hoist_zeros();
	void hoist_pointer_updates();
	void get_iterators(set<string> &itrs);
	void get_partitions_used(map<string,int> &sizeMap,
			map<string,string> &partDeps,
			set<string> &funcParts);
	void assigns_to(vector<string> &assgns);
	void hoist_parallel_functions(statement_list_t &pFunc);
	void make_parallel_messages(statement_list_t &pMesg,
			vector<declare_t> &decls);
	void allocate_pthread_and_message(map<int, vector<string> > &sizeMap,
			vector<string> &sizes);
};

struct pthread_function_t {
	// ordered set of statement in pthread body
	statement_list_t *stmts;

	// function name is name_body_uid
	string name;
	int uid;

	pthread_function_t(statement_list_t *stmts, string name, int uid) : 
		stmts(stmts),name(name),uid(uid) {}

	~pthread_function_t() {
		for (size_t i = 0; i < stmts->size(); ++i)
			delete (*stmts)[i];

		delete stmts;
	}

	string to_string();
	void hoist_allocations();
	void insert_frees();
	void hoist_zeros();
	void hoist_pointer_updates();
	void get_iterators(set<string> &itrs);
	void get_partitions_used(map<string,int> &sizeMap,
			map<string,string> &partDeps,
			set<string> &parts);
	void assigns_to(vector<string> &assgns);
	void hoist_parallel_functions(statement_list_t &pFunc);
	void make_parallel_messages(statement_list_t &pMesg,
			vector<declare_t> &decls);
	void handle_top_level_accesses(statement_list_t &access); 
	void allocate_pthread_and_message(map<int, vector<string> > &sizeMap,
			vector<string> &sizes);
};

struct pthread_function_call_t {
	string name;
	int uid;
	// call is
	// name_body_uid()

	pthread_function_call_t(string name, int uid) : name(name), uid(uid) {}

	string to_string();
};

struct pthread_message_pack_t {
	statement_list_t *stmts;
	// over all message will
	// name_uid_msg_t mesg_uid

	int uid;         // unique identifier
	string name;     // function name

	// JUST KEEPING A COPY OF pthread_message_t's pointer for
	// stmts

	pthread_message_pack_t(statement_list_t *stmts, string name, int uid) : 
		stmts(stmts), uid(uid), name(name) {}

	//~pthread_message_pack_t() {
	// stmts is just a copy of the original messages data so
	// no not delete.
	//}

	string to_string();
};

struct pthread_message_unpack_t {
	statement_list_t *stmts;
	// over all message will
	// name_uid_msg_t mesg_uid

	int uid;         // unique identifier
	string name;     // function name

	// JUST KEEPING A COPY OF pthread_message_t's pointer for
	// stmts

	pthread_message_unpack_t(statement_list_t *stmts, string name, int uid) : 
		stmts(stmts), uid(uid), name(name) {}

	//~pthread_message_unpack_t() {
	// stmts is just a copy of the original messages data so
	// no not delete.
	//}

	string to_string();
};

struct pthread_message_t {
	statement_list_t *stmts;
	// over all message will
	// name_uid_msg_t mesg_uid

	int uid;         // unique identifier
	string name;     // function name

	pthread_message_t(statement_list_t *stmts, string name, int uid) : 
		stmts(stmts), uid(uid), name(name) {}

	~pthread_message_t() {
		for (size_t i = 0; i < stmts->size(); ++i)
			delete (*stmts)[i];

		delete stmts;
	}

	string to_string();
	pthread_message_pack_t *make_message_packer();
	pthread_message_unpack_t *make_message_unpacker();
};

struct pthread_message_alloc_t {
	// allocate space for pthread messages
	int uid;
	string size;
	string name;

	pthread_message_alloc_t(int uid, string size, string name) : 
		uid(uid), size(size), name(name) {}
	void make_parallel_messages(vector<declare_t> &decls);

	string to_string();
};

struct pthread_alloc_t {
	// allocate pthreads
	int uid;
	string size;

	pthread_alloc_t(int uid, string size) : uid(uid), size(size) {}
	void make_parallel_messages(vector<declare_t> &decls);

	string to_string();
};

struct pthread_join_t {
	// join threads
	int uid;
	string size;

	pthread_join_t(int uid, string size) : uid(uid), size(size) {}

	string to_string();
};

struct container_reduction_t {
	// perform a reduction of containers

	vector<string> rLoops;      // holds the reduction loops
	string fromIndex;           // holds the indexing of the from side
	string toIndex;             // holds the indexing of the to side
	string zeroCheck;           // holds the check for = or +=
	string fromName;            // reduction scratch space name
	string toName;              // reducing to this
	string decls;               // local iterators to declare
	bool isScalar;              // reduce to scalar
	bool element;               // treat reduced scalar value as element when
	//   when true (t15[0]) or scalar otherwise (t15)
	bool reduceToSelf;

	container_reduction_t(vertex v, graph &g, string to, string from,
			bool sclr, bool element);
	void make_parallel_messages(vector<declare_t> &decls);
	void assigns_to(vector<string> &assgns);
	string to_string();
};

struct pointer_update_t {
	// details about pointer update as
	// c_type leftName = rightName + update
	// i.e.
	// double *t5 = t10 + i*A_ncols

	string c_type;
	string leftName;
	string rightName;
	string update;
	bool cast;

	pointer_update_t(string c_type, string leftName, string rightName,
			string update, bool cast = false) : c_type(c_type),
	leftName(leftName), rightName(rightName), update(update), cast(cast) {}

	string to_string();
	void assigns_to(vector<string> &assgns);
	void make_parallel_messages(vector<declare_t> &decls);
};

struct declare_t {
	// declare a variable as 
	// if (ptr)
	//  c_type *name
	// else
	//  c_type name
	string c_type;
	string name;
	bool ptr;

	declare_t(string c_type, string name, bool ptr=false) : 
		c_type(c_type), name(name), ptr(ptr) {}

	bool operator==(declare_t b) {
		return (name.compare(b.name) == 0);
	}
	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

struct allocate_t {
	// allocate temporary space as
	// if (scalar)
	//   c_type leftName
	// else
	//   c_type leftName = malloc(sizeof(c_type)*size) 

	string c_type;
	string leftName;
	string size;
	bool sclr;          // true if scalar

	allocate_t(string type, string name, string size, bool sclr) 
		: c_type(type), leftName(name), size(size), sclr(sclr) {}

	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

struct free_t {
	// free allocated space.
	// free(name)
	string name;

	free_t(string n) : name(n) {}

	string to_string();
};

struct zero_data_t {
	// zero the container or scalar
	// if sclr
	//   if element
	//     name[itr] = 0.0
	//   else
	//     name = 0.0
	// else
	//   for (int ii = 0; ii < size; ++ii)
	//     name[ii] = 0.0;
	string name;
	string size;
	bool sclr;
	bool element;
	string itr;

	zero_data_t(string name, string size, string itr, bool sclr, bool element) : name(name), size(size),
	sclr(sclr), element(element), itr(itr) {}

	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

struct constant_t {
	// constant value of c_type
	string c_type;
	union {
		int i;
		float f;
		double d;
	} val;

	constant_t(string c_type, int i) : c_type(c_type) {
		if (c_type.compare("int") != 0)
			std::cout << "code_gen.hpp; constant_t constructor; unexpected use\n";
		val.i = i;
	}
	constant_t(string c_type, float f) : c_type(c_type) {
		if (c_type.compare("float") != 0)
			std::cout << "code_gen.hpp; constant_t constructor; unexpected use\n";
		val.f = f;
	}
	constant_t(string c_type, double d) : c_type(c_type) {
		if (c_type.compare("double") != 0)
			std::cout << "code_gen.hpp; constant_t constructor; unexpected use\n";
		val.d = d;
	}
	constant_t(constant_t *ol) : c_type(ol->c_type) {
		if (c_type.compare("int") == 0)
			val.i = ol->val.i;
		else if (c_type.compare("float") == 0)
			val.f = ol->val.f;
		else if (c_type.compare("double") == 0)
			val.d = ol->val.d;
	}

	string to_string();
};

struct access_t {
	// element or scalar access as
	// if (sclr)
	//  name
	// else
	//  name[index]
	string name;
	string index;
	bool sclr;

	access_t(string name, string index, bool sclr) 
		: name(name), index(index), sclr(sclr) {}
	access_t(access_t *a) : name(a->name), index(a->index), sclr(a->sclr) {}

	string to_string();
	void assigns_to(vector<string> &assgns);
	void make_parallel_messages(vector<declare_t> &decls);
};

// build pair for allowing operands to be either access's or other
// operations
struct operation_t;

typedef union opUnion {
	operation_t *operation;
	access_t *access;
	constant_t *constant;
} op_union;

enum op_union_code {
	_operation,
	_access,
	_constant
};

struct operand_t {
	// an operand which can be access, or another operation.
	op_union_code code;
	op_union op;

	operand_t(access_t *acc, bool makeNew = false);
	operand_t(operation_t *opp, bool makeNew = false);
	operand_t(constant_t *con, bool makeNew = false);
	operand_t(operand_t *l);

	~operand_t();

	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

struct operation_t {
	// operation as
	// if (unary)
	//   (op(left))
	// else
	//   (left op right)
	// where left and right can be other operations
	// or element/scalar access.
	operand_t *left;
	operand_t *right;
	op_code op;
	bool unary;

	operation_t(op_code op, bool unary) : left(NULL), 
	right(NULL), op(op), unary(unary) {}
	operation_t(operation_t *ol) : op(ol->op), unary(ol->unary) {
		left = new operand_t(ol->left);
		if (!unary)
			right = new operand_t(ol->right);
		else
			right = NULL;
	}

	operation_t(const operation_t &ol) {
		op = ol.op;
		unary = ol.unary;

		left = new operand_t(ol.left);
		right = new operand_t(ol.right);
	}

	~operation_t() {
		if (left)
			delete left;
		if (right) 
			delete right;
	}

	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

typedef union rightHandSide {
	operation_t *operation;
	access_t *access;
	constant_t *constant;
} rhs_union;

enum rhs_code {
	_rhs_operation,
	_rhs_access,
	_rhs_constant
};

struct assign_t {
	// assign the results of some right hand side
	// if (sum)
	//   access_t += rhs
	// else
	//   access_t = rhs
	// which can be many things
	// b = c
	// b[i] = t6[i] + t7[i]
	// t9[i] += t5[j]
	// etc.
	access_t *leftHandSide;
	rhs_union rightHandSide;
	rhs_code rhsCode;
	bool sum;

	assign_t(access_t *left, rhs_union rhs, rhs_code rhsCode, bool sum) :
		leftHandSide(left), rightHandSide(rhs), rhsCode(rhsCode), sum(sum) {}

	~assign_t() {
		delete leftHandSide;
		switch (rhsCode) {
			case _rhs_operation:
				delete rightHandSide.operation;
				break;
			case _rhs_access:
				delete rightHandSide.access;
				break;
			case _rhs_constant:
				delete rightHandSide.constant;
				break;
		}
	}

	string to_string();
	void assigns_to(vector<string> &assgns);
	void make_parallel_messages(vector<declare_t> &decls);
};

struct declare_partition_t {
	string name;
	int partSize;
	string size;
	
	// decide between a partition being of a fixed size, i.e a tile size of
	// 128 elements, or a partition being of a number of partitions, i.e
	// 8 threads are wanted.
	bool fixedSize;         // if false, partition as name = size / partSize
	// if true, partitions as name = partSize

	declare_partition_t(string name, int partSize) : name(name), 
	partSize(partSize), fixedSize(true) {}
	declare_partition_t(string name, int partSize, string size) : name(name),
	partSize(partSize), size(size), fixedSize(false) {}

	string to_string();
	void make_parallel_messages(vector<declare_t> &decls);
};

struct bto_program_t {
	// ordered list of 
	// 1) message structures
	// 2) thread bodies
	// 3) entry point function

	bto_function_t *func;

	statement_list_t *parallel_mesgs;
	statement_list_t *parallel_funcs;

	~bto_program_t() {
		delete func;
		delete parallel_mesgs;
		delete parallel_funcs;
	}

	string to_string();
};

bto_program_t* build_AST(graph &g, string name, map<string,type*> &inputs,
		map<string,type*> &outputs,
		map<subgraph*, enum code_gen_t> &genMap);

#endif
