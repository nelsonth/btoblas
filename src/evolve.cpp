#include "evolve.hpp"
#include <string>
#include <map>
#include <utility>
#include <set>
#include <algorithm>
#include <vector>
#include <iterator>

boost::mt19937 rng2;

/* OrganismData methods {{{ */

/* There's a bit of an issue with the same traits being checked in multiple places
 * connectivity is checked here in crossover, then again in buildVersion
 * Not sure exactly what to do about this.
 */

template <class GenomeType>
OrganismData<GenomeType>::OrganismData(int n, GenomeType* ptr, double fit, int b) {
	name = n;
	genome = ptr;
	fitresult = fit;
	birth = b;
	has_parents = false;
	parent1 = -1;
	parent2 = -1;
}

//note this means genome must be an allocated pointer (i.e. from new)
template <class GenomeType>
OrganismData<GenomeType>::~OrganismData() {
	//cout << "~ deleting genome from " << name << " ptr: " << genome << endl;
	delete genome;
}

int smallrand(int lo, int hi) {
	boost::uniform_smallint<> dist(lo,hi);
	boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > pchoice(rng2, dist);
	return pchoice();
}

template <class GenomeType>
OrganismData<GenomeType>::OrganismData(int n, GenomeType* ptr,
		double fit, int b, int p1, int p2) {
	name = n;
	genome = ptr;
	fitresult = fit;
	birth = b;
	has_parents = true;
	parent1 = p1;
	parent2 = p2;
}

/* }}} */

/* Main Genetic Algorithm {{{ */

template <class GenomeType>
bool lowerFitness(OrganismData<GenomeType>* a, OrganismData<GenomeType>* b) {
	return (a->fitresult < b->fitresult);
}

template <class GenomeType>
void logPop(vector< OrganismData<GenomeType>* >* organisms, ofstream* logHandle) {
	if (logHandle) {
		typename vector< OrganismData<GenomeType>* >::iterator i;
		(*logHandle) << "[";
		for (i = organisms->begin(); i != organisms->end(); ++i) {
			cout << "logging " << (*i)->name << endl;
			(*logHandle) << "[";
			(*logHandle) << (*i)->name << ",";
			(*logHandle) << (*i)->fitresult << ",";
			(*logHandle) << (*i)->birth << ",";
			(*logHandle) << (*i)->parent1 << ",";
			(*logHandle) << (*i)->parent2 << ",";
			(*logHandle) << "(";
			(*logHandle) << (*i)->genome->repr();
			(*logHandle) << ")";
			(*logHandle) << "],";
		}
		(*logHandle) << "]";
		(*logHandle) << endl;
		logHandle->flush();
	}
}

void NestedSets::datalog(int vid, double fitresult, ofstream *logHandle) {
	if (logHandle) {
		(*logHandle) << vid << "," << time(0) << ",";
		(*logHandle) << repr();
		(*logHandle) << fitresult << endl;
		logHandle->flush();
	}
}

template <class GenomeType>
void fitmapDebug(vector< OrganismData<GenomeType>* >* organisms, map<string, double> fittable) {
	typename vector< OrganismData<GenomeType>* >::iterator i;
	cout << "in fitmap" << endl;
	for (i = organisms->begin(); i != organisms->end(); ++i) {
		cout << "testing " << (*i)->name << endl;
		cout << (*i)->genome->repr() << endl;
//		cout << (*i)->parent1 << ",";
//		cout << (*i)->parent2 << endl;
		(*i)->fitresult = (*i)->genome->fitnessDebug((*i)->name, fittable);
	}
}

template <class GenomeType>
void fitmap(vector< OrganismData<GenomeType>* >* organisms, ofstream *csv) {
	typename vector< OrganismData<GenomeType>* >::iterator i;
	//cout << "in fitmap" << endl;
	for (i = organisms->begin(); i != organisms->end(); ++i) {
		cout << "testing " << (*i)->name << endl;
		cout << (*i)->genome->repr() << endl;
//		cout << (*i)->parent1 << ",";
//		cout << (*i)->parent2 << endl;
		(*i)->fitresult = (*i)->genome->fitness((*i)->name, csv);
	}
}

/*
 *
 */
template <class GenomeType>
OrganismData<GenomeType>* binarySelect(vector< OrganismData<GenomeType>* > *population) {
	boost::uniform_smallint<> space(0, population->size()-1);
	boost::variate_generator<boost::mt19937&, boost::uniform_smallint<> > choose(rng2, space);
	int i = choose();
	int j = choose();
	OrganismData<GenomeType> *a = (*population)[i];
	OrganismData<GenomeType> *b = (*population)[j];
	if (a->fitresult <= b->fitresult) {
		return a;
	} else {
		return b;
	}
}

template <class GenomeType>
OrganismData<GenomeType>* binaryTournamentPair(vector< OrganismData<GenomeType>* > *population,
	int id, int generation) {
	OrganismData<GenomeType> *a = binarySelect(population);
	OrganismData<GenomeType> *b = binarySelect(population);
	// cout << "binaryTournamentPair names: " << a->name << ", " << b-> name << endl;
	GenomeType *t = a->genome->crossover(b->genome);
	t->mutate();
	// cout << "child name: " << id << endl;
	OrganismData<GenomeType> *o = new OrganismData<GenomeType>(id, t,
		2.0e99, generation, a->name, b->name);
	return o;
}

template <class GenomeType>
void clearOrganisms(vector<OrganismData<GenomeType>* > *population) {
	//cout << "clearing organisms" << endl;
	typename vector<OrganismData<GenomeType>* >::iterator i;
	for (i = population->begin(); i != population->end(); ++i) {
		delete *i;
	}
}

template <class GenomeType>
void generation_tournament(
		vector< OrganismData<GenomeType>* > *population,
		vector<OrganismData<GenomeType>*> *children, int popsize,
		int currentGeneration, int start) {
	// cout << "starting generation tournament" << endl;
	for (int i = start; i < start+popsize; ++i) {
		children->push_back(binaryTournamentPair(population, i,
			currentGeneration));
	}
}

