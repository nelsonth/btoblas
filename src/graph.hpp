#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cassert>
#include <algorithm>
#include "boost/lexical_cast.hpp"

#include "iterator.hpp"

using std::vector;
using std::map;
using std::string;
using std::pair;

struct subgraph;

typedef std::pair<int,int> subgraph_id;

// interdependencies make local declaration of these necessary.
// they are located in partition.cpp
void cleanup_partition_t(void *);
void* copy_partition_t(void *);

	template<class T>
void remove(vector<T>& vec, T const& x)
{
	typename vector<T>::iterator i = std::find(vec.begin(), vec.end(), x);
	if (i != vec.end())
		vec.erase(i);
}

	template<class T>
void append(vector<T>& v1, vector<T> const& v2)
{
	v1.insert(v1.end(), v2.begin(), v2.end());
}

template<class V>
class Graph {
	public:
		typedef vector<unsigned int>::const_iterator adj_iter;
		typedef vector<unsigned int>::const_iterator inv_adj_iter;

		vector<unsigned int> const& adj(unsigned int i) const {
			assert(i < outs.size());
			return outs[i];
		}
		vector<unsigned int> const& inv_adj(unsigned int i) const { 
			assert(i < ins.size());
			return ins[i];
		}
		V& info(unsigned int i) { return _info[i]; }
		V const& info(unsigned int i) const { return _info[i]; }

		std::size_t num_vertices() const { return _info.size(); }

		int add_vertex(V const& v) {
			int n = _info.size();
			_info.push_back(v);
			ins.push_back(vector<unsigned int>());
			outs.push_back(vector<unsigned int>());
			return n;
		}
		void move_out_edges_to_new_vertex(unsigned int from, unsigned int to);
		void add_edge(unsigned int u, unsigned int v);
		void add_edge_no_dup(unsigned int u, unsigned int v);
		void add_left_operand(unsigned int u, unsigned int v);
		void remove_edges(unsigned int u, unsigned int v);
		void clear_vertex(unsigned int u);
		void move_vertex(subgraph *from, subgraph *to, unsigned int v);

		subgraph_id merge(subgraph* sg1, subgraph* sg2);
		int mergeable(unsigned int u, unsigned int v);
		int mergeable_majorID_at_minorID(subgraph *sg1, subgraph *sg2, int minor);

		void register_iter(string iter) {
			if (iter_rep.find(iter) == iter_rep.end())
				iter_rep[iter] = iter;
		}

		string get_iter_rep(string iter) {
			map<string,string>::iterator ret = iter_rep.find(iter);

			if (ret != iter_rep.end())
				return ret->second;
			return iter;
		}

		void merge_iterOps(vector<iterOp_t> &itr1, vector<iterOp_t> &itr2) {
			vector<iterOp_t>::iterator i,j;
			for (i=itr1.begin(); i != itr1.end(); ++i) {
				for (j=itr2.begin(); j != itr2.end(); ++j) {
					if (i->left.compare(j->left) == 0) {
						// if iteration variable's match, then merge the right
						// hand side.
						merge_iters(i->right,j->right);
					}
				}
			}
		}

		void merge_iters(string iter1, string iter2) {
			string r1 = iter_rep[iter1];
			string r2 = iter_rep[iter2];
			if (r1 != r2) {
				vector<string> keys;
				for (map<string,string>::iterator i = iter_rep.begin(); i != iter_rep.end(); ++i) {
					keys.push_back(i->first);
				}
				for (vector<string>::iterator i = keys.begin(); i != keys.end(); ++i) {
					if (iter_rep[*i] == r2)
						iter_rep[*i] = r1;
				}
			}
		}

		bool is_equal(string iter1, string iter2) {
			register_iter(iter1);
			register_iter(iter2);
			return iter_rep[iter1] == iter_rep[iter2];
		}



		Graph() { partitionInformation = NULL;}

		Graph(const Graph<V>& g);

		// destructor
		~Graph() {
			for (unsigned int i = 0; i != subgraphs.size(); i++) {
				delete subgraphs[i];
			}

			if (partitionInformation) {
				cleanup_partition_t(partitionInformation);
				partitionInformation = NULL;
			}
		}

		subgraph* find_parent(unsigned int v);

