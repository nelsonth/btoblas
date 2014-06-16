#ifndef MEMMODEL_H
#define MEMMODEL_H

#include "parallel_machines.h"
#include "tree_par.h"

#define wordsize 8

long long* all_missesP(struct machine*, struct node*); //clean
long long mem_misses_structureP(struct node*, long long, long long, int, struct node*, int); //clean
long long calc_working_setP(struct node*, int); //clean
long long calc_reuse_distanceP(struct node*, int, int);  //clean
struct var** find_distinct_varsP(struct node*, int *); //clean
long long calc_var_sizeP(struct node*, struct var*, int); //clean
int var_used_onceP(struct node*, int); //clean
int var_iterated_overP(struct node*, int);  //clean
int var_always_iterated_overP(struct node*, int); //clean
int first_accessP(struct node*, int); //clean
int find_last_accessP(struct node*, int); //clean
int find_previous_accessP(struct node*, int); //clean
int find_next_accessP(struct node*, int); //clean
int childP(struct node*, int); //clean
int child_locationP(struct node*, int); //clean
long long after_varP(struct node*, int, char**, long long*, int, int); //buggy
long long before_varP(struct node*, int, int); //clean
long long calc_working_set_plus_itsP(struct node*, char**, long long*, int, int); //clean
long long calc_var_offitsP(struct var*, char**, long long*, int, int); //clean
int in_different_subloopP(struct node*, int, int); //clean
long long reuse_other_loopsP(struct node*, int, int); //clean
int used_in_sameP(struct node*, int); //works for what I tested it on but may not work in all cases I'm skeptical of this function
void find_smallestP(long long, struct node*, int, long long*);
int iterated_aboveP(struct node*, long long, char **);

#endif
