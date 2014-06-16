#ifndef COST_PAR_H
#define COST_PAR_H

#include "parallel_machines.h"
#include "tree_par.h"
#include "memmodel_par.h"

double* parallel_costs(struct machine *, struct node *);
double cost_low(struct machine *, struct node *, int, double); 
double cost_high(struct machine *, struct node *, int, int); 
long long* sumvarsP(struct machine *, struct node *, int, int); 
double min_loop_costP(struct machine *, long long*, int, int*, struct machine**);

#endif