		void * get_partition_information() {
			return partitionInformation;
		}

		void set_partition_information(void *pI) {
			partitionInformation = pI;
		}

		void add_partition_id(unsigned int u, string id) {
			partitionIds[u].insert(partitionIds[u].begin(),id);
		}

		vector<string>* get_partition_ids(unsigned int u) {
			if (partitionIds.find(u) != partitionIds.end())
				return &(partitionIds[u]);
			else return NULL;
		}

	private:
		vector< V > _info;
		vector< vector<unsigned int> > outs;
		vector< vector<unsigned int> > ins;
		map<string,string> iter_rep; // representative for iterations (key, represntative)

		void *partitionInformation;		// NULL when unpartitioned
		// partition_t struct when partitioned.
		// map partition id's to vertex in the order they are introduced
		map<unsigned int, vector<string> >partitionIds;
	public:
		vector<subgraph*> subgraphs;
};

//extern int subgraphUniqueID;
struct subgraph {

	template<class V>
		subgraph(std::string n, subgraph* p, Graph<V>& g, 
				sg_iterator_t &sg_iterator_in, int v)
		: name(n), parent(p), sg_iterator(sg_iterator_in) {
			if (p == 0) {
				g.subgraphs.push_back(this);
				setMajorID(v);
				setMinorID(1);
			} else {
				p->subs.push_back(this);
				setMajorID(p->getMajorID());
				setMinorID(p->getMinorID()+1);
			}
			str_id = boost::lexical_cast<string>(getMajorID()) + "_" + boost::lexical_cast<string>(getMinorID());
			vector<iterOp_t>::iterator i;
			for (i = sg_iterator.conditions.begin(); i != sg_iterator.conditions.end(); ++i)
				g.register_iter(i->right);
			//cout << "making subgraph " << str_id << endl;
		}

	subgraph(const subgraph& g, subgraph* p = 0)
		: name(g.name), parent(p), sg_iterator(g.sg_iterator), str_id(g.str_id),
		uid(g.uid), vertices(g.vertices)
	{
		for (unsigned int i = 0; i != g.subs.size(); ++i)
			subs.push_back(new subgraph(*g.subs[i], this));
		//cout << "making subgraph " << str_id << endl;
	}

	subgraph* find_parent(unsigned int u)
	{

		vector<unsigned int>::iterator i = find(vertices.begin(), vertices.end(), u);
		if (i != vertices.end()) {
			return this;
		}

		for (unsigned int i = 0; i != subs.size(); ++i) {
			subgraph* p = subs[i]->find_parent(u);
			if (p) return p;
		}

		return 0;
	}

	int depth() {
		if (parent == 0) 
			return 1;
		else 
			return 1 + parent->depth();
	}

	// destructor
	~subgraph() {
		for (auto i : subs) {
			delete i;
		}
	}

	int getMajorID() {
		return uid.first;
	}

	int getMinorID() {
		return uid.second;
	}

	void setMajorID(int a) {
		uid.first = a;
	}

	void setMinorID(int a) {
		uid.second = a;
	}

	std::string name;
	subgraph* parent;
	sg_iterator_t sg_iterator;
	std::string str_id;					// id string
	pair<int,int> uid;					// majorID, minorID
	vector<unsigned int> vertices;
	vector<subgraph*> subs;
};

	template<class V>
subgraph* Graph<V>::find_parent(unsigned int u)
{
	for (unsigned int i = 0; i != subgraphs.size(); ++i) {
		subgraph* p = subgraphs[i]->find_parent(u);
		if (p) return p;
	}
	return 0;
}

template<class V>
void Graph<V>::move_vertex(subgraph *from, subgraph *to, unsigned int v) {
	// move vertex from one subgraph to the other
	if (from) {
		remove(from->vertices,v);
	}
	if (to) {
		to->vertices.push_back(v);
	}
}

	template<class V>
void Graph<V>::clear_vertex(unsigned int u)
{
	info(u).algo = 0;
	info(u).op = deleted;

	vector<unsigned int> preds(inv_adj(u));
	for (unsigned int i = 0; i != preds.size(); ++i)
		remove_edges(preds[i], u);
	vector<unsigned int> succs(adj(u));
	for (unsigned int i = 0; i != succs.size(); ++i)
		remove_edges(u, succs[i]);
	subgraph* p = this->find_parent(u);
	if (p) {
		remove(p->vertices, u);
	}
}

