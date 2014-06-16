#include "type_analysis.hpp"
#include "build_graph.hpp"
#include <fstream>

//////////////////////////////// INITIAL COMPUTATION OF TYPES ///////////////////////////////////

string prnt(type *t) {
	if (t->k == scalar)
		return " S";
	else
		if (t->k == row)
			return " R" + prnt(t->t);
		else
			return " C" + prnt(t->t);
}

string prnt_detail(type *in) {
	type *t  = in;
	string orien = "";
	
	while (t->k != scalar) {
		if (t->k == row)
			orien += " R";
		else
			orien += " C";
		t = t->t;
	}
	orien += " S";
	
	t = in;
	string dim = "";
	while (t->k != scalar) {
		if (t->k == row)
			dim += " " + t->dim.dim;
		else
			dim += " " + t->dim.dim;
		t = t->t;
	}
	dim += " " + t->dim.dim;
	
	t = in;
	string step = "";
	while (t->k != scalar) {
		if (t->k == row)
			step += " " + t->dim.step;
		else
			step += " " + t->dim.step;
		t = t->t;
	}
	step += " " + t->dim.step;
	
	t = in;
	string rows = "";
	while (t->k != scalar) {
		if (t->k == row)
			rows += " " + t->dim.base_rows;
		else
			rows += " " + t->dim.base_rows;
		t = t->t;
	}
	rows += " " + t->dim.base_rows;
	
	t = in;
	string col = "";
	while (t->k != scalar) {
		if (t->k == row)
			col += " " + t->dim.base_cols;
		else
			col += " " + t->dim.base_cols;
		t = t->t;
	}
	col += " " + t->dim.base_cols;
	
	t = in;
	string ld = "";
	while (t->k != scalar) {
		if (t->k == row)
			ld += " " + t->dim.lead_dim;
		else
			ld += " " + t->dim.lead_dim;
		t = t->t;
	}
	ld += " " + t->dim.lead_dim;
	
	return orien + "\ndim: " + dim + "\nstep: " + step + "\nbase_rows: " 
			+ rows + "\nbase_cols: " 
			+ col + "\nlead_dim: " + ld + "\n";
}

void add_to_curr(type *newt, type &curr) {
	if (curr.k == unknown)
		curr = *newt;
	else {
		type *t = &curr;
		while (t->t)
			t = t->t;
		t->t = newt;
	}
}

void fix_height(type *tp) {
	if (tp->k != scalar) {
		fix_height(tp->t);
		tp->height = tp->t->height + 1;	
	}
	else
		tp->height = 0;
}

void know_ops_mult(type *l, type *r, type curr, vector<type> &rt) {
	if (l->k == row && r->k == row) {		
		type *newr = new type(*r);
		delete newr->t;
		newr->t = 0;
		add_to_curr(newr, curr);
		know_ops_mult(l,r->t, curr, rt);
	}
	else if (l->k == column && r->k == column) {
		type *newl = new type(*l);
		delete newl->t;
		newl->t = 0;
		add_to_curr(newl,curr);
		know_ops_mult(l->t,r, curr, rt);		
	}
	else if (l->k == row && r->k == column) {
		know_ops_mult(l->t,r->t,curr, rt);
	}
	else if (l->k == column && r->k == row) {
		type *cpy_curr = new type(curr);
		
		type *newl = new type(*l);
		delete newl->t;
		newl->t = 0;
		add_to_curr(newl,curr);
		know_ops_mult(l->t,r,curr,rt);
		
		type *newr = new type(*r);
		delete newr->t;
		newr->t = 0;
		add_to_curr(newr,*cpy_curr);
		know_ops_mult(l,r->t,*cpy_curr,rt);
	}
	else if (l->k == scalar && r->k != scalar) {
		type *newr = new type(*r);
		delete newr->t;
		newr->t = 0;
		add_to_curr(newr, curr);
		know_ops_mult(l,r->t,curr, rt);
	}
	else if (l->k != scalar && r->k == scalar) {
		type *newl = new type(*l);
		delete newl->t;
		newl->t = 0;
		add_to_curr(newl,curr);
		know_ops_mult(l->t ,r, curr, rt);
	}
	else if (l->k == scalar && r->k == scalar) {
		type *scl = new type(*l);
		delete scl->t;
		scl->t = 0;
		add_to_curr(scl,curr);
		
		// update height and lda
		fix_height(&curr);
		//add current to ptypes
		rt.push_back(curr);
	}
	else {
		std::cout << "ERROR: type_analysis.cpp: know_ops_mult(): unexpected types\n";
	}	
}