template <class GenomeType>
ResultInfo evolve(ofstream* logHandle,
		vector< OrganismData<GenomeType>* > population,
		int seconds, double growthRate, int popMax, double pop,int nameCount,	
		map<string, double> fittable, ofstream *csvhandle) {
	int generationCount = 1;
	int nextgensize = max(2, popMax);
	vector< OrganismData<GenomeType>* > children;
	typename vector< OrganismData<GenomeType>* >::iterator b =
		min_element(population.begin(), population.end(),
		lowerFitness<GenomeType>);
	OrganismData<GenomeType> *bestYet = *b;
	while (time(NULL) < seconds) {
		cout << "generation " << generationCount << endl;
		// turning off chaotic dynamics for now
		// pop = growthRate * pop * (1 - pop);
		// int nextgensize = max(2, static_cast<int>(pop * popMax));

		//first generate untested
		generation_tournament(&population, &children,
			nextgensize-1, generationCount, nameCount);
		fitmap(&children, csvhandle); //test the new organisms
		//fitmapDebug(&children, fittable);

		// pull the best from last generation and push it on the new generation
		cout << "best organism: " << bestYet->name << endl;
		population.erase(b);
		children.push_back(bestYet);
		clearOrganisms(&population);
		population = children;
		children = vector<OrganismData<GenomeType>*>();
		logPop(&population, logHandle);

		generationCount += 1;
		nameCount += nextgensize-1;

		b = min_element(population.begin(),population.end(),
			lowerFitness<GenomeType>);
		bestYet = *b;
	}
	cout << "Time ran out" << endl;
	if (logHandle) {
		logHandle->close();
	}
	if (csvhandle) {
		csvhandle->close();
	}
	OptPoint bestpt = bestYet->genome->getOptRep();
	ResultInfo r(nameCount,bestYet->name,bestpt);
	return r;
}

// this is the main function to run an experiment
/*
 * Note about parameters: growthRate controls population dynamics, roughly:
 * 1 <= growthRate <= 2 gives static population
 * 2 <= growthRate <= 3 gives oscillations converging towards stasis
 * 3 <= growthRate <= 4 periodic or chaotic behavior
 *
 * popMax is the maximum allowable population size, but the average will be around
 * popMax * (growthRate - 1) / growthRate
 */

template <class GenomeType>
ResultInfo newSolution(string filename, int seconds,
		double growthRate, int popMax, double popInit, 
		string tmpPath, bool trymax) {//, opt=max):

	// Sadly turning off the chaotic dynamics for now
	// popMax = static_cast<int>( popMax / (1 - 1/growthRate));
	// int gensize = max(2,static_cast<int>(popMax*popInit));

	int gensize = max(2,popMax);
	vector< OrganismData<GenomeType>* > organisms;
	for (int n=1; n<=gensize; ++n) {
		// default constructor = random constructor
		GenomeType* fresh = new GenomeType(trymax);
		OrganismData<GenomeType>* wrapped = new OrganismData<GenomeType>(n,
			fresh, 0, 0);
		organisms.push_back(wrapped);
	}
	ofstream logHandle;
	ofstream* logHandlePtr;
	if (filename != "") {
		logHandle.open(filename.c_str(), ios::out);
		logHandlePtr = &logHandle;
	} else {
		logHandlePtr = NULL;
	}
	//fitmap(&organisms);
	//fstream f("../btoresults/westmere_12core/dgemvt.11.16.11_18.20.18/exhaustive.csv");
	//if (f.fail()) {
		//cout << "Failed to open file" << endl;
		//exit(1);
	//}

	ofstream csvlogHandle;
	csvlogHandle.open((tmpPath+"datalog.csv").c_str());
	map<string, double> fittable;
	//while (!f.eof()) {
		//string s;
		//getline(f,s);
		////cout << "starting string: " << s << endl;
		//size_t found = s.find_last_of(',');
		//string val = s.substr(found+1);
		//double d;
		//istringstream i(val);
		//i >> d;
		//string key = s.substr(0,found+1);
		//size_t c = key.find_first_of(',');
		//c = key.find_first_of(',',c+1);
		//key = key.substr(c+1);
		////sscanf(val.c_str(),"%f",&d);
		////cout << "key: |" << key << "| val: " << d << endl;;
		//fittable[key] = d;
	//}
	
	//fitmapDebug(&organisms, fittable);
	fitmap(&organisms, &csvlogHandle);
	logPop(&organisms, logHandlePtr);
	return evolve<GenomeType>(logHandlePtr, organisms, seconds,
		growthRate, popMax, popInit, gensize+1, fittable, &csvlogHandle);
}
/* }}} */

/* Fuselevels methods {{{ */
Fuselevel::Fuselevel(vertex vert) : isBase(true), v(vert), sublevels(), 
	partition(false), active_part(false), w(m), blocksize(0) {}

Fuselevel::Fuselevel() : isBase(false), v(-1),
	sublevels(), partition(false), active_part(false), w(m), depth(-1) {} 

Fuselevel::Fuselevel(Fuselevel *f, bool deepcopy) {
	//cout << "copy constructor" << endl;
	//ostringstream t;
	//f->repr(t);
	//cout << "input: " << t.str() << endl;
	if (f->isBase) {
		isBase = true;
		v = f->v;
	} else {
		if (deepcopy) {
			vector<Fuselevel *>::iterator i;
			for (i = f->sublevels.begin(); i != f->sublevels.end(); ++i) {
				Fuselevel *x = new Fuselevel(*i, deepcopy);
				sublevels.push_back(x);
			}
		}
		isBase = false;
		depth = f->depth;
		if (f->partition) {
			partition = true;
			active_part = f->active_part;
			w = f->w;
			blocksize = f->blocksize;
		} else {
			partition = false;
		}
	}
	//ostringstream s;
	//repr(s);
	//cout << "constructor result " << s.str() << endl;
}