// Put everything in sg1 into sg2
	template<class V>
subgraph_id Graph<V>::merge(subgraph* sg1, subgraph* sg2)
{
	// this will need to be updated to support more than
	// general matrices
	if (sg1->sg_iterator.conditions.size() == 0 ||
			sg2->sg_iterator.conditions.size() == 0)
		return std::pair<int,int>(-1,-1);

	string sg1Iterations = sg1->sg_iterator.conditions[0].right;
	string sg2Iterations = sg2->sg_iterator.conditions[0].right;

	if (sg1 != sg2
			&& sg1->parent == sg2->parent
			&& is_equal(sg1Iterations, sg2Iterations)) {

#if 0
		// Change the vertex parent pointers
		for (unsigned int i = 0; i != sg1->vertices.size(); ++i)
			info(sg1->vertices[i]).parent = sg2;
#endif

		// Change the subgraph parent pointers
		if (sg1->getMajorID() < sg2->getMajorID()) {
			subgraph* temp;
			temp = sg2;
			sg2 = sg1;
			sg1 = temp;
		}
		for (unsigned int i = 0; i != sg1->subs.size(); ++i)
			sg1->subs[i]->parent = sg2;

		append(sg2->vertices, sg1->vertices);
		append(sg2->subs, sg1->subs);

		vector<subgraph*>* parent_subs;
		if (sg1->parent == 0)
			parent_subs = &subgraphs;
		else
			parent_subs = &(sg1->parent->subs);
		remove(*parent_subs, sg1);

		//delete sg1;
		return sg2->uid;
	} else
		return std::pair<int,int>(-1,-1);
}

template<class V>
int Graph<V>::mergeable(unsigned int u, unsigned int v) {
	// return
	// 0 not mergeable
	// 0x1 mergeable
	// 0x2 mergeable except iterations are $$x and $$y
	// 0x4 mergeable except steps are $$x and $$y
	subgraph* sg1 = 0, *sg2 = 0;
	sg1 = find_parent(u);
	sg2 = find_parent(v);
	if (!sg1 || !sg2 || sg1 == sg2 || sg1->parent != sg2->parent)
		return 0;

	int ret = 0;

	// this will need to be updated to support more than
	// general matrices
	if (sg1->sg_iterator.conditions.size() == 0 ||
			sg2->sg_iterator.conditions.size() == 0 ||
			sg1->sg_iterator.updates.size() == 0 ||
			sg2->sg_iterator.updates.size() == 0 )
		return 0;

	string sg1Iterations = sg1->sg_iterator.conditions[0].right;
	string sg2Iterations = sg2->sg_iterator.conditions[0].right;
	string sg1Step = sg1->sg_iterator.updates[0].right;
	string sg2Step = sg2->sg_iterator.updates[0].right;

	if (!is_equal(sg1Iterations, sg2Iterations)) {
		if (sg1Iterations.find("$$") == 0 && sg2Iterations.find("$$") == 0)
			ret |= 0x2;
	}
	else {
		ret |= 0x1;
	}

	if (!is_equal(sg1Step, sg2Step)) {
		if (sg1Step.find("$$") == 0 && sg2Step.find("$$") == 0)
			ret |= 0x4;
	}
	else {
		ret |= 0x1;
	}

	return ret;
}