bool rtc(type *l, type *r) {
	if (l->height != r->height)
		return false;
	while (l && r) {
		if (l->k != r->k)
			return false;
		l = l->t;
		r = r->t;
	}
	return true;
}

int check_mult(type *l, type *r, type *u) {
	// recursively check a multiply
	// return 0, 1, 2
	// 0 -> illegal operation
	// 1 -> correct operation
	// 2 -> not enough information or unfinished check
	
	if (l->k == row && r->k == row) {		
		return 2;
	}
	else if (l->k == column && r->k == column) {
		return 2;		
	}
	else if (l->k == row && r->k == column) {
		return check_mult(l->t,r->t,u);
	}
	else if (l->k == column && r->k == row) {
		if (u->k == row) {
			return check_mult(l,r->t,u->t);
		}
		else if (u->k == column) {
			return check_mult(l->t,r,u->t);
		}
		else {
			return 0;
		}
	}
	else if (l->k == scalar && r->k != scalar) {
		return rtc(r,u);
	}
	else if (l->k != scalar && r->k == scalar) {
		return rtc(l,u);
	}
	else if (l->k == scalar && r->k == scalar) {
		return 1;
	}
	else {
		std::cout << "ERROR: type_analysis.cpp: mult_check(): unexpected types\n";
		return 0;
	}
}

int check_types(vector<vector<type> > &ptypes) {
	int known = 0;
	for (unsigned int i = 0; i != ptypes.size(); i++) {
		if (ptypes[i].size() == 1)
			known++;
	}	
	return known;
}