Fuselevel* singleVertex(OptSpace *space, vertex vert)  {
	//cout << "fuselevel constructor w/ vert = " << vert << endl;
	Fuselevel *base = new Fuselevel(vert);
	int max_fuse = space->op_nodes[vert].maxFuseDepth;
	//cout << "max_fuse = " << max_fuse << endl;
	for (int i=max_fuse; i > 0; --i) {
		Fuselevel *sub = new Fuselevel();
		sub->sublevels.push_back(base);
		base = sub;
		base->depth = i;
		//ostringstream s;
		//base->repr(s);
		//cout << i << s.str() << endl;
	}
	// cout << "MAX_PARTS = " << MAX_PARTS << endl;
	for (int i=MAX_PARTS-1; i >= 0; --i) {
		Fuselevel *sub = new Fuselevel();
		sub->sublevels.push_back(base);
		base = sub;
		base->partition = true;
		base->depth = i;
		base->active_part = true;
		base->blocksize = partition_bounds[i][1]; // setting to hi bound
		//ostringstream s;
		//base->repr(s);
		//cout << i << s.str() << endl;
	}
	return base;
}

void Fuselevel::getVertices(set<vertex> *s) {
	if (isBase) {
		s->insert(v);
	} else {
		vector<Fuselevel *>::iterator j;
		for (j = sublevels.begin(); j != sublevels.end(); ++j) {
			(*j)->getVertices(s);
		}
	}
}

void Fuselevel::repr(ostringstream &s) {
	if (isBase) {
		s << v;
	} else {
		s << "(";
		if (partition) {
			s << "_" << blocksize << "_";
		}
		vector<Fuselevel *>::iterator j;
		for (j = sublevels.begin(); j != sublevels.end(); ++j) {
			(*j)->repr(s);
			s << ",";
		}
		s << ")";
	}
}

// recursive function, returning vertices in sublevel
// Along the way populates the OptPoint data at each level
set<vertex> Fuselevel::gather_opt_point(graph &g, vector<partitionTree_t*>
		part_forest, OptPoint &pt, const OptSpace &space) {
	set<vertex> s;
	if (isBase) {
		//cout << "base case" << endl;
		s.insert(v);
		return s;
	} else {
		vector<Fuselevel *>::iterator i;
		for (i = sublevels.begin(); i != sublevels.end(); ++i) {
			set<vertex> r = (*i)->gather_opt_point(g,part_forest, pt, space);
			for (set<vertex>::iterator j = r.begin(); j != r.end(); ++j) {
				s.insert(*j);
			}
		}
		set<vertex>::iterator a;
		for (a = s.begin(); a != s.end(); ++a) {
			// num_parts & partition info
			if (partition && active_part) {
				pt.num_parts[*a]++;
				// way is fixed later
				//pt.partitions[*a][part_depth].way = w;
				pt.partitions[*a][depth].blocksize = blocksize;
			}
			// adjacency matrix: only incremented if non-partition or active partition
			if (!partition || active_part) {
				set<vertex>::iterator b;
				for (b = s.begin(); b != s.end(); ++b) {
					if (*a < *b) {
						pair<FuseEdge, bool> e = edge(*a, *b, pt.fsgr);
						pt.fsgr[e.first]++;
					}
				}
			}
		}
		// Here we have to adjust way, to make sure the set is legal
		if (partition) {
			set<vertex> t;
			for (set<vertex>::iterator i = s.begin(); i != s.end(); ++i) {
				//cout << "adding " << space.op_nodes[*i].id << endl;
				t.insert(space.op_nodes[*i].id);
			}
			vector<partitionChoices_t> choices = 
				queryFuseSet(g, part_forest, t, depth);
			if (w < static_cast<int>(choices.size())) {
				partitionChoices_t choice = choices[w];
				//cout << "found legal choice: w =" << w << ", size=" << choices.size() << endl;
				for (unsigned int ch = 0; ch < choice.iterators.size(); ++ch) {
					if (choice.iterators[ch] != NULL) {
						// need to convert from vertex # back to [0..]
						int k=0;
						while (space.op_nodes[k].id != ch) {
							++k;
						}
						Way way = choice.branch_paths[ch];
						//cout << "k = " << k << ", way = " << way << endl;
						pt.partitions[k][depth].way = way;
					}
				}
			} else {
				cout << "way points to illegal choice" << endl;
			}
		}
	}
   return s;
}

bool Fuselevel::contains(vertex u) {
	if (isBase) {
		return (u==v);
	} else {
		vector<Fuselevel *>::iterator j;
		for (j = sublevels.begin(); j!= sublevels.end(); ++j) {
			if ((*j)->contains(u)) {
				return true;
			}
		}
		return false;
	}
}

bool Fuselevel::fuse(Fuselevel *other) {
	sublevels.insert(sublevels.end(), 
		other->sublevels.begin(), other->sublevels.end());
	delete other;
	return true;
}

Fuselevel *findVertexAtDepth(int v, int depth, vector<Fuselevel*> levels) {
	while (true) {
		vector<Fuselevel *>::iterator it;
		for (it = levels.begin(); it != levels.end(); ++it) {
			if ((*it)->contains(v)) {
				break;
			}
		}
		if (depth == 0) {
			return *it;
		}
		--depth;
		levels = (*it)->sublevels;
	}
}

bool Fuselevel::overlap(set<vertex> s) {
	set<vertex> t;
	getVertices(&t);
	set<vertex>::iterator i;
	for (i = t.begin(); i != t.end(); ++i) {
		if (s.find(*i) != s.end()) {
			return true;
		}
	}
	return false;
}



/* }}} */

/* NestedSets methods {{{ */
graph *NestedSets::g;
OptSpace *NestedSets::space;
graph *NestedSets::baseGraph;
vector<partitionTree_t* > NestedSets::partitionForest;
string *NestedSets::routine_name;
string *NestedSets::tmpPath;
string *NestedSets::fileName;
bool NestedSets::noptr;
modelMsg *NestedSets::msg;
bool NestedSets::useCorrectness;
bool NestedSets::mpi;
vector< vector<bool> > NestedSets::legalparts;
map<vector<int>, double> NestedSets::fitness_cache;
vector<pair<int, int> > NestedSets::legalfuses;
compile_details_t *NestedSets::cDetails;
build_details_t *NestedSets::bDetails;

int bound(int x,int a,int b) {
	return max(a,min(b,x));
}

NestedSets::NestedSets(vector<Fuselevel *> vec) {
	fuses = vec;
}