template<class V>
int Graph<V>::mergeable_majorID_at_minorID(subgraph *sg1, subgraph *sg2, int minor) {
	/*
	   do not require parent(u) == parent(v)
	   imagine
	   d
	   / \
	   sg0(nop)   sg1(nop)
	   /	   \
	   sg2(get)		sg3(get)

	   we would like to be able to merge sg0 and sg1, but mergeable below
	   would say this is not a legal merge.

	   return
	   0 not mergeable
	   0x1 mergeable
	   0x2 mergeable except iterations are $$x and $$y
	   0x4 mergeable except steps are $$x and $$y
	   */

	if (sg1 == NULL || sg2 == NULL || sg1 == sg2 || sg1->uid.first == sg2->uid.first
			|| sg1->uid.second != sg2->uid.second)
		return 0;

	if (sg1->uid.second < minor)
		return 0;

	while (sg1->uid.second > minor)
		sg1 = sg1->parent;

	while (sg2->uid.second > minor)
		sg2 = sg2->parent;

	if (sg1 == NULL || sg2 == NULL)
		return 0;

	// this will need to be updated to support more than
	// general matrices
	if (sg1->sg_iterator.conditions.size() == 0 ||
			sg2->sg_iterator.conditions.size() == 0 ||
			sg1->sg_iterator.updates.size() == 0 ||
			sg2->sg_iterator.updates.size() == 0 )
		return 0;

	string sg1Iterations = sg1->sg_iterator.conditions[0].right;
	string sg2Iterations = sg2->sg_iterator.conditions[0].right;
	string sg1Step = sg1->sg_iterator.updates[0].right;
	string sg2Step = sg2->sg_iterator.updates[0].right;

	int ret = 0;
	if (!is_equal(sg1Iterations, sg2Iterations)) {
		if (sg1Iterations.find("$$") == 0 && sg2Iterations.find("$$") == 0)
			ret |= 0x2;
	}
	else {
		ret |= 0x1;
	}

	if (!is_equal(sg1Step, sg2Step)) {
		if (sg1Step.find("$$") == 0 && sg2Step.find("$$") == 0)
			ret |= 0x4;
	}
	else {
		ret |= 0x1;
	}

	return ret;
}


template<class V>
void Graph<V>::move_out_edges_to_new_vertex(unsigned int from, unsigned int to) {
	// if vertex 'from' has all of the out edges that 'to' needs then
	// this function will update the edge data structures while
	// preserving the operand ordering created by the old egdges.
	// THIS IS GOING TO LEAVE vertex 'from' with no out edges.

	assert(from < num_vertices());
	assert(to < num_vertices());

	// outs order does not matter, however ins order controls
	// operand order

	vector<unsigned int>::iterator i = outs[from].begin();
	vector<unsigned int>::iterator j;
	for (; i != outs[from].end(); ++i) {
		j = std::find(ins[*i].begin(),ins[*i].end(),from);

		assert(j != ins[*i].end());

		*j = to;

		//// with this check in place operations with both operands
		//// the same break..
		//j = std::find(outs[to].begin(), outs[to].end(),*i);
		//if (j == outs[to].end())
		outs[to].push_back(*i);
	}

	//outs[to] = outs[from];
	outs[from].clear();
}

template<class V>
void Graph<V>::add_edge(unsigned int u, unsigned int v) {
	assert(u < num_vertices());
	assert(v < num_vertices());
	outs[u].push_back(v);
	ins[v].push_back(u);
}

template<class V>
void Graph<V>::add_edge_no_dup(unsigned int u, unsigned int v) {
	// add edge, but disallow duplicate edge
	assert(u < num_vertices());
	assert(v < num_vertices());
	for (unsigned int i = 0; i < outs[u].size(); ++i) {
		if (outs[u][i] == v)
			return;
	}
	outs[u].push_back(v);
	ins[v].push_back(u);
}

template<class V>
void Graph<V>::add_left_operand(unsigned int u, unsigned int v) {
	assert(u < num_vertices());
	assert(v < num_vertices());
	outs[u].insert(outs[u].begin(),v);
	ins[v].insert(ins[v].begin(),u);
}

template<class V>
void Graph<V>::remove_edges(unsigned int u, unsigned int v) {
	assert(u < num_vertices());
	assert(v < num_vertices());
	outs[u].erase(std::remove(outs[u].begin(), outs[u].end(), v), outs[u].end());
	ins[v].erase(std::remove(ins[v].begin(), ins[v].end(), u), ins[v].end());
}

	template <class V>
	Graph<V>::Graph(const Graph<V>& g)
: _info(g._info), outs(g.outs), ins(g.ins), iter_rep(g.iter_rep)
{
	for (unsigned int i = 0; i != g.subgraphs.size(); ++i)
		subgraphs.push_back(new subgraph(*g.subgraphs[i]));

	partitionInformation = copy_partition_t(g.partitionInformation);
}


#endif // GRAPH_HPP