void compute_types(graph &g) {
	vector<vector<type> > *pt = new vector<vector<type> >
    (g.num_vertices(),vector<type>());
	vector<vector<type> > &ptypes = *pt;
	
	// infer the types for all vertices with unknowns.  most of the time, this
	// works, however it is possible to create a program with a temporary outer
	// product result where either row or column major is correctly typed.  in
	// this case there are a couple of options.
	// 1) give the user enough information to modify the program so a type
	//			can be assigned
	// 2) arbitrarily select one of the safe types and see if inference can
	//			continue
	// 3) produce all correctly typed versions.
	//
	// For now, we take option 2.  This may need to be revisisted in the future.
	// to achieve this, compute_types will run in 2 modes.  
	// 1) strict - default and forces outer products to be fully resolved
	// 2) relaxed - after default mode can make no more progress, enter relaxed
	//			mode which selects one of the two outer product result types.
	
	
	
	// if the type is known for a vertex add it to ptypes
	for (vertex v = 0; v != g.num_vertices(); v++) {
		if (g.info(v).t.k != unknown)
			ptypes[v].push_back(g.info(v).t);
		
	}
	//std::ofstream out("lower.dot");
    //print_graph(out, g);
	
	enum type_mode_t {strict, relaxed};
	
	enum type_mode_t mode = strict;
	
	int last = -1;
	while (true) {
		for (vertex v = 0; v != g.num_vertices(); v++) {
			switch (g.info(v).op) {
				case add:
				case subtract: {
					vertex l = g.inv_adj(v)[0];
					vertex r = g.inv_adj(v)[1];
					//std::cout << l << "\t" << r << "\n";
					int check = 0;
					//operands must be same type
					if (ptypes[l].size() == 1 && ptypes[r].size() == 1) {
						if (!rtc(&ptypes[l][0],&ptypes[r][0]))  {
							std::cout << "addition at " << v << ": l&r dont match\n"; 
							std::cout << prnt(&ptypes[l][0]) << "\t+\t" << prnt(&ptypes[r][0]) 
										<< "\n"; 
							std::cout << "Type Error\n";
							exit(1);
						}
						check++;
					}
					//ops must match return type
					if (ptypes[l].size() == 1 && ptypes[v].size() == 1) {
						if (!rtc(&ptypes[l][0],&ptypes[v][0])) {
							std::cout << "addition at " << v << ": l&u dont match\n"; 
							std::cout << prnt(&ptypes[l][0]) << "\t+\t" << prnt(&ptypes[v][0]) 
										<< "\n"; 
							std::cout << "Type Error\n";
							exit(1);
						}					
						check++;
					}
					if (ptypes[r].size() == 1 && ptypes[v].size() == 1) {
						if (!rtc(&ptypes[r][0],&ptypes[v][0])) {
							std::cout << "addition at " << v << ": r&u dont match\n";
							std::cout << prnt(&ptypes[r][0]) << "\t+\t" << prnt(&ptypes[v][0]) 
										<< "\n"; 
							std::cout << "Type Error\n";
							exit(1);
						}
						check++;
					}
					//dont need updates
					if (check == 3)
						break;
					
					if (ptypes[l].size() == 1) {
						ptypes[r] = ptypes[l];
						ptypes[v] = ptypes[l];
					}
					else if (ptypes[v].size() == 1) {
						ptypes[l] = ptypes[v];
						ptypes[r] = ptypes[v];
					}
					else if (ptypes[r].size() == 1) {
						ptypes[l] = ptypes[r];
						ptypes[v] = ptypes[r];
					}
						
					break;
				}
				case squareroot: {
					vertex l = g.inv_adj(v)[0];
					if (ptypes[l].size() == 1 && ptypes[v].size() == 1) {
						if (!rtc(&ptypes[l][0],&ptypes[v][0])) {
							std::cout << "addition at " << v << ": l&u dont match\n"; 
							std::cout << prnt(&ptypes[l][0]) << "\t+\t" << prnt(&ptypes[v][0]) 
										<< "\n"; 
							std::cout << "Type Error\n";
							exit(1);
						}					
						break;
					}
					if (ptypes[l].size() == 1) {
						ptypes[v] = ptypes[l];
					}
					else if (ptypes[v].size() == 1) {
						ptypes[l] = ptypes[v];
					}
					break;
				}
				case divide: {
					vertex l = g.inv_adj(v)[0];
					vertex r = g.inv_adj(v)[1];
					if (ptypes[l].size() == 1 && ptypes[r].size() == 1 && ptypes[v].size() == 1) {
						if (!rtc(&ptypes[l][0],&ptypes[v][0])) {
							std::cout << "division at " << v << ": l&u don't match\n";
							std::cout << prnt(&ptypes[l][0]) << "\t+\t" << prnt(&ptypes[v][0])
										<< "\n";
							std::cout << "Type Error\n";
							exit(1);
						}
						break;
					}
					if (ptypes[l].size() == 1) {
						ptypes[v] = ptypes[l];
					}
					else if (ptypes[v].size() == 1) {
						ptypes[l] = ptypes[v];
					}
					break;
				}
				case multiply: {
					vector<type> &l = ptypes[g.inv_adj(v)[0]];
					vector<type> &r = ptypes[g.inv_adj(v)[1]];
					vector<type> &u = ptypes[v];
					
					if (l.size() == 1 && r.size() == 1 && u.size() == 1) {
						// check types here
						switch (check_mult(&l[0],&r[0],&u[0])) {
							case 0:
								// Error
								std::cout << "Type Error\n";
								std::cout << "multiplication at " << v << ": ";
								std::cout << prnt(&l[0]) << "\t*\t" << prnt(&r[0]) 
										<< "\t=\t" << prnt(&u[0]) << "\n"; 
								exit(1);								
							case 1:
								// Pass
								break;
							case 2:
								// Unhandled case
								
								// do nothing for now, this will need to beome
								// more strict.
								break;
						}
						break;
					}
					//if I dont know anything move on
					if (l.size() == 0 && r.size() == 0 && u.size() == 0)
						break;
					
					if (u.size() == 1) {
						//find possible operand types
						
					}
					else if (l.size() == 1 && r.size() == 1) {
						//find possible return types
						type *tt = new type();
						u.clear();
						know_ops_mult(&l[0], &r[0], *tt, u);
						delete tt;
						
						// if we have entered relaxed mode and we can
						// identify this as an outer product and the
						// possible result types are 2, select the first.
						if (mode == relaxed) {
							// if outerproduct && there are 2 result types
							if (l[0].k == column && r[0].k == row && u.size() == 2) {
								// switch back to strict mode so that we make as few
								// arbitrary decisions as possible
								mode = strict;
								u.erase(u.begin()+1);
								std::cout << "\nWARNING: type_analysis.cpp; compute_types(); "
										  << "outer product result with arbitrary row/col major "
										  << "type identified.  Selecting ";
								if (u[0].k == row)
									std::cout << "column ";
								else
									std::cout << "row ";
								std::cout << "major\n\n";
							}
						}
					}
					break;
				}
				case trans: {
					vector<type> &u = ptypes[v];
					vector<type> &pred = ptypes[g.inv_adj(v)[0]];
					if (u.size() == 1) {
						if (pred.size() == 1) {
							if (pred[0].height != u[0].height) {
								std::cout << "transpose at " << v
                                        << ": pred mismatch 1\n";
								std::cout << prnt(&pred[0]) << "\t'\t"
                                        << prnt(&u[0]) << "\n"; 
								std::cout << "Type Error\n";
								exit(1);
							}
							type *l = &u[0];
							type *r = &pred[0];
							while (l && r) {
								if (!((l->k == row && r->k == column) 
									|| (l->k == column && r->k == row) 
									|| (l->k == scalar && r->k == scalar))) {
									std::cout << "transpose at " << v << ": pred mismatch 2\n";
									std::cout << prnt(&pred[0]) << "\t+\t" << prnt(&u[0]) 
										<< "\n"; 
									std::cout << "Type Error\n";
									exit(1);
								}
								l = l->t;
								r = r->t;
							}
						}
						else {
							type *nt = new type(u[0]);
							type *t = nt;
							while (t) {
								if (t->k == row)
									t->k = column;
								else if (t->k == column)
									t->k = row;
								t = t->t; 
							}
							pred = *new vector<type>(1,*nt);
							delete nt;
						}
					}
					else if (pred.size() == 1) {
						type *nt = new type(pred[0]);
						type *t = nt;
						while (t) {
							if (t->k == row)
								t->k = column;
							else if (t->k == column)
								t->k = row;
							t = t->t;
						}
						u.clear();
						u.push_back(*nt);
						delete nt;
					}
					break;
				}
				default:
					break;
			}
		}
		int known = check_types(ptypes);

		// if all types are known: done
		if (known == static_cast<int>(g.num_vertices()))
			break;
		
		// if no change has occured.. somethings wrong
		// if currently in strict, downgrade to relaxed
		// else, we have something that this algorithm cannot
		// resolve.
		if (last == known && mode == strict) {
			mode = relaxed;
			last = -1;
			continue;
		}
		else if (last == known) {
			std::cout << "ERROR: type_analysis.cpp: compute_types(): unable to compute all types\n";
			std::cout << known << " vertices have known types, there are " << g.num_vertices()
					  << " vertices\n";
			
			for (unsigned int i = 0; i != g.num_vertices(); i++)
				if (ptypes[i].size() == 1)
    				g.info(i).t = ptypes[i][0];
    				
			std::ofstream out("lower.dot");
    		print_graph(out, g);
    		
			exit(0);
		}
		last = known;
	}
    
    for (unsigned int i = 0; i != g.num_vertices(); i++)
    	g.info(i).t = ptypes[i][0];
	
	delete pt;
	//std::ofstream out("lower.dot");
    //print_graph(out, g);
}