// random constructor
NestedSets::NestedSets(bool trymax) {
	for (unsigned int v=0; v != space->op_nodes.size(); ++v) {
		Fuselevel *f = singleVertex(space, v);
		fuses.push_back(f);
	}
	if (trymax) {
		tryMaxFuse(fuses);
	}
	// cout << "initialization:" << repr() << endl;
	// debug();
	int mutate_reps = 3;
	for (int i = 0; i < mutate_reps; ++i) {
		mutate();
		// cout << "after mutation:" << endl;
		// debug();
		// cout << "\t" << repr() << endl;
	}
	// cout << "pt: " << repr() << endl;
}

/* Convert Fuselevel forest into adjacency matrix */
OptPoint NestedSets::getOptRep() {
	OptPoint pt(space->op_nodes.size());
	vector<Fuselevel *>::iterator i;
	for (i = fuses.begin(); i != fuses.end(); ++i) {
		(*i)->gather_opt_point(*g, partitionForest, pt, *space);
	}
	return pt;
}

double NestedSets::fitness(int vid, ofstream *csvlogHandle) {
	OptPoint pt = getOptRep();
	// pt.print(space->op_nodes);
	vector<int> ptkey;
	for (unsigned int i=0; i < space->search_dimension; ++i) {
		ptkey.push_back(pt.get(i));
	}
	map<vector<int>, double>::iterator it = fitness_cache.find(ptkey);
	if (it != fitness_cache.end()) {
		cout << "pulling fitness from cache: " << it->second << endl;
		return (it->second + 1e-8); // a slight punishment for not being original
	}
	
	map<vertex, vector<PartitionChoice> > partitions;
	//partitions = getPartitionMap();
	partitions = space->mapPartition(&pt);

    double cost = getPerformance(vid, *g, *baseGraph, partitions,
                                 pt.fsgr,  pt.num_parts, space->op_nodes, 
                                 false, *cDetails, *bDetails);
    
	if (cost < 0) {
		cout << "fail " << cost << endl;
		cost = 1e99;
	}
	datalog(vid, cost, csvlogHandle);
	fitness_cache[ptkey] = cost;
	return cost;
}

double NestedSets::fitnessDebug(int vid, map<string, double> fittable) {
	string s = repr();
	cout << "looking for |" << s << "|" << endl;
	map<string, double>::iterator i = fittable.find(s);
	if (i != fittable.end()) {
		return i->second;
	} else {
		cout << "Error, Not in table" << endl;
		exit(1);
		return -1;
	}
}

vector<set<vertex> >::iterator find_fuseset(vector<set<vertex> > &v, vertex r) {
	vector<set<vertex> >::iterator s;
	for (s = v.begin(); s != v.end(); ++s) {
		if (s->find(r) != s->end()) {
			return s;
		}
	}
	cout << "Error! " << r << " not found!" << endl;
	exit(1);
}

void NestedSets::tryMaxFuse(vector<Fuselevel *> &fs) {
	//cout << "tryMaxFuse" << endl;
	bool change = true;
	while (change) {
		change = false;
		vector<Fuselevel *>::iterator i;
		// f is the arbitrarily chosen starting fuseset
		Fuselevel *f = fs[0];
		i = fs.begin()+1;
		while (i != fs.end()) {
			if (f->isBase || (*i)->isBase) {
				return;
			}
			bool legal = tryFusePair(f,*i, space, g, baseGraph, partitionForest,
					 bDetails);
			if (legal) {
				change = true;
				fs.erase(i);
			} else {
				++i;
			}
		}
	}
	vector<Fuselevel *>::iterator sub;
	for (sub = fs.begin(); sub != fs.end(); ++sub) {
		if (!(*sub)->isBase) {
			tryMaxFuse((*sub)->sublevels);
		}
	}
}


// fusability test and set
// true = success, fuse happend
// false = no fuse possible
bool tryFusePair(Fuselevel *f1, Fuselevel *f2, OptSpace *space,
		graph *g, graph *baseGraph, vector<partitionTree_t *> partitionForest,
		build_details_t *bDetails) {
	//ostringstream stream1;
	//f1->repr(stream1);
	//ostringstream stream2;
	//f2->repr(stream2);
	//cout << "tryFusePair inputs:" << stream1.str() << " + " << stream2.str() << endl;
	if (f1->isBase || f2->isBase) {
		return false; //base case
	}
	// fuse, need to check legality here
	set<vertex> baseSet;
	f1->getVertices(&baseSet);
	set<vertex> combined;
	f2->getVertices(&combined);
	// test for legality
	if (!f1->partition) {
		set<vertex>::iterator u;
		for (u = baseSet.begin(); u != baseSet.end(); ++u) {
			combined.insert(*u);
		}
		bool legal = queryBaseFuse(combined, f1->depth, space, 
					baseGraph, bDetails);
		if (!legal) {
			// not legal to fuse
			return false;
		}
	} else { // partition case
		set<vertex>::iterator u;
		for (u = baseSet.begin(); u != baseSet.end(); ++u) {
			combined.insert(*u);
		}
		set<vertex> t;
		set<vertex>::iterator i;
		for (i = combined.begin(); i != combined.end(); ++i) {
			t.insert(space->op_nodes[*i].id);
		}
		vector<partitionChoices_t> choices = 
			queryFuseSet(*g, partitionForest, t, f1->depth);
		if (choices.empty()) {
			// no legal fuse
			return false;
		}
		// have to make sure new way is possible
		if (f1->w > static_cast<int>(choices.size() - 1)) {
			f1->w = choices.size()-1;
		}
	}
	f1->fuse(f2);
	//ostringstream stream3;
	//f1->repr(stream3);
	//cout << "result:" << stream3.str() << endl;
	return true;
}


/* Crossover {{{ */

void gatherSublevel(vector<Fuselevel *> *mainParent, 
			vector<Fuselevel *> *auxParent, vector<Fuselevel *> *output) {
	set<vertex> guardSet;
	vector<Fuselevel *>::iterator i;
	for (i = mainParent->begin(); i != mainParent->end(); ++i) {
		(*i)->getVertices(&guardSet);
	}
	// cout << "guardSet: ";
	// printSet(guardSet);
	// cout << endl;
	for (i = auxParent->begin(); i != auxParent->end(); ++i) {
		if ((*i)->isBase) {
			continue;
		}
		vector<Fuselevel *>::iterator j;
		for (j = (*i)->sublevels.begin(); j != (*i)->sublevels.end(); ++j) {
			if ((*j)->overlap(guardSet)) {
				output->push_back(*j);
			}
		}
	}
}

