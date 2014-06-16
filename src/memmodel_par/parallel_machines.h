#ifndef PARALLEL_MACHINES_H
#define PARALLEL_MACHINES_H

#include "tree_par.h"

struct machine{
  char *name;
  long long numcaches;
  struct cache **caches;
  struct machine *sibling;
  struct machine *children;
  int compressed; //if not zero then this structure represent x compressed siblings that are identical
  long long cycles; //clock speed in hz
  int flops; //number of floating point operations capable of being performed per cycle
  int cores; //number of cores at a given level
  int threads; //number of threads at a given level
  int sockets; //0 if socket level is higher up.  1 if this is the socket level. x for the number of sockets below this level.
};

struct cache{
  char* name;
  long long size; //in bytes
  long long linesize; //in bytes
  long long associativity;
  long long lines;
  long long bandwidth; //in bytes per second from next larger structure to this one
  long long latency; //in clock cycles for some quantities this may need to be approximate this is not used yet
};

struct machine* new_machine(int, char**, long long*, int, int, int, char*);
struct machine* add_sibling(struct machine*, int, char**, long long*, int, int, int);
struct machine* add_child(struct machine*, int, char**, long long*, int, int, int);
struct cache* create_cacheP(long long, long long, long long, char*, long long);
void compress_machine(struct machine*);
void perculate_up(struct machine*, int*);
int cache_equal(struct cache*, struct cache*);
int children_equal(struct machine*, struct machine*);
int count_caches(struct machine*);
int buses_to_me(struct machine*, struct machine*, int);
struct machine* par_to_serial(struct machine*);
void print_missesP(struct machine*, struct node*);
void print_machine(struct machine*);
void print_cache(struct cache*);
/*struct machine* create_clovertown();
struct machine* create_quadfatherP();
struct machine* create_i7();
struct machine* create_opteron();
struct machine* create_opteron_lowP();
struct machine* create_opteron_midP();
struct machine* create_northwood();*/
void delete_machineP(struct machine*);
void delete_cacheP(struct cache*);

#endif