//////////////////// END INITIAL COMPUTATION OF TYPES /////////////////////////////////////////////

//////////////////// USING YICES COMMAND LINE ////////////////////////////////////////////////////
string to_yices(type &t) {
	switch (t.k) {
	case row:
		return "(N Row " + to_yices(*t.t) + ")";
		break;
	case column:
		return "(N Col " + to_yices(*t.t) + ")";
		break;
	case scalar:
		return "scl";
		break;
	default:
		std::cout << "ERROR: compile.cpp: to_yices(): unexpected type\n";
	}
	return "";
}

void generate_constraints(graph &g) {
	std::ofstream out("con.yices");
	out << "(set-evidence! true)\n"
		<< "(define-type O (datatype Row Col))\n"
		<< "(define-type T (datatype scl (N orien::O t::T)))\n\n";
	
	enum o {a, s, t, m};
	std::set<int> ops;
	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		if (g.info(i).op == add)
			ops.insert(a);
		if (g.info(i).op == subtract)
			ops.insert(s);
		if (g.info(i).op == trans)
			ops.insert(t);
		if (g.info(i).op == multiply)
			ops.insert(m);
	}
	std::set<int>::iterator it = ops.begin();
	for (; it != ops.end(); it++) {
		switch (*it) {
		case a:
			/*out << "(define ADD::(-> T T T int bool) (lambda (U::T L::T R::T i::int)\n"
				<< "(ite (= i 0)\n" 
				<< "\ttrue\n"
				<< "\t(or\n" 
				<< "\t\t(and (scl? L) (scl? R) (scl? U))\n"
				<< "\t\t(and (N? L) (N? R) (N? U) (= (orien L) (orien R)) (= (orien L) (orien U))\n"
				<< "\t\t\t(ADD (t U) (t L) (t R) (- i 1)))\n"
				<< "\t))))\n\n";*/
			out << "(define ADD::(-> T T T bool) (lambda (U::T L::T R::T)\n"
				<< "\t(and (= L R) (= L U))\n"
				<< "))\n\n";
			break;
		case s:
			/*out << "(define SUBTRACT::(-> T T T int bool) (lambda (U::T L::T R::T i::int)\n"
				<< "(ite (= i 0)\n" 
				<< "\ttrue\n"
				<< "\t(or\n" 
				<< "\t\t(and (scl? L) (scl? R) (scl? U))\n"
				<< "\t\t(and (N? L) (N? R) (N? U) (= (orien L) (orien R)) (= (orien L) (orien U))\n"
				<< "\t\t\t(SUBTRACT (t U) (t L) (t R) (- i 1)))\n"
				<< "\t))))\n\n";*/
			out << "(define SUBTRACT::(-> T T T bool) (lambda (U::T L::T R::T)\n"
				<< "\t(and (= L R) (= L U))\n"
				<< "))\n\n";
			break;
			
		case t:
			out << "(define TRAN::(-> T T int bool) (lambda (U::T I::T i::int)\n"
				<< "\t(ite (= i 0)\n"
				<< "\t\ttrue\n"
				<< "\t\t(or\n"
				<< "\t\t\t(and (scl? U) (scl? I))\n"
				<< "\t\t\t(and (N? U) (N? I) (/= (orien U) (orien I))\n" 
				<< "\t\t\t\t(TRAN (t U) (t I) (- i 1)))\n"
				<< "\t\t))))\n\n";
			break;
		case m:
			out << "(define MULT::(-> T T T int bool) (lambda (U::T L::T R::T i::int)\n"
			<< "\t(ite (= i 0)\n"
			<< "\t\ttrue\n"
			<< "\t\t(or\n"
			<< "\t\t\t(and (scl? U) (scl? L) (scl? R)) ;;s*s\n"
			<< "\t\t\t(and (N? U) (N? L) (scl? R) (= (orien U) (orien L)) ;;n*s\n"
			<< "\t\t\t\t(MULT (t U) (t L) R (- i 1)))\n"
			<< "\t\t\t(and (N? U) (scl? L) (N? R) (= (orien U) (orien R)) ;;s*n\n"
			<< "\t\t\t\t(MULT (t U) L (t R) (- i 1)))\n"
			<< "\t\t\t(and (N? L) (N? R) (= (orien L) Row) (= (orien R) Col) ;;r*c\n"
			<< "\t\t\t\t(MULT U (t L) (t R) (- i 1)))\n"
			<< "\t\t\t(and (N? U) (N? L) (N? R) (= (orien L) Row) (= (orien R) Row) ;;r*r\n"
			<< "\t\t\t\t(= (orien U) (orien R)) (MULT (t U) L (t R) (- i 1)))\n"
			<< "\t\t\t(and (N? U) (N? L) (N? R) (= (orien L) Col) (= (orien R) Col) ;;c*c\n"
			<< "\t\t\t\t(= (orien U) (orien L)) (MULT (t U) (t L) R (- i 1)))\n"
			<< "\t\t\t(and (N? U) (N? L) (N? R) (= (orien L) Col) (= (orien R) Row) ;;c*r\n"
			<< "\t\t\t\t(= (orien U) (orien L)) (MULT (t U) (t L) R (- i 1)))\n"
			<< "\t\t\t(and (N? U) (N? L) (N? R) (= (orien L) Col) (= (orien R) Row) ;;c*r\n"
			<< "\t\t\t\t(= (orien U) (orien R)) (MULT (t U) L (t R) (- i 1)))\n"
			<< "\t\t))))\n\n";
			break;
		default:
			std::cout << "ERROR: compile.cpp: generate_constraints(): huh??\n";
		}
	}
	
	out << "(define OUT::(-> T T bool) (lambda (U::T I::T) (= U I)))\n\n";
	
	int rec_depth = 3;
	vector<vertex> cons;
	for (unsigned int i = 0; i != g.num_vertices(); i++) {
		out << "(define T" << i << "::T";
		switch (g.info(i).op) {
		case add:
			out << ")\n";
			out << "(define t" << i << "::bool (ADD T" << i << " T" << g.inv_adj(i)[0] 
			    << " T" << g.inv_adj(i)[1] << "))\n";
			cons.push_back(i);
			break;
		case subtract:
			out << ")\n";
			out << "(define t" << i << "::bool (SUBTRACT T" << i << " T" << g.inv_adj(i)[0] 
			    << " T" << g.inv_adj(i)[1] << "))\n";
			cons.push_back(i);
			break;
		case trans:
			out << ")\n";
			out << "(define t" << i << "::bool (TRAN T" << i << " T" << g.inv_adj(i)[0] 
			    << " " << rec_depth << "))\n";
			cons.push_back(i);
			break;
		case multiply:
			out << ")\n";
			out << "(define t" << i << "::bool (MULT T" << i << " T" << g.inv_adj(i)[0] 
			    << " T" << g.inv_adj(i)[1] << " " << rec_depth << "))\n";
			cons.push_back(i);
			break;
		case output:
			out << " " << to_yices(g.info(i).t) << ")\n";
			for (unsigned int v = 0; v != g.inv_adj(i).size(); v++) {
				out << "(define t" << i << "::bool (OUT T" << i << " T" << g.inv_adj(i)[v] << "))\n";
			}
			cons.push_back(i);
			break;
		case input:
			out << " " << to_yices(g.info(i).t) << ")\n";
			break;
		default:
			std::cout << "ERROR: compile.cpp: generate_constraints(): unexpected vertex operation\n";
		}
	}
	
	out << "\n(assert (and";
	for (unsigned int i = 0; i != cons.size(); i++) {
		out << " t" << cons[i];
	}
	out << "))\n"
		<< "(check)\n";
}

/////////////////////////////////////////// END YICES COMMAND LINE ///////////////////////////////////////////