void sublevelMatch(Fuselevel *main, vector<Fuselevel *> *auxParent, vector<Fuselevel *> *output) {
	set<vertex> s;
	main->getVertices(&s);
	vector<Fuselevel *>::iterator j;
	for (j = auxParent->begin(); j != auxParent->end(); ++j) {
		set<vertex>::iterator si;
		for (si = s.begin(); si != s.end(); ++ si) {
			if ((*j)->contains(*si)) {
				output->push_back(*j);
				break;
			}
		}
	}
}

void sublevelFromParents(vector<Fuselevel *> *mainP, 
		vector<Fuselevel *> *auxP, vector<Fuselevel *> *output, 
		OptSpace *space, graph *g,
		graph *baseGraph, vector<partitionTree_t *> partitionForest, 
		build_details_t *bDetails) {

	vector<Fuselevel *>::iterator i;
	for (i = mainP->begin(); i != mainP->end(); ++i) {
		if ((*i)->isBase) {
			Fuselevel *child = new Fuselevel((*i)->v);
			// ostringstream s;
			// child->repr(s);
			// cout << "child: " << s.str() << endl;
			output->push_back(child);
		} else {
			Fuselevel *newChild = new Fuselevel(*i, false); // empty child
			vector<Fuselevel *> *subAuxP = &( (*i)->sublevels );
			vector<Fuselevel *> subMainP;
			gatherSublevel(subAuxP, auxP, &subMainP);
			sublevelFromParents(&subMainP, subAuxP, &(newChild->sublevels),
					space, g, baseGraph, partitionForest, bDetails);
			output->push_back(newChild);
			// The timing of the test needs to be considered
			// also how to handle failure
			// bool test = tryFusePair(child,newChild, space, g, baseGraph,
				  //partitionForest, bDetails);
		}
	}
}

// this recursive function builds the child from the parents
// Taking the easy way at first: only one parent
Fuselevel *childFromParents(Fuselevel *mainParent, vector<Fuselevel *> auxParent,
		OptSpace *space, graph *g,
		graph *baseGraph, vector<partitionTree_t *> partitionForest, 
		build_details_t *bDetails) {
	vector<Fuselevel *>::iterator a;
	// debug
	ostringstream stream1;
	mainParent->repr(stream1);
	cout << "entering childFromParents, parent=" << stream1.str() << endl;
	cout << "auxParents: ";
	for (a = auxParent.begin(); a != auxParent.end(); ++a) {
		ostringstream stream2;
		(*a)->repr(stream2);
		cout << stream2.str() << " ; ";
	}
	cout << endl;
	// gubed

	// base case
	if (mainParent->isBase) {
		Fuselevel *child = new Fuselevel(mainParent->v);
		ostringstream s;
		child->repr(s);
		cout << "child: " << s.str() << endl;;
		return child;
	}
	// multilevel
	Fuselevel *child = new Fuselevel(mainParent, false); // empty child
	// first element of sublevel

	// need each sublevel of each auxParent matching
	vector<Fuselevel *> auxSubParent;
	for (a = auxParent.begin(); a != auxParent.end(); ++a ) {
		sublevelMatch(mainParent->sublevels[0], &((*a)->sublevels), &auxSubParent);
	}

	cout << "recursive call for sublevels[0]" << endl;
	Fuselevel *newSub = childFromParents(mainParent->sublevels[0], auxSubParent,
			space, g, baseGraph, partitionForest, bDetails);
	child->sublevels.push_back(newSub);
	if (mainParent->sublevels.size() > 1) { // fusion
		vector<Fuselevel *>::iterator f;
		for (f = mainParent->sublevels.begin()+1; 
								f != mainParent->sublevels.end(); ++f) {
			cout << "recursive call for fuse loop" << endl;
			// need each sublevel of each auxParent matching
			vector<Fuselevel *>::iterator a;
			vector<Fuselevel *> auxSubParent;
			for (a = auxParent.begin(); a != auxParent.end(); ++a ) {
				sublevelMatch(*f, &((*a)->sublevels), &auxSubParent);
			}
			Fuselevel *newSub = childFromParents(*f, auxSubParent,
						space, g, baseGraph, partitionForest, bDetails);
			Fuselevel *newChild = new Fuselevel(mainParent, false); // empty child
			newChild->sublevels.push_back(newSub);
			bool test = tryFusePair(child,newChild, space, g, baseGraph,
				  partitionForest, bDetails);
			if (test) { // should always succeed for now
				cout << "win" << endl;
			} else {
				cout << "lose" << endl;
			}
		}
	}
	ostringstream s;
	child->repr(s);
	cout << "child: " << s.str() << endl;;
	return child;
}

// Class method
// input: other parent organism
// output: pointer to new child organism (on the heap)
NestedSets* NestedSets::crossover(NestedSets *other) {
	// cout << "parent1 = ";
	// debug();
	// cout << endl;
	// cout << "parent2 = ";
	// other->debug();
	// cout << endl;
	vector<Fuselevel *> vec;
	sublevelFromParents(&fuses, &other->fuses, &vec, space, g, baseGraph,
			partitionForest, bDetails);
//	vector<Fuselevel *>::iterator i;
//	for (i = fuses.begin(); i != fuses.end(); ++i) {
//		// find which vertices are in parent
//		vector<Fuselevel *> auxParent;
//		sublevelMatch(*i, &(other->fuses), &auxParent);
//		Fuselevel *f = childFromParents(*i, auxParent, space, g, baseGraph, partitionForest, bDetails);
//		vec.push_back(f);
//	}
	NestedSets *n = new NestedSets(vec);
	// cout << "crossover result ";
	// n->debug();
	// cout << endl;
	return n;
}

/* }}} */

/* Mutations {{{ */

