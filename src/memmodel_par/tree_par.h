#ifndef TREE_H
#define TREE_H

struct var{
  char *name;
  char **iterate; //The iterates that access a variable
  long long iterates;
  long long* misses;
  int summed; //1 if a variable has multiple values that must each be stored to be summed
};

struct node{
  long long its;  //iterations of a loop.  If this is a statement then it is 0
  struct node **children;
  long long numchildren;
  struct var **vars; //Variables used.  If a variable is used multiple times then it appears more than once
  long long variables;
  char *iterate; //What iterate moves through a loop
};

struct var* create_varP(char*, char**, long long, int);
struct node* create_stateP(struct var**, long long);
struct node* create_loopP(long long, struct node**, long long, char*);
void delete_treeP(struct node*);
void delete_nodeP(struct node*);
void delete_nodesP(struct node*);
void delete_varP(struct var*);
struct node* dot_to_tree(char*);

#endif
