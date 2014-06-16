#ifndef EVOLVE_HPP 
#define EVOLVE_HPP 

#include <math.h>
#include <sstream>
#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/uniform_smallint.hpp>
//#include <boost/graph/transitive_closure.hpp>
#include "opt_decision.hpp"

using namespace std;

struct ResultInfo {
	public:
		int version_count;
		int best_version;
		OptPoint best_point;
		ResultInfo(int n, int b, OptPoint p)
			: version_count(n), best_version(b), best_point(p) {}
};

class Fuselevel {
	public:
		bool isBase; // true if no sublevels.
		vertex v;
		vector<Fuselevel*> sublevels;
		bool partition;
		// these only active if partition is true
		bool active_part;
		int w;
		int blocksize;
		int depth;
		Fuselevel(vertex v); // constructor for base
		Fuselevel();  // Sublevel constructor
		Fuselevel(Fuselevel *f, bool deepcopy); // copy constructor
		void getVertices(set<vertex> *s);
		void toAdjMat(FuseGraph &fsgr);
		bool contains(vertex u);
		bool overlap(set<vertex> s);
		void repr(ostringstream &s);
		bool fuse(Fuselevel *other);
		//void randFuse(graph &g, vector<partitionTree_t*> part_forest,
			//const OptSpace &space);
		set<vertex> gather_opt_point(graph &g, vector<partitionTree_t*>
			part_forest, OptPoint &pt, const OptSpace &space);
};
bool tryFusePair(Fuselevel *f1, Fuselevel *f2, OptSpace *space, graph *g, graph *baseGraph, vector<partitionTree_t* > partitionForest, build_details_t *bDet);

Fuselevel* singleVertex(OptSpace *space, vertex vert); 
bool queryBaseFuse(set<vertex> s, int depth, OptSpace *space, graph *baseGraph, 
		build_details_t *bDetails);

/*
 * NestedSets is a particular instance of an organism, meant to be used in
 * OrganismData.  It has all these static arguments as a way to keep from
 * passing them around all over the place for fitness.  Still waiting for a
 * longer-term solution to that issue
 */
class NestedSets {
	public:
		static OptSpace *space;
		static graph *g;
		static graph *baseGraph;
		static vector<partitionTree_t* > partitionForest;
		static string *routine_name;
		static string *tmpPath;
		static string *fileName;
		static compile_details_t *cDetails;
		static build_details_t *bDetails;
		static vector< vector<bool> > legalparts;
		static vector<pair<int, int> > legalfuses;
		static bool noptr;
		static modelMsg *msg;
		static bool useCorrectness;
		static bool mpi;
		double fitness(int, ofstream *csvlogHandle); // returns fitness
		double fitnessDebug(int vid, map<string, double> fittable);
		NestedSets(bool trymax); // random constructor
		NestedSets(vector<Fuselevel *> v); // copy constructor
		void mutate(); //mutated version
		NestedSets* crossover(NestedSets *other);
		string repr();
		void debug();
		void datalog(int vid, double fitresult, ofstream *logHandle);
		void tryMaxFuse(vector<Fuselevel *> &fs);
		OptPoint getOptRep();
	private:
		static map<vector<int>, double> fitness_cache;
		void mutate_fuse(int r1, int r2);
		void mutate_unfuse(int r1);
		void mutate_toggle_partition(int r1, int depth);
		void mutate_blocksize(int vert, int depth, int delta);
		void mutate_way(int vert, int depth);
		vector<Fuselevel *> fuses;
};


/*
 * This is a templated class holding all the metadata associated with a particular
 * organism during a GA run.  It has fitness, parent information, etc.
 * It contains a pointer to a GenomeType, which is that actual type being
 * searched over. That will be deallocated on delete, so it must be on the heap.
 */
template <class GenomeType>
class OrganismData {
	public:
	int name;
	double fitresult;
	int birth;
	bool has_parents;
	int parent1;
	int parent2;
	GenomeType *genome;
	OrganismData(int n, GenomeType* ptr, double fit, int b);
	OrganismData(int n, GenomeType* ptr, double fit, int b, int p1, int p2);
	~OrganismData();
};

ResultInfo genetic_algorithm_search(graph &g, graph &baseGraph,
	vector< partitionTree_t* > partitionForest,
	int seconds, 
	int popsize, bool trymax, compile_details_t &cDet,
	build_details_t &bDet);

void fuseLevelDebug(vector<Fuselevel *> &fuses);

int ga_thread(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
				 int seconds, bool maxfuse, int popsize,
                 compile_details_t &cDet,
                 build_details_t &bDet);

int ga_exthread(graph &g, graph &baseGraph,
                 vector<partitionTree_t*> part_forest,
				 int seconds, bool maxfuse, int popsize,
                 compile_details_t &cDet,
                 build_details_t &bDet);
#endif