void NestedSets::mutate_toggle_partition(int vert, int depth) {
	// cout << "toggle partition vert=" << vert << " depth=" << depth << endl;
	Fuselevel *f = findVertexAtDepth(vert, depth, fuses);
	f->active_part = !f->active_part;
}

void NestedSets::mutate_fuse(int r1, int r2) {
	// cout << "fuse r1=" << r1 << " r2=" << r2 << endl;
	vector<Fuselevel *>::iterator i1;
	vector<Fuselevel *>::iterator i2;
	for (i1 = fuses.begin(); i1 != fuses.end(); ++i1) {
		if ((*i1)->contains(r1)) {
			break;
		}
	}
	for (i2 = fuses.begin(); i2 != fuses.end(); ++i2) {
		if ((*i2)->contains(r2)) {
			break;
		}
	}
	if (i1!=i2) {
		bool fused = tryFusePair(*i1, *i2, space, g, baseGraph, 
						partitionForest, bDetails);
		if (fused) {
			fuses.erase(i2);
		}
		return;
	}
	Fuselevel *f = *i1;
	while (true) {
		for (i1 = f->sublevels.begin(); i1 != f->sublevels.end(); ++i1) {
			if ((*i1)->contains(r1)) {
				break;
			}
		}
		for (i2 = f->sublevels.begin(); i2 != f->sublevels.end(); ++i2) {
			if ((*i2)->contains(r2)) {
				break;
			}
		}
		if (i1!=i2) {
			bool fused = tryFusePair(*i1, *i2, space, g, baseGraph,
				partitionForest, bDetails);
			if (fused) {
				f->sublevels.erase(i2);
			}
			return;
		}
		f = *i1;
	}
}

/* unfuse a fused loop */
void NestedSets::mutate_unfuse(int r1) {
	// cout << "unfuse r="<< r1 << endl;

	// initialization: finding fuselevel
	vector<Fuselevel *>::iterator parentItr; // iterator pointing to parentItr
	for (parentItr = fuses.begin(); parentItr != fuses.end(); ++parentItr) {
		if ((*parentItr)->contains(r1)) {
			break;
		}
	}

	if ((*parentItr)->isBase) {
		return;  // no unfusion possible
	}

	// Because the top level data structure of NestedSets is 
	// vector<Fuselevel *> , we have to treat it slightly differently than the
	// Fuselevels in the while loop below
	if ((*parentItr)->sublevels.size() > 1) { // a fusion we can unfuse
		// every child in the sublevel gets a new parentItr
		Fuselevel *parent = *parentItr;
		fuses.erase(parentItr);
		vector<Fuselevel *>::iterator childItr;
		for (childItr = parent->sublevels.begin(); childItr != parent->sublevels.end(); ++childItr) {
			Fuselevel *newParent = new Fuselevel(parent, false);
			newParent->sublevels.push_back(*childItr);
			fuses.push_back(newParent);
		}
		delete parent;
		// now we can delete the original parentItr
		// this calls deconstructor (yes?)
		return;
	}
	vector<Fuselevel *>::iterator childItr;
	for (childItr = (*parentItr)->sublevels.begin(); childItr != (*parentItr)->sublevels.end(); ++childItr) {
		if ((*childItr)->contains(r1)) {
			break;
		}
	}

	// cout << "entering loop" << endl;
	while (true) {
		ostringstream s;
		(*childItr)->repr(s);
		// cout << "unfusing " << s.str() << endl;
		if ((*childItr)->isBase) {
			return;
		}
		if ((*childItr)->sublevels.size() > 1) { // a fusion we can unfuse
			// cout << "sublevels.size() > 1" << endl;
			// splitting
			Fuselevel *child = *childItr;
			vector<Fuselevel *>::iterator grandchildItr;
			(*parentItr)->sublevels.erase(childItr); // erases the pointer from vector
			for (grandchildItr = child->sublevels.begin(); grandchildItr != child->sublevels.end(); ++grandchildItr) {
				// ostringstream t;
				// (*grandchildItr)->repr(t);
				// cout << "grandchildItr " << t.str() << endl;
				Fuselevel *newChild = new Fuselevel(child,false);
				newChild->sublevels.push_back(*grandchildItr);
				(*parentItr)->sublevels.push_back(newChild);
			}
			delete child;
			return;
		}
		// not unfusing yet, so we descend another level
		vector<Fuselevel *>::iterator grandchildItr;
		for (grandchildItr = (*childItr)->sublevels.begin(); grandchildItr != (*childItr)->sublevels.end(); ++grandchildItr) {
			if ((*grandchildItr)->contains(r1)) {
				break;
			}
		}
		parentItr = childItr;
		childItr = grandchildItr;
	}
}

void NestedSets::mutate_blocksize(int vert, int depth, int delta) {
	// cout << "mutate blocksize v=" << vert << " depth=" << depth << " delta="
		 // << delta << endl;
	Fuselevel *f = findVertexAtDepth(vert, depth, fuses);
	f->blocksize += delta;
	if (f->blocksize < partition_bounds[depth][0]) {
		f->blocksize = partition_bounds[depth][0];
	}
	if (f->blocksize > partition_bounds[depth][1]) {
		f->blocksize = partition_bounds[depth][1];
	}
}

void NestedSets::mutate_way(int vert, int depth) {
	// cout << "mutate way v=" << vert << "depth=" << depth << endl;
	Fuselevel *f = findVertexAtDepth(vert, depth, fuses);
	set<vertex> s;
	f->getVertices(&s);
	set<vertex> t;
	for (set<vertex>::iterator i = s.begin(); i != s.end(); ++i) {
		t.insert(space->op_nodes[*i].id);
	}
	vector<partitionChoices_t> choices = 
		queryFuseSet(*g, partitionForest, t, depth);
	// cout << "choice size = " << choices.size();
	int r = smallrand(0,choices.size()-1);
	f->w = r;
}

