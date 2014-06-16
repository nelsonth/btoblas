
#ifndef COST_ESTIMATE_COMMON_HPP_
#define COST_ESTIMATE_COMMON_HPP_

#include "syntax.hpp"
#include "modelInfo.hpp"
#include <list>

using std::list;

static string iterators = "abcdefghijklmnopqrstuvwxyz";


// when searching for iterators, operands need to search inv_adj(up) 
// and results need to search adj (down)
enum dir {up, down};

//////////////////// Models ///////////////////////////////////////////

// serial analytic model
bool evalGraphAS(graph &g, int vid, string root, modelMsg &mMsg, 
				 versionData *verData);
bool insertOrderedAS(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData);



// parallel analytic model
bool evalGraphAP(graph &g, int vid, string root, modelMsg &mMsg, 
				 versionData *verData);
bool insertOrderedAP(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData);


// symbolic model
bool evalGraphSYM(graph &g, int vid, string root, modelMsg &mMsg, 
				 versionData *verData);
bool insertOrderedSYM(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData);


// temporary count
bool evalGraphTC(graph &g, int vid, string root, modelMsg &mMsg, 
				 versionData *verData);
bool insertOrderedTC(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData);




//struct node *build_tree(graph &g);

/////////////////// PRINT UTILS /////////////////////////////////////
void print_tree(struct node *tree, int version);
void print_node(struct node* n, int &id, std::ofstream &out);
void print_var(struct var *v, int &id, std::ofstream &out);

#endif