void NestedSets::mutate() {
	double toggle_part_weight = 1.0;
	double fuse_weight = 1.0;
	double unfuse_weight = 1.0;
	double blocksize_weight = 1.0;
	double way_weight = 1.0;
	double nop_weight = 0.0;
	double s = toggle_part_weight + fuse_weight + unfuse_weight + blocksize_weight + way_weight + nop_weight;
	boost::uniform_real<> interval(0,s);
	boost::variate_generator<boost::mt19937&, boost::uniform_real<> >
		prob(rng2, interval);
	double p = prob();
	if (p < toggle_part_weight) {
		int r1 = smallrand(0,space->op_nodes.size()-1);
		int depth = smallrand(0, MAX_PARTS-1); // Fails when no partitioning?
		mutate_toggle_partition(r1, depth);
	} else {
	p -= toggle_part_weight;

	if (p < fuse_weight && legalfuses.size() > 0) {
		int i = smallrand(0,legalfuses.size()-1);
		int r1 = legalfuses[i].first;
		int r2 = legalfuses[i].second;
		if (smallrand(0,1)) {
			i = r1;
			r1 = r2;
			r2 = i;
		}
		mutate_fuse(r1, r2);
	} else {
	p -= fuse_weight;

	if (p < unfuse_weight) {
		int r1 = smallrand(0,space->op_nodes.size()-1);
		//int depth = smallrand(0, fuselevels.size() - 1);
		mutate_unfuse(r1);
	} else {
	p -= unfuse_weight;

	if (p < blocksize_weight) {
		int v = smallrand(0,space->op_nodes.size()-1);
		int depth = smallrand(0, MAX_PARTS-1); 
		int delta = smallrand(0,1);
		if (delta == 0) {delta = -1;}
		delta *= partition_bounds[depth][2]; // step
		mutate_blocksize(v, depth, delta);
	} else {
	p -= blocksize_weight;

	if (p < way_weight) {
		int v = smallrand(0,space->op_nodes.size()-1);
		int depth = smallrand(0, MAX_PARTS-1); 
		mutate_way(v, depth);
	}} }} }
	// DEBUG
	//cout << "== Mutation Result ==" << endl;
	//vector< vector<set<vertex> > >::iterator v;
	//for (v = fuselevels.begin(); v != fuselevels.end(); ++v) {
		//for (vector<set<vertex> >::iterator s = v->begin(); s != v->end(); ++s) {
			//printSet(*s);
		//}
		//cout << endl;
	//}
	//for (int a = 0; a < pt.size(); ++a) {
		//cout << pt.get(a) << ", ";
	//}
	//cout << endl;
}

/* }}} */

void NestedSets::debug() {
	vector<Fuselevel *>::iterator i;
	for (i = fuses.begin(); i != fuses.end(); ++i) {
		ostringstream s;
		(*i)->repr(s);
		cout << s.str();
	}
	cout << endl;
}

void fuseLevelDebug(vector<Fuselevel *> &fuses) {
	vector<Fuselevel *>::iterator i;
	for (i = fuses.begin(); i != fuses.end(); ++i) {
		ostringstream s;
		(*i)->repr(s);
		cout << s.str();
	}
	cout << endl;
}

string NestedSets::repr() {
	OptPoint pt = getOptRep();
	ostringstream s;
	for (unsigned int i = 0; i < space->search_dimension; ++i) {
		s << pt.get(i) << ",";
	}
	return s.str();
}

bool queryBaseFuse(const set<vertex> s, int depth, OptSpace *space, graph *baseGraph, build_details_t *bDetails) {
	//cout << "entering queryBaseFuse, depth = " << depth << endl;
	OptPoint pt(NestedSets::space->op_nodes.size());
	map<vertex, vector<PartitionChoice> > partitions;
	for (set<vertex>::iterator i = s.begin(); i != s.end(); ++i) {
		for (set<vertex>::iterator j = s.begin(); j != s.end(); ++j) {
			//cout << *i << "," << *j << endl;
			if (*i < *j) {
				pair<FuseEdge, bool> e = edge(*i, *j, pt.fsgr);
				//cout << "\tsetting" << *i << "," << *j << endl;
				pt.fsgr[e.first] = depth;
			}
		}
	}
	graph *toOptimize = new graph(*baseGraph);
	//int baseDepth = 0;
	partitions = NestedSets::space->mapPartition(&pt);
	//pt.print(space->op_nodes);
	int r = buildVersion(toOptimize, partitions,
		pt.fsgr, pt.num_parts, NestedSets::space->op_nodes, -1, false, *bDetails);
	if (r >= 0) {
		//cout << "legal fuse in queryBaseFuse" << endl;
		//printSet(s);
		return true;
	}
	//cout << "illegal fuse in queryBaseFuse" << endl;
	return false;
}

/* Entry Point:
 * Notice this instantiates the GA template functions with the NestedSets class
 */
ResultInfo genetic_algorithm_search(graph &g, graph &baseGraph,
	vector< partitionTree_t* > partitionForest,
	int seconds,int pop,bool trymax,
	compile_details_t &cDet, build_details_t &bDet) {
    
	NestedSets::partitionForest = partitionForest;
	NestedSets::space = new OptSpace(g);
	NestedSets::g = &g;
	NestedSets::baseGraph = &baseGraph;
	NestedSets::routine_name = &cDet.routine_name;
	NestedSets::tmpPath = &cDet.tmpPath;
	NestedSets::fileName = &cDet.fileName;
	NestedSets::noptr = cDet.noptr;
	NestedSets::msg = bDet.modelMessage;
	NestedSets::useCorrectness = cDet.runCorrectness;
	NestedSets::mpi = cDet.mpi;
	NestedSets::cDetails = &cDet;
	NestedSets::bDetails = &bDet;
	time_t start = time(NULL);
	seconds = start+seconds;
	// start = 1337794180;
	cout << "seed: " << start << endl;
	rng2.seed((unsigned int)start);

	// First we build a table of which Ways are legal
	OptPoint pt(NestedSets::space->op_nodes.size());
	map<vertex, vector<PartitionChoice> > partitions;
	for (unsigned int i = 0; i < pt.num_nodes ; ++i) {
		pt.num_parts[i] = 1; // should only need 1?
		vector<bool> legals;
		for (int j = 0 ; j <= 2; ++j) { // m,n,k
			pt.partitions[i][0].way = (Way)j; //only setting 1st partition
			partitions = NestedSets::space->mapPartition(&pt);
			graph *toOptimize = new graph(baseGraph);
			bool b = buildPartition(toOptimize, partitions, bDet);
			legals.push_back(b);
			delete toOptimize;
			//cout << "node " << i << " way " << j << ": " << b << endl;
		}
		pt.num_parts[i] = 0;
		pt.partitions[i][0].way = m;
		NestedSets::legalparts.push_back(legals);
	}
	// And now the fusions options
	for (unsigned int i = 0; i < pt.num_nodes; ++i) {
		for (unsigned int j=i+1; j < pt.num_nodes; ++j) {
			pair<FuseEdge, bool> e = edge(i, j, pt.fsgr);
			pt.fsgr[e.first] = 1;
			graph *toOptimize = new graph(baseGraph);
			//int baseDepth = 0;
			partitions = NestedSets::space->mapPartition(&pt);
			int r = buildVersion(toOptimize, partitions, 
				pt.fsgr, 
				pt.num_parts, 
				NestedSets::space->op_nodes,
				-1, 
				false, bDet);
			if (r >= 0) {
				//cout << "legal fuse: " << i << ", " << j << endl;
				NestedSets::legalfuses.push_back(make_pair(i,j));
			}
			pt.fsgr[e.first] = 0;
		}
	}
	// seconds, growthRate, popmax, popinit
	ResultInfo r = newSolution<NestedSets>(cDet.tmpPath+"log.txt",
		seconds, 3.6, pop, 0.375, cDet.tmpPath, trymax);
	// cout << "finished newSolution" << endl;
	// r.best_point.print(NestedSets::space->op_nodes);
	return r;
}

/* }}} */

/*{{{  ga + thread search */

int ga_thread(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
				 int seconds, bool maxfuse, int popsize,
                 compile_details_t &cDet,
                 build_details_t &bDet) {

	OptSpace space(g);

	cout << "Starting Genetic Algorithm" << endl;
	ResultInfo r = genetic_algorithm_search(g, baseGraph,
		part_forest, seconds, popsize, maxfuse, cDet, bDet);

	OptPoint bestpoint = r.best_point;
	int vid = r.version_count+1;
	int bestid = r.best_version;
	double besttime = 1e99;
	ofstream datalog;
	datalog.open((cDet.tmpPath+"datalog.csv").c_str(),fstream::app);
	cout << "Starting Final Thread Search" << endl;
	for (int depth = 0; depth < MAX_PARTS; ++depth) {
		for (int threadnum = partition_bounds[depth][0];
				threadnum <= partition_bounds[depth][1];
				threadnum += partition_bounds[depth][2]) {

			for (unsigned int v = 0; v < space.op_nodes.size(); ++v) {
				bestpoint.partitions[v][depth].blocksize = threadnum;
			}
			map<vertex, vector<PartitionChoice> > partitions;
			partitions = space.mapPartition(&bestpoint);
			double cost = getPerformance(vid, g, baseGraph, partitions, 
										 bestpoint.fsgr,  bestpoint.num_parts, 
										 space.op_nodes, false, cDet, bDet);
			datalog << vid << "," << time(0) << ",";
			
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				datalog << bestpoint.get(z) << ",";
			}
			datalog << cost << endl;
			if (cost < besttime) {
				bestid = vid;
        besttime = cost;
			}
			++vid;
		}
	}
	datalog.close();
	return bestid;
}

int ga_exthread(graph &g, graph &baseGraph, 
                 vector<partitionTree_t*> part_forest,
				 int seconds, bool maxfuse, int popsize,
                 compile_details_t &cDet,
                 build_details_t &bDet) {

	OptSpace space(g);

	cout << "Starting Genetic Algorithm" << endl;
	ResultInfo r = genetic_algorithm_search(g, baseGraph,
		part_forest, seconds, popsize, maxfuse, cDet, bDet);

	cout << "Found Genetic Point" << endl;
	OptPoint bestpoint = r.best_point;
	int vid = r.version_count+1;
	int bestid = r.best_version;
	double besttime = 1e99;
	ofstream datalog;
	datalog.open((cDet.tmpPath+"datalog.csv").c_str(),fstream::app);
	cout << "datalog open" << endl;
	map<vertex, vector<PartitionChoice> > partitions;
	for (int depth = 0; depth < MAX_PARTS; ++ depth) {
		// Initialize to low
		for (unsigned int v = 0; v < space.op_nodes.size(); ++v) {
				bestpoint.partitions[v][depth].blocksize =
					partition_bounds[depth][0];
		}
		// test point
		partitions = space.mapPartition(&bestpoint);
		double cost = getPerformance(vid, g, baseGraph, partitions, 
									 bestpoint.fsgr,  bestpoint.num_parts, 
									 space.op_nodes, false, cDet, bDet);
		if (cost > 0) {
			datalog << vid << "," << time(0) << ",";
			
			for (unsigned int z=0; z < space.search_dimension; ++z) {
				datalog << bestpoint.get(z) << ",";
			}
			datalog << cost << endl;
			if (cost < besttime) {
				bestid = vid;
			}
			++vid;
		}
		// end test
		unsigned int i = 0;
		while (i < space.op_nodes.size()) {
			if (bestpoint.partitions[i][depth].blocksize <
								partition_bounds[depth][1]) {
				bestpoint.partitions[i][depth].blocksize +=
					partition_bounds[depth][2];
				// test point
				partitions = space.mapPartition(&bestpoint);
				double cost = getPerformance(vid, g, baseGraph, partitions,
						 bestpoint.fsgr,  bestpoint.num_parts,
						 space.op_nodes, false, cDet, bDet);
				if (cost > 0) {
					datalog << vid << "," << time(0) << ",";
					
					for (unsigned int z=0; z < space.search_dimension; ++z) {
						datalog << bestpoint.get(z) << ",";
					}
					datalog << cost << endl;
					if (cost < besttime) {
						bestid = vid;
            besttime = cost;
					}
					++vid;
				}
				// end test
				i = 0;
			} else {
				bestpoint.partitions[i][depth].blocksize =
					partition_bounds[depth][0];
				++i;
			}
		}
	}
	datalog.close();
	return bestid;
}
/* }}} */
