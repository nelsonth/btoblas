#include "translate_to_code.hpp"
#include "partition.hpp" // for check_depth
#include "iterator.hpp"

using namespace std;
extern std::string precision_type;




void makePartMap(subgraph *sg, map<string,partitionInfo> &partMap, int threadDepth, int curDepth) {
	// recursively map partition sizing information
	if (curDepth > threadDepth)
		return;
	
	for (int i = 0; i < sg->subs.size(); ++i) {
		subgraph *s = sg->subs[i];
		if (s->sg_iterator.step.find("$$") == 0) {
			string tmp = s->sg_iterator.step;
			tmp.erase(0,2);
			tmp = "__s" + tmp;
			string tmp2 = s->sg_iterator.iterations;
			if (tmp2.find("$$") == 0) {
				tmp2.erase(0,2);
				tmp2 = "__s" + tmp2;
			}
			partMap[tmp] = partitionInfo(tmp2,s);
		}
		
		makePartMap(s,partMap,threadDepth,curDepth+1);
	}
}

string generatePartitionSizes(graph &g, map<subgraph*,string> &subgraphTonparts, int threadDepth, string name, bool mpi) {
	// generate code for dividing the problem
	
	// at the top level there may be many separate parallel loops
	// each with their own size.  size we are determining that size
	// here, we need to return a map from top level subgraphs to
	// the number of threads they have.  This is important because each
	// dispatch loop must be able to wait for the correct number of threads
	// and perform reductions correctly.
	// NOTE: this will need to be revisited when multidimension support is
	// in place. 
	
	stringstream out("");
	
	map<string, partitionInfo> partMap;
	// first find all abstract partition sizes
	for (int i = 0; i < g.subgraphs.size(); ++i) {
		subgraph *sg = g.subgraphs[i];
		if (sg->sg_iterator.step.find("$$") == 0) {
			string tmp = string(sg->sg_iterator.step);
			tmp.erase(0,2);
			tmp = "__s" + tmp;
			string tmp2 = string(sg->sg_iterator.iterations);
			if (tmp2.find("$$") == 0) {
				tmp2.erase(0,2);
				tmp2 = "__s" + tmp2;
			}
			partMap[tmp] = partitionInfo(tmp2,sg);
		}
		
		makePartMap(sg,partMap,threadDepth,2);
	}
	
	map<string,partitionInfo>::iterator itr;
	//for (itr=partMap.begin(); itr != partMap.end(); ++itr) {
	//	std::cout << itr->first << "\t" << itr->second.parentSize << "\n";
	//}
	
	partition_t *pInfo = (partition_t*)g.get_partition_information();
	if (!pInfo)
		return "";
	
	// for now this is just the largest number we see, however
	// in multidimensional cases, this will be a function of
	// dimensionality and size
	int mostActiveThreads = 0;		
		
	itr = partMap.begin();
	for (; itr != partMap.end(); ++itr) {
		string nparts = "nparts" + itr->first;
		
		string size = "UNKNOWN_SIZE";
		string tmp = itr->first;
		tmp.erase(0,3);
		tmp = "$$" + tmp;
		if (pInfo->sizeMap.find(tmp) != pInfo->sizeMap.end()) {
			size = boost::lexical_cast<string>(pInfo->sizeMap[tmp]);
			if (pInfo->sizeMap[tmp] > mostActiveThreads)
				mostActiveThreads = pInfo->sizeMap[tmp];
		}
		
		out << "int " << nparts << " = " << size << ";\n";
		out << "int " << itr->first << ";\n";
		out << "if (" << nparts << " > 1 && " << itr->second.parentSize << " > " << nparts 
			<< ") {\n";
		out << itr->first << " = " << itr->second.parentSize << "/" << nparts << ";\n";
		out << "if (" << itr->first << " * " << nparts << "!= " << itr->second.parentSize << ")\n";
		out << itr->first << "++;\n";
		out << "}\nelse {\n";
		out << itr->first << " = " << itr->second.parentSize << ";\n";
		out << nparts << " = 1;\n";
		out << "}\n";
	}
	
	// create thread and message structures
	// for now lets create most threads that will have live at once
	// message structures will be a function of dimensionality,
	// and must map to each top subgraph so that reductions work
	// correctly.
	if (!mpi) {
		out << "\n// threads and message structures\n";
		out << "pthread_t *threads = (pthread_t*)malloc(sizeof(pthread_t)*" 
			<< mostActiveThreads << ");\n";
	}
	
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		// not all top level subgraphs are partitioned for parallel
		if (g.subgraphs[i]->sg_iterator.step.compare("1") == 0)
			continue;
		
		string nparts = "nparts_"+boost::lexical_cast<string>(g.subgraphs[i]->uid.first)
					+"_"+boost::lexical_cast<string>(g.subgraphs[i]->uid.second);
		
		// for now the number of threads active is same size as the 
		// subgraphs step 
		string s = g.subgraphs[i]->sg_iterator.step;
		s.erase(0,2);
		out << "int " << nparts << " = nparts__s" << s << ";\n";
		
		
		subgraph *sg = g.subgraphs[i];
		string uid = sg->str_id;
		string messageType = name + "_" + uid + "_msg_t";
		string messageName = "mesg_" + uid;
		out << messageType << " *" << messageName << " = (" << messageType 
		<< "*)malloc(sizeof(" << messageType << ")"
		<< "*" << nparts << ");\n";
		
		subgraphTonparts[g.subgraphs[i]] = nparts; 
	}
	
	return out.str();
	/*
	switch (partMap.size()) {
		case 0:
			break;
		case 1:
			itr = partMap.begin();
			out << "int nparts = NUM_THREADS;\n";
			out << "int " << itr->first << ";\n";
			out << "if (nparts > 1 && " << itr->second.parentSize << " > nparts) {\n";
			out << itr->first << " = " << itr->second.parentSize << "/nparts;\n";
			out << "if (" << itr->first << " * nparts != " << itr->second.parentSize << ")\n";
			out << itr->first << "++;\n";
			out << "}\nelse {\n";
			out << itr->first << " = " << itr->second.parentSize << ";\n";
			out << "nparts = 1;\n";
			out << "}\n";
			break;
		case 2: {
			
			out << "// assuming an even number of threads\n";
			out << "int nparts = 1;\n";
			
			for (itr=partMap.begin(); itr != partMap.end(); ++itr) {
				out << "int " << itr->first << "=" << itr->second.parentSize << ";\n";
			}
			
			itr=partMap.begin();
			string one = itr->first;
			itr++;
			string two = itr->first;
			int divLimit = 32;
			out << "int divLimit = " << divLimit << ";\n";
			out << "while (nparts < NUM_THREADS) {\n";
			out	<< "if (" << one << " > " << two << ") {\n";
			out << "if (" << one << " < divLimit)\n";
			out << "break;\n";
			out << one << " = ((" << one << " & 0x1) == 0) ? " << one << " >> 1 : ("
				<< one << " >> 1) + 1;\n";
			out << "}\nelse {\n";
			out << "if (" << two << " < divLimit)\n";
			out << "break;\n";
			out << two << " = ((" << two << " & 0x1) == 0) ? " << two << " >> 1 : ("
				<< two << " >> 1) + 1;\n";
			out << "}\n";
			out << "nparts <<= 1;\n";
			out << "}\n\n";
			break;
		}
		case 3:
			std::cout << "FINISH: translate_to_pthreads.cpp; genratePartitionSizes(); handle 3 dimensional"
					  << " partitioning\n";
			break;
		default:
			std::cout << "ERROR: translate_to_pthreads.cpp; genratePartitionSizes(); unexpected number"
			<< " of partitions\n";
	}
	*/
}

void mapSgToDepth(subgraph *sg, map<subgraph*,int> &sgDepth, int curDepth) {
	// recusively map subgraphs to their overall depth
	sgDepth[sg] = curDepth;
	for (int i = 0; i < sg->subs.size(); ++i)
		mapSgToDepth(sg->subs[i],sgDepth,curDepth+1);
}

void codeGenThreadBody(ostream& out, graph &g, subgraph *sg, int threadDepth, int curDepth) {
	// for the thread body, we need to start code generation from the correct
	// level of nesting.  This will find that level and start code gen for that
	// level and deeper.
	if (curDepth >= threadDepth) {
  		vector<subgraph*> d(1,sg);
  		init_partitions(g, d, out);
		translate_graph_intrin(out, sg, sg->vertices, sg->subs, g);	
	}
	else {
		// these need to be ordered to ensure dependencies are maintained
		deque<vertex> order;
  		map<vertex,subgraph*> new_sub;
  		map<vertex,vertex> new_old;
  		order_subgraphs(order, new_sub, new_old, sg, sg->vertices, sg->subs, g);
  		// once we have an order we can continue
  		for (unsigned int i = 0; i != order.size(); ++i) {
    		map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    		if (iter != new_sub.end()) {
	    		subgraph* sgOrdered = iter->second;
      			codeGenThreadBody(out, g, sgOrdered, threadDepth, curDepth+1);
    		}
    	} 
	}
}	

void codeGenThreadMessage(ostream& out, graph &g, std::set<string> &usedSizes,
						  std::map<string,vertex> &data, string &name, string uid,
						  std::map<vertex, int> &inOut,
						  std::map<vertex, string> &sumtoSize) {
 	// when we have hit the thread body part of the graph, create
	// the thread message and dispatch the thread

	string messageType = name + "_" + uid + "_msg_t";
	string messageName = "mesg_" + uid;	
	string threadFunctionName = name + "_body_" + uid;
	
	// fill message
	// first iterations and size values
	std::set<string>::iterator is = usedSizes.begin();
	for (; is != usedSizes.end(); ++is) {
		out << messageName << "[disp]." << *is << " = " << *is << ";\n";
	}
		
	// then pointers
	std::map<string,vertex>::iterator im = data.begin();
	for (; im != data.end(); ++im) {
		out << messageName << "[disp]." << im->first << " = ";
		// look for scalars that are pointers
		// scalars are complicated; if they are an output
		// the right hand side is a pointer.  If the top level 
		// allocated an array of pointers that will be reduced
		// the left hand side at this point is already a pointer
		// and the * can be commited, else the * is needed.
		// if the scalar is an input the left hand side is
		// not a pointer.  The right hand side will be a
		// non-pointer if it is an input, otherwise another
		// sg must have generated it as an output making it a pointer
		
		if (g.info(im->second).t.k == scalar && inOut[im->second]
			&& sumtoSize[im->second].compare("") != 0)
			out << "*";
		
		if (g.info(im->second).t.k == scalar 
			&& g.info(im->second).op != input
			&& !inOut[im->second])
			out << "*";
		
		out	<< im->first;
		// look for sumtos that have been reduced
		if (g.info(im->second).op == sumto && inOut[im->second] 
			&& sumtoSize[im->second].compare("") != 0)
			out << "+disp*" << container_size(im->second, g);
		out << ";\n";
	}
		
	return;
}

void codeGenDispatchMPI(ostream& out, graph &g, subgraph *sg, int threadDepth, int curDepth,
					 std::map<string,vertex> &data, std::set<string> &usedSizes, 
					 string &name, std::map<vertex, int> &inOut,
					 std::map<vertex, string> &sumtoSize) {
					
	// create the dispatch loops and necessary pointers
	char itrVar = var_name[curDepth+1];
	string step = sg->sg_iterator.step;
	string min;
	bool useMin = false;
	if (step.find("$$") == 0) {
		step.erase(0,2);
		min = string(step);
		min = "__m" + min;
		step = "__s" + step;
		useMin = true;
	}
	string iterations = sg->sg_iterator.iterations;
	if (iterations.find("$$") == 0) {
		iterations.erase(0,2);
		iterations = "__s" + iterations;
	}
	// set itrVar
	out << "MPI_Comm_rank(MPI_COMM_WORLD, &disp" << ");\n";
	out << itrVar << " = disp*" << step << ";\n";
	if (useMin) {
		out << "int " << min << " = " << itrVar << "+" << step << ">" << iterations
			 << "?" << iterations << "-" << itrVar << ":" << step << ";\n"; 
	}
	
	// create pointers and variables
	// NOTE: this may be need to be ordered?
	for (int i = 0; i < sg->vertices.size(); ++i) {
		translate_declareVariables_intrin(out, g, sg->vertices[i],true);
	}
	
	// handle next level
	// when curDepth+1 < threadDepth, we have more dispatch loops to make
	// but when curDepth+1 == threadDepth, we just want to set up the message
	// for the threads
	if (curDepth+1 < threadDepth) {
		for (int i = 0; i < sg->subs.size(); ++i)
			codeGenDispatchMPI(out,g,sg->subs[i],threadDepth,curDepth+1, data, usedSizes,
							name, inOut, sumtoSize);
	}
	else {
		string uid = sg->str_id;
		codeGenThreadMessage(out, g, usedSizes, data, name, 
							 uid, inOut,
							 sumtoSize);
		out << (name+"_body_"+uid) << "( (void *)(" << ("mesg_"+uid) << "+disp));\n";
	}
}

void codeGenDispatch(ostream& out, graph &g, subgraph *sg, int threadDepth, int curDepth,
					 std::map<string,vertex> &data, std::set<string> &usedSizes, 
					 string &name, std::map<vertex, int> &inOut,
					 std::map<vertex, string> &sumtoSize) {
					
	// create the dispatch loops and necessary pointers
	char itrVar = var_name[curDepth+1];
	string step = sg->sg_iterator.step;
	string min;
	bool useMin = false;
	if (step.find("$$") == 0) {
		step.erase(0,2);
		min = string(step);
		min = "__m" + min;
		step = "__s" + step;
		useMin = true;
	}
	string iterations = sg->sg_iterator.iterations;
	if (iterations.find("$$") == 0) {
		iterations.erase(0,2);
		iterations = "__s" + iterations;
	}
	// create loop
	out << "for (" << itrVar << " = 0; " << itrVar << " < " << iterations << "; " 
		<< itrVar << " += " << step << ") {\n";
	if (useMin) {
		out << "int " << min << " = " << itrVar << "+" << step << ">" << iterations
			 << "?" << iterations << "-" << itrVar << ":" << step << ";\n"; 
	}
	
	// create pointers and variables
	// NOTE: this may be need to be ordered?
	for (int i = 0; i < sg->vertices.size(); ++i) {
		translate_declareVariables_intrin(out, g, sg->vertices[i],true);
	}
	
	// handle next level
	// when curDepth+1 < threadDepth, we have more dispatch loops to make
	// but when curDepth+1 == threadDepth, we just want to set up the message
	// for the threads
	if (curDepth+1 < threadDepth) {
		for (int i = 0; i < sg->subs.size(); ++i)
			codeGenDispatch(out,g,sg->subs[i],threadDepth,curDepth+1, data, usedSizes,
							name, inOut, sumtoSize);
	}
	else {
		string uid = sg->str_id;
		codeGenThreadMessage(out, g, usedSizes, data, name, 
							 uid, inOut,
							 sumtoSize);
		// dispatch thread
		out << "pthread_create(&threads[disp], NULL, " << (name+"_body_"+uid)
			<< ", (void *)(" << ("mesg_"+uid) << "+disp));\n";
		out << "++disp;\n";
	}
	out << "}\n";
}

bool vectorContains(vector<subgraph*>&subs, subgraph *sg) {
	for (unsigned int i = 0; i < subs.size(); ++i) 
		if (subs[i] == sg)
			return true;
	return false;
}

void translate_to_pthreads(ostream& out,
		    string name,
		    map<string,type*>const& inputs,
		    map<string,type*>const& outputs, 
		    graph& g,
			int threadDepth,
			int numThreads)
{
	// change all vertex sizes from $$* to s*
	// have to touch all levels of a type because functions like get_next_element use
	// lower levels of a given type
	for (int i = 0; i != g.num_vertices(); i++) {
		type *t = &g.info(i).t;
		while (t) {
			string &s = t->dim.dim;
			if (s.find("$$") == 0) {
				s.erase(0,2);
				s = "__m" + s;
			}
			t = t->t;
		}
	}
	
	out << "#include <stdlib.h>\n";
	out << "#include <pthread.h>\n";
	out << "#define NUM_THREADS " << numThreads << "\n";
	
	// find any top level subgraphs that are not threaded.
	vector<subgraph*> noThreadSg;
	for (int i = 0; i < g.subgraphs.size(); ++i) {
		subgraph *sg = g.subgraphs[i];
		if (sg->sg_iterator.step.compare("1") == 0)
			noThreadSg.push_back(sg);
	}
	
	
	// handle partition sizing
	// NOTE: this needs attention
	map<subgraph*,string>subToThreadCnt;
	string partitionSize = generatePartitionSizes(g,subToThreadCnt,threadDepth,name, false);
	
	
	//////////// create struct //////////////////////	

	// This is looking for all vertices that are defined in the thread dispatch level
	// partitions and have an adjacent edge to a vertex that lies in the thread body
	// level.  These values need to be sent to the thread in the message structure.
	// They are put into the map 'sgToMessage'
	
	// because we can have multiple subgraphs that want to be parallel at a top level
	// we need to create a thread body, message structure, and dispatch code for
	// each of these
	std::map<subgraph*, std::map<string, vertex> > sgToMessage;
	
	// map subgraphs to depths
	std::map<subgraph*,int> sgDepth;
	for (int i = 0; i < g.subgraphs.size(); ++i)
		mapSgToDepth(g.subgraphs[i], sgDepth,1);
	
	// map strings to types for all inputs and outputs
	for (int i = 0; i != g.num_vertices(); i++) {
		if (g.info(i).op == deleted || g.info(i).op == partition_cast)
			continue;
				
		// gather top most level information
		if (g.info(i).op == sumto && g.find_parent(i) == NULL) {
			// get correct name
			string target;
			if (g.info(i).op != input && g.info(i).op != output) {
				target = "t" + boost::lexical_cast<string>(i);
			}
			else {
				target = string(g.info(i).label);
			}
			sgToMessage[NULL][target] = i;
		}
		
		// temporaries and sumtos need special casing, if they are within a thread body
		// the threads should create their own, however if they are at the top level
		// and used to pass a result between two threads, the top level 
		// code gen is responsible for them
		//
		// if depth == 0, this needs to be created at top level to pass between
		//		threads, so we need this in the message structure
		// if depth is in a dispatch loop (depth <= threadDepth) 
		// (this may need to be restricted the deepest dispatch loop, depth == threadDepth)
		// then either
		// 1) if the inv adj and adj are thread body (depth > threadDepth)
		//			thread will create temporary space, no pointer needed
		// 2) else 
		//			top level will create this space and we need pointer in structure
		if ((g.info(i).op == sumto || g.info(i).op == temporary) 
			&& sgDepth[g.find_parent(i)] > 0 && sgDepth[g.find_parent(i)] <= threadDepth
			&& g.inv_adj(i).size() > 0 && sgDepth[g.find_parent(g.inv_adj(i)[0])] > threadDepth
			&& g.adj(i).size() > 0 && sgDepth[g.find_parent(g.adj(i)[0])] > threadDepth)
			continue;

		if (sgDepth[g.find_parent(i)] <= threadDepth) {

			bool adj = false;
			bool inv_adj = false;
			for (int k = 0; k < g.adj(i).size(); ++k) {
				vertex u = g.adj(i)[k];
				while (g.info(u).op == partition_cast) {
					u = g.adj(u)[0];
				}
				if (sgDepth[g.find_parent(u)] > threadDepth) {
					adj = true;
					break;
				}
			}
			
			for (int k = 0; k < g.inv_adj(i).size() && !adj; ++k) {
				vertex u = g.inv_adj(i)[k];
				while (g.info(u).op == partition_cast) {
					u = g.inv_adj(u)[0];
				}
				if (sgDepth[g.find_parent(u)] > threadDepth) {
					inv_adj = true;
					break;
				}
			}
			
			if (adj || inv_adj) {
				// we collect this information based on which top level subgraph
				// these belong to, however information can need to come from
				// the outermost graph (parent == NULL) or a nested graph
				// used for threads.  In these cases we have to
				// determine which top level subgraph(s) this belongs to

				subgraph *parent = g.find_parent(i);
				std::set<subgraph*> parents;
				parents.clear();
				if (parent == NULL) {
					// handle adjacent
					for (unsigned int j = 0; j < g.adj(i).size(); ++j) {
						vertex u = g.adj(i)[j];
						while (g.info(u).op == partition_cast) {
							u = g.adj(u)[0];
						}
						subgraph *p = g.find_parent(u);
						if (sgDepth[p] <= threadDepth)
							continue;
						while (p && !vectorContains(g.subgraphs,p)) {
							// nested subgraph, find top level
							p = p->parent;
						}
						if (p != NULL)
							parents.insert(p);
					}
					// handle inverse adjacent
					for (unsigned int j = 0; j < g.inv_adj(i).size(); ++j) {
						vertex u = g.inv_adj(i)[j];
						while (g.info(u).op == partition_cast) {
							u = g.inv_adj(u)[0];
						}
						subgraph *p = g.find_parent(u);
						if (sgDepth[p] <= threadDepth)
							continue;
						while (p && !vectorContains(g.subgraphs,p)) {
							// nested subgraph, find top level
							p = p->parent;
						}
						if (p != NULL)
							parents.insert(p);
					}
				}
				else {
					while (!vectorContains(g.subgraphs,parent)) {
						// nested subgraph for threads, find top level
						parent = parent->parent;
					}
					parents.insert(parent);
				}
				
				// get correct name
				string target;
				if (g.info(i).op != input && g.info(i).op != output) {
					target = "t" + boost::lexical_cast<string>(i);
				}
				else {
					target = string(g.info(i).label);
				}
				
				// add to structure
				std::set<subgraph*>::iterator parItr = parents.begin();
				for (;parItr != parents.end(); ++parItr) {
					sgToMessage[*parItr][target] = i;
				}
			}
		}
	}	
	
	// find sumto information
	// for each top level dispatch sg find all sumto vertices and if they
	// are input or output
	// where 
	// input = 0; output = 1;
	//	top level dispatch sg -> vertex -> input/output
	// and
	// for each sumto vertex, store its size information 
	// where
	// size info will be "" or "*nparts"
	std::map<vertex, string> sumtoSize;
	std::map<subgraph*, std::map<vertex, int> > inOut;
	std::map<subgraph*, std::map<string, vertex> >::iterator mesgItr = sgToMessage.begin();
	std::map<string,vertex>::iterator im;
	for (; mesgItr != sgToMessage.end(); ++mesgItr) {
		im = mesgItr->second.begin();
		for (; im != mesgItr->second.end(); ++im) {
			string size = "";
			if (g.find_parent(im->second) == NULL) {
				if (g.info(im->second).op == sumto && g.inv_adj(im->second).size() > 0) {
					vertex pred = g.inv_adj(im->second)[0];
					while (g.info(pred).op == partition_cast) {
						pred = g.inv_adj(pred)[0];
					}
					
					subgraph *p = g.find_parent(pred);
					bool threaded = true;
					while (p->parent) {
						p = p->parent;
					}
					if (threaded) {
						string nparts = subToThreadCnt[p];
						size = "*"+nparts;
					}
				}
				//size = "*nparts";
			}
			
			if (g.info(im->second).op == sumto) { 
				vertex pred = g.inv_adj(im->second)[0];
				while (g.info(pred).op == partition_cast) {
					pred = g.inv_adj(pred)[0];
				}
				
				subgraph *p = g.find_parent(pred);
				bool threaded = true;
				while (p) {
					if (vectorContains(noThreadSg, p)) {
						threaded = false;
						break;
					}
					p = p->parent;
				}
				if (threaded)
					sumtoSize[im->second] = size;
			}
			
			// adj -> input
			for (int i = 0; i < g.adj(im->second).size(); ++i) {
				vertex v = g.adj(im->second)[i];
				subgraph *parent = g.find_parent(v);
				while (parent) {
					if (!vectorContains(g.subgraphs,parent)) {
						parent = parent->parent;
						continue;
					}
					break;
				}
				if (parent && parent == mesgItr->first) {
					inOut[mesgItr->first][im->second] = 0;
					break;
				}
			}
			// inv_adj -> output
			for (int i = 0; i < g.inv_adj(im->second).size(); ++i) {
				vertex v = g.inv_adj(im->second)[i];
				subgraph *parent = g.find_parent(v);
				while (parent) {
					if (!vectorContains(g.subgraphs,parent)) {
						parent = parent->parent;
						continue;
					}
					break;
				}
				if (parent && parent == mesgItr->first) {
					inOut[mesgItr->first][im->second] = 1;
					break;
				}
			}
		}
	}
	
	// find sizes that are actually used
	std::map<subgraph*,std::set<string> > sgToUsedSizes;
	mesgItr = sgToMessage.begin();
	for (; mesgItr != sgToMessage.end(); ++mesgItr) {
		im = mesgItr->second.begin();
		for (; im != mesgItr->second.end(); ++im) {
			type &t = g.info(im->second).t;
			string s = string(t.dim.base_rows);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}			
			sgToUsedSizes[mesgItr->first].insert(s);
		
			s = string(t.dim.base_cols);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}			
			sgToUsedSizes[mesgItr->first].insert(s);

			s = string(t.dim.dim);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}
			sgToUsedSizes[mesgItr->first].insert(s);
		
			type *tt = t.t;
			while (tt) {
				if (tt->k == scalar)
					break;
				if (tt->k != t.k) {
					s = string(tt->dim.dim);
					if (s.find("$$") == 0) {
						s.erase(0,2);
						string t = "__s" + s;
						s = "__m" + s;
						sgToUsedSizes[mesgItr->first].insert(t);
					}
					else if (s.find("__s") == 0) {
						sgToUsedSizes[mesgItr->first].insert(s);
						s.erase(0,3);
						s = "__m" + s;
					}					
					sgToUsedSizes[mesgItr->first].insert(s);
					break;
				}
				tt = tt->t;
			}
		}
		// clean up
		std::set<string>::iterator usedIt = sgToUsedSizes[mesgItr->first].find("1");
		if (usedIt != sgToUsedSizes[mesgItr->first].end())
			sgToUsedSizes[mesgItr->first].erase(usedIt);
	}
	
	// NOTE: When more than one level of subgraphs is used for threading
	// both sgToMessage and sgToUsedSizes will need to be collapsed
	// down to only their top level graph.
	
	// for each top level thread subgraph create a message structure
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		// not all top level subgraphs are partitioned for parallel
		if (g.subgraphs[i]->sg_iterator.step.compare("1") == 0)
			continue;
			
		subgraph *sg = g.subgraphs[i];
		// for each top level subgraph
		out << "typedef struct {\n";
		
		std::map<string, vertex> &data = sgToMessage[sg];
		for (im=data.begin(); im != data.end(); ++im) {
			if (g.info(im->second).t.k == scalar) {
				// if this scalar is an output it must be a pointer
				if (inOut[sg][im->second])
					out << precision_type << " *" << im->first << ";\n";
				else
					out << precision_type << " " << im->first << ";\n";
			}
			else
				out << precision_type << " *" << im->first << ";\n";
		}
		out << "// usedSizes\n";
		std::set<string>::iterator usedIt = sgToUsedSizes[sg].begin();
		for (;usedIt != sgToUsedSizes[sg].end(); ++usedIt)
			out << "int " << *usedIt << ";\n";
	
		out << "} " << name << "_" << sg->str_id 
			<< "_msg_t;\n\n";
	}
	
	//////////// end struct ////////////////////////
	
	/////////// generate thread body //////////////
	// one thread body for each top level subgraph intended for threads
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		// not all top level subgraphs are partitioned for parallel
		if (g.subgraphs[i]->sg_iterator.step.compare("1") == 0)
			continue;
			
		subgraph *sg = g.subgraphs[i];
		string uid = sg->str_id;
		string messageType = name + "_" + uid + "_msg_t";
		string threadFunctionName = name + "_body_" + uid;
		
		out << "void *" << threadFunctionName 	<< "(void* mesg) {\n";
		out << messageType + " *msg = (" << messageType + "*)mesg;\n";
	
		// decode message
		// first iterations and size values
		out << "// iterations and size values\n";
		std::set<string>::iterator is = sgToUsedSizes[sg].begin();
		for (; is != sgToUsedSizes[sg].end(); ++is) {
			if (is->compare("1") == 0)
				continue;
			string tmp = *is;
			out << "int " << tmp << " = msg->" << *is << ";\n";
		}
	
		// then pointers
		string scalarOuts = "";
		out << "// pointers\n";
		im = sgToMessage[sg].begin();
		for (; im != sgToMessage[sg].end(); ++im) {
			if (g.info(im->second).t.k == scalar) {
				// if this scalar is an output it must be a pointer
				if (sgDepth[g.find_parent(im->second)] 
					> sgDepth[g.find_parent(g.adj(im->second)[0])]) {
					out << precision_type << "* " << im->first << "_ptr = msg->" 
						<< im->first << ";\n";
					out << precision_type << " " << im->first << " = *" 
						<< im->first << "_ptr;\n";
					scalarOuts += "*" + im->first + "_ptr = " + im->first + ";\n";
				}
				else {
					out << precision_type << " " << im->first << " = msg->" 
					<< im->first << ";\n";
				}
			}
			else {
				out << precision_type << "* " << im->first << " = msg->" 
					<< im->first << ";\n";
			}
		}
	
		// declare iteration vars
		out << "// iteration variables\n";
		int maxd = 0;
		check_depth(1,maxd, g.subgraphs);
		if (maxd > 0) {
			out << "int ii";
			for (int i = threadDepth+1; i <= maxd; ++i)
				out << "," << var_name[i];
			out << ";\n";
		}
		else {
	  		out << "int ii;\n";
		}
	
		// set up temporary structures
		for (unsigned int u = 0; u != g.num_vertices(); ++u) {
    		if (g.find_parent(u) == sg && sgDepth[g.find_parent(u)] == threadDepth 
    			&& (g.adj(u).size() > 0 || g.inv_adj(u).size() > 0)) {
      			translate_tmp_intrin(out, g, u);
    		}
  		}
	
		// clear reduction space
		std::map<vertex, int>::iterator ioItr = inOut[sg].begin();
		for (; ioItr != inOut[sg].end(); ++ioItr) {
			if (g.info(ioItr->first).op != sumto)
				continue;
			
			// do not have to clear inputs
			if (!ioItr->second)
				continue;
			
			// only need to clear those structures at the top level
			// others will already be cleared at appropriate location
			vertex u = ioItr->first;
			if (g.find_parent(u) != NULL)
				continue;
			
			string label = "t" + boost::lexical_cast<string>(u);
				
			if (g.info(u).t.k == scalar)
				out << label << " = 0.0;\n";
			else {
				out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
					<< label << "[ii] = 0.0;\n";
			}
		}
		
		// thread body graph to code
		codeGenThreadBody(out, g, sg, threadDepth, 0);
	
		out << scalarOuts << "\n";
		out << "return NULL;\n}\n\n\n";
	}
	/////////// end thread body //////////////////
	
	/////////// main function body ////////////////////
  	// map for function (required by noPtr, empty here)
	map<string, pair<vertex, type> > data;
	
	out << make_function_signature(name,inputs,outputs,data,"",typeWithName,false);
	//out << "void " << name << function_args(inputs,outputs,data,"",typeWithName,false);
    //out << "{\n";
	
    // string of pointers for scalar output
    string ptrOutputLcl;
    string ptrOutputEnd;
    for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
        if (i->second->k == scalar) {
            // create local working scalar value
            ptrOutputLcl += type_to_c(*i->second) + " " + i->first + " = ";
            ptrOutputLcl += "*" + i->first + "_ptr;\n";
            // create store back to argument
            ptrOutputEnd +="*" + i->first + "_ptr = " + i->first + ";\n";
        }
    }
  	
	// local copies of scalar outputs
	out << ptrOutputLcl;
	
	// declare iteration vars
	// a top level sg may not be partitioned for threading but may be deeper than
	// threadDepth
	int maxd = threadDepth;
	check_depth(1,maxd, noThreadSg);
	out << "int ii";
	for (int i = 1; i <= maxd; ++i)
		out << "," << var_name[i];
	out << ";\n";

	
	
	// print partition size information
	out << partitionSize;
	
	
	// handle any non declared data at the top level
	// specifically, we are looking for reductions
	map<subgraph*, set<vertex> > sgToReduce;
	for (vertex i = 0; i != g.num_vertices(); ++i) {
		// only sumto, temporary, inputs, outputs should be at this level
		// however we only need to handle sumto and temporary
		if (g.info(i).op == sumto || g.info(i).op == temporary) {
			// only way to find top level non input/outputs
			if (g.find_parent(i) == NULL) {
				
                // if the outer loop is not partitioned for parallel
                // i.e. we have a mixed parallel/non parallel case
                // then these values do not need to be special cased
                // because they already exist.
                vertex pred = g.inv_adj(i)[0];
                while (g.find_parent(pred) == NULL)
                    pred = g.inv_adj(pred)[0];
                bool partitioned = g.find_parent(pred)->sg_iterator.partitioned();
                if (!partitioned && g.info(i).t.k == scalar)
                    continue;
                
                
				if (g.info(g.adj(i)[0]).op == output && 
					sumtoSize[i].compare("") == 0) {
					out << precision_type << "* t" << i << " = "
						<< g.info(g.adj(i)[0]).label << ";\n";
				}
				else {					
					// create temporary space for each thread
					out << precision_type << "* t" << i << " = (" << precision_type 
					<< "*) malloc(sizeof(" << precision_type << ") * "
					<< container_size(i, g) << sumtoSize[i]
					<< ");\n";
				}
				if (g.info(i).op == sumto) {
					subgraph *p = g.find_parent(g.inv_adj(i)[0]);
					while (p && !vectorContains(g.subgraphs,p)) {
						// nested subgraph, find top level
						p = p->parent;
					}
					if (p)
						sgToReduce[p].insert(i);
				}
			}
		}
	}
	
	// clear any data structures being reduced to in top level (depth = 0)
	out << "// clear reduction structures\n";
	
	map<string, vertex>::iterator fi = sgToMessage[NULL].begin();
	for (; fi != sgToMessage[NULL].end(); ++fi) {
		vertex u = fi->second;
		
		if (g.info(g.inv_adj(u)[0]).op == sumto &&
			sgDepth[g.find_parent(g.inv_adj(u)[0])] <= threadDepth
			&& g.info(g.adj(u)[0]).op != output)
			continue;
		
		string label;
		if (g.info(g.adj(u)[0]).op == output) {
			u = g.adj(u)[0];
			label = g.info(u).label;
		}
		else {
			label = "t" + boost::lexical_cast<string>(u);
		}	
		if (g.info(u).t.k == scalar) {

			if (sumtoSize[u].compare("") == 0)
				out << label << " = 0.0;\n";
			else {
				out << "for (ii = 0; ii < nparts; ++ii)\n"
					<< label << "[ii] = 0.0;\n";
			}
		}
		else {
			out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
			<< label << "[ii] = 0.0;\n";
		}
	}
	
	// set up dispatch loops and correct pointers
	out << "// dispatch loops\n";
	out << "int disp;\n";

	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
	vector<vertex> vertices;
	for (unsigned int i = 0; i < g.num_vertices(); ++i)
		if (g.find_parent(i) == NULL)
			vertices.push_back(i);
  	order_subgraphs(order, new_sub, new_old, NULL, vertices, g.subgraphs, g);
	
	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter == new_sub.end()) 
			continue;
		
		subgraph* sg = iter->second;
	
		// not all top level subgraphs are partitioned for parallel
		if (sg->sg_iterator.step.compare("1") == 0) {
			translate_graph_intrin(out, sg, sg->vertices, sg->subs, g);
			continue;
		}
			
		out << "\n";
		out << "disp = 0;\n";
		set<vertex>::iterator reIt = sgToReduce[sg].begin();
		
		// dispatch threads
		out << "// dispatch to threads\n";
		codeGenDispatch(out, g, sg, threadDepth, 0, sgToMessage[sg], 
			sgToUsedSizes[sg], name, inOut[sg], sumtoSize);
		
		// wait for threads to end and reduce if appropriate
		string nparts = subToThreadCnt[sg];
		out << "// wait for threads and reduce when appropriate\n";
		out << "for (disp = 0; disp < " << nparts << "; ++disp) {\n";
		out << "pthread_join(threads[disp],NULL);\n";
		reIt = sgToReduce[sg].begin();
		for (; reIt != sgToReduce[sg].end(); ++reIt) {
			
			vertex u;
			// deterimine if label[0] += or label += is correct.
			bool ptr = false;
			if (sumtoSize[*reIt].compare("") != 0)
				ptr == true;
			//std::cout << *reIt << "\t" << (ptr ? "true" : "false") << "\n";
			
			string label;
			if (g.info(g.adj(*reIt)[0]).op == output) {
				u = g.adj(*reIt)[0];
				label = g.info(u).label;
			}
			else {
				// when there is reduction into the same structure work was performed
				// in, skip the reduction for the first thread, because the data
				// is already in place.
				out << "if (disp) {\n";
				
				u = *reIt;
				label = "t" + boost::lexical_cast<string>(u);
				ptr = true;
			}	
			
			if (g.info(u).t.k == scalar) {
				if (ptr)
					out << label << "[0] += t" << boost::lexical_cast<string>(*reIt)
						<< "[disp];\n";
				else
					out << label << " += t" << boost::lexical_cast<string>(*reIt)
			  			<< "[disp];\n";
			 }
			else {
				out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
				 	 	<< label << "[ii] += t" << boost::lexical_cast<string>(*reIt)
			  			<< "[disp*" << container_size(*reIt,g) << "+ii];\n";
			}
			
			// end if(disp)
			if (g.info(g.adj(*reIt)[0]).op != output)
				out << "}\n";
		}
		out << "}\n\n";
	
		// clean up
		out << "free(mesg_" << sg->str_id << ");\n";
	}
	out << "free(threads);\n";
	
	// handle any scalar outputs
	out << ptrOutputEnd;
 
  	out << "}\n";
  	
}

void translate_to_mpi(ostream& out,
		    string name,
		    map<string,type*>const& inputs,
		    map<string,type*>const& outputs, 
		    graph& g,
			int threadDepth,
			int numThreads)
{
	cout << "Entering MPI generation" << endl;
	// change all vertex sizes from $$* to s*
	// have to touch all levels of a type because functions like get_next_element use
	// lower levels of a given type
	for (int i = 0; i != g.num_vertices(); i++) {
		type *t = &g.info(i).t;
		while (t) {
			string &s = t->dim.dim;
			if (s.find("$$") == 0) {
				s.erase(0,2);
				s = "__m" + s;
			}
			t = t->t;
		}
	}
	
	out << "#include <stdlib.h>\n";
	out << "#include \"mpi.h\"\n";
	out << "#define NUM_NODES " << numThreads << "\n";
	
	// find any top level subgraphs that are not threaded.
	vector<subgraph*> noThreadSg;
	for (int i = 0; i < g.subgraphs.size(); ++i) {
		subgraph *sg = g.subgraphs[i];
		if (sg->sg_iterator.step.compare("1") == 0)
			noThreadSg.push_back(sg);
	}
	
	
	// handle partition sizing
	// NOTE: this needs attention
	map<subgraph*,string>subToThreadCnt;
	string partitionSize = generatePartitionSizes(g,subToThreadCnt,threadDepth,name,true);
	
	
	//////////// create struct //////////////////////	

	// This is looking for all vertices that are defined in the thread dispatch level
	// partitions and have an adjacent edge to a vertex that lies in the thread body
	// level.  These values need to be sent to the thread in the message structure.
	// They are put into the map 'sgToMessage'
	
	// because we can have multiple subgraphs that want to be parallel at a top level
	// we need to create a thread body, message structure, and dispatch code for
	// each of these
	std::map<subgraph*, std::map<string, vertex> > sgToMessage;
	
	// map subgraphs to depths
	std::map<subgraph*,int> sgDepth;
	for (int i = 0; i < g.subgraphs.size(); ++i)
		mapSgToDepth(g.subgraphs[i], sgDepth,1);
	
	// map strings to types for all inputs and outputs
	for (int i = 0; i != g.num_vertices(); i++) {
		if (g.info(i).op == deleted || g.info(i).op == partition_cast)
			continue;
				
		// gather top most level information
		if (g.info(i).op == sumto && g.find_parent(i) == NULL) {
			// get correct name
			string target;
			if (g.info(i).op != input && g.info(i).op != output) {
				target = "t" + boost::lexical_cast<string>(i);
			}
			else {
				target = string(g.info(i).label);
			}
			sgToMessage[NULL][target] = i;
		}
		
		// temporaries and sumtos need special casing, if they are within a thread body
		// the threads should create their own, however if they are at the top level
		// and used to pass a result between two threads, the top level 
		// code gen is responsible for them
		//
		// if depth == 0, this needs to be created at top level to pass between
		//		threads, so we need this in the message structure
		// if depth is in a dispatch loop (depth <= threadDepth) 
		// (this may need to be restricted the deepest dispatch loop, depth == threadDepth)
		// then either
		// 1) if the inv adj and adj are thread body (depth > threadDepth)
		//			thread will create temporary space, no pointer needed
		// 2) else 
		//			top level will create this space and we need pointer in structure
		if ((g.info(i).op == sumto || g.info(i).op == temporary) 
			&& sgDepth[g.find_parent(i)] > 0 && sgDepth[g.find_parent(i)] <= threadDepth
			&& g.inv_adj(i).size() > 0 && sgDepth[g.find_parent(g.inv_adj(i)[0])] > threadDepth
			&& g.adj(i).size() > 0 && sgDepth[g.find_parent(g.adj(i)[0])] > threadDepth)
			continue;

		if (sgDepth[g.find_parent(i)] <= threadDepth) {

			bool adj = false;
			bool inv_adj = false;
			for (int k = 0; k < g.adj(i).size(); ++k) {
				vertex u = g.adj(i)[k];
				while (g.info(u).op == partition_cast) {
					u = g.adj(u)[0];
				}
				if (sgDepth[g.find_parent(u)] > threadDepth) {
					adj = true;
					break;
				}
			}
			
			for (int k = 0; k < g.inv_adj(i).size() && !adj; ++k) {
				vertex u = g.inv_adj(i)[k];
				while (g.info(u).op == partition_cast) {
					u = g.inv_adj(u)[0];
				}
				if (sgDepth[g.find_parent(u)] > threadDepth) {
					inv_adj = true;
					break;
				}
			}
			
			if (adj || inv_adj) {
				// we collect this information based on which top level subgraph
				// these belong to, however information can need to come from
				// the outermost graph (parent == NULL) or a nested graph
				// used for threads.  In these cases we have to
				// determine which top level subgraph(s) this belongs to

				subgraph *parent = g.find_parent(i);
				std::set<subgraph*> parents;
				parents.clear();
				if (parent == NULL) {
					// handle adjacent
					for (unsigned int j = 0; j < g.adj(i).size(); ++j) {
						vertex u = g.adj(i)[j];
						while (g.info(u).op == partition_cast) {
							u = g.adj(u)[0];
						}
						subgraph *p = g.find_parent(u);
						if (sgDepth[p] <= threadDepth)
							continue;
						while (p && !vectorContains(g.subgraphs,p)) {
							// nested subgraph, find top level
							p = p->parent;
						}
						if (p != NULL)
							parents.insert(p);
					}
					// handle inverse adjacent
					for (unsigned int j = 0; j < g.inv_adj(i).size(); ++j) {
						vertex u = g.inv_adj(i)[j];
						while (g.info(u).op == partition_cast) {
							u = g.inv_adj(u)[0];
						}
						subgraph *p = g.find_parent(u);
						if (sgDepth[p] <= threadDepth)
							continue;
						while (p && !vectorContains(g.subgraphs,p)) {
							// nested subgraph, find top level
							p = p->parent;
						}
						if (p != NULL)
							parents.insert(p);
					}
				}
				else {
					while (!vectorContains(g.subgraphs,parent)) {
						// nested subgraph for threads, find top level
						parent = parent->parent;
					}
					parents.insert(parent);
				}
				
				// get correct name
				string target;
				if (g.info(i).op != input && g.info(i).op != output) {
					target = "t" + boost::lexical_cast<string>(i);
				}
				else {
					target = string(g.info(i).label);
				}
				
				// add to structure
				std::set<subgraph*>::iterator parItr = parents.begin();
				for (;parItr != parents.end(); ++parItr) {
					sgToMessage[*parItr][target] = i;
				}
			}
		}
	}	
	
	// find sumto information
	// for each top level dispatch sg find all sumto vertices and if they
	// are input or output
	// where 
	// input = 0; output = 1;
	//	top level dispatch sg -> vertex -> input/output
	// and
	// for each sumto vertex, store its size information 
	// where
	// size info will be "" or "*nparts"
	std::map<vertex, string> sumtoSize;
	std::map<subgraph*, std::map<vertex, int> > inOut;
	std::map<subgraph*, std::map<string, vertex> >::iterator mesgItr = sgToMessage.begin();
	std::map<string,vertex>::iterator im;
	for (; mesgItr != sgToMessage.end(); ++mesgItr) {
		im = mesgItr->second.begin();
		for (; im != mesgItr->second.end(); ++im) {
			string size = "";
			if (g.find_parent(im->second) == NULL) {
				if (g.info(im->second).op == sumto && g.inv_adj(im->second).size() > 0) {
					vertex pred = g.inv_adj(im->second)[0];
					while (g.info(pred).op == partition_cast) {
						pred = g.inv_adj(pred)[0];
					}
					
					subgraph *p = g.find_parent(pred);
					bool threaded = true;
					while (p->parent) {
						p = p->parent;
					}
					if (threaded) {
						string nparts = subToThreadCnt[p];
						size = "*"+nparts;
					}
				}
				//size = "*nparts";
			}
			
			if (g.info(im->second).op == sumto) { 
				vertex pred = g.inv_adj(im->second)[0];
				while (g.info(pred).op == partition_cast) {
					pred = g.inv_adj(pred)[0];
				}
				
				subgraph *p = g.find_parent(pred);
				bool threaded = true;
				while (p) {
					if (vectorContains(noThreadSg, p)) {
						threaded = false;
						break;
					}
					p = p->parent;
				}
				if (threaded)
					sumtoSize[im->second] = size;
			}
			
			// adj -> input
			for (int i = 0; i < g.adj(im->second).size(); ++i) {
				vertex v = g.adj(im->second)[i];
				subgraph *parent = g.find_parent(v);
				while (parent) {
					if (!vectorContains(g.subgraphs,parent)) {
						parent = parent->parent;
						continue;
					}
					break;
				}
				if (parent && parent == mesgItr->first) {
					inOut[mesgItr->first][im->second] = 0;
					break;
				}
			}
			// inv_adj -> output
			for (int i = 0; i < g.inv_adj(im->second).size(); ++i) {
				vertex v = g.inv_adj(im->second)[i];
				subgraph *parent = g.find_parent(v);
				while (parent) {
					if (!vectorContains(g.subgraphs,parent)) {
						parent = parent->parent;
						continue;
					}
					break;
				}
				if (parent && parent == mesgItr->first) {
					inOut[mesgItr->first][im->second] = 1;
					break;
				}
			}
		}
	}
	
	// find sizes that are actually used
	std::map<subgraph*,std::set<string> > sgToUsedSizes;
	mesgItr = sgToMessage.begin();
	for (; mesgItr != sgToMessage.end(); ++mesgItr) {
		im = mesgItr->second.begin();
		for (; im != mesgItr->second.end(); ++im) {
			type &t = g.info(im->second).t;
			string s = string(t.dim.base_rows);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}			
			sgToUsedSizes[mesgItr->first].insert(s);
		
			s = string(t.dim.base_cols);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}			
			sgToUsedSizes[mesgItr->first].insert(s);

			s = string(t.dim.dim);
			if (s.find("$$") == 0) {
				s.erase(0,2);
				string t = "__s" + s;
				s = "__m" + s;
				sgToUsedSizes[mesgItr->first].insert(t);
			}
			else if (s.find("__s") == 0) {
				sgToUsedSizes[mesgItr->first].insert(s);
				s.erase(0,3);
				s = "__m" + s;
			}
			sgToUsedSizes[mesgItr->first].insert(s);
		
			type *tt = t.t;
			while (tt) {
				if (tt->k == scalar)
					break;
				if (tt->k != t.k) {
					s = string(tt->dim.dim);
					if (s.find("$$") == 0) {
						s.erase(0,2);
						string t = "__s" + s;
						s = "__m" + s;
						sgToUsedSizes[mesgItr->first].insert(t);
					}
					else if (s.find("__s") == 0) {
						sgToUsedSizes[mesgItr->first].insert(s);
						s.erase(0,3);
						s = "__m" + s;
					}					
					sgToUsedSizes[mesgItr->first].insert(s);
					break;
				}
				tt = tt->t;
			}
		}
		// clean up
		std::set<string>::iterator usedIt = sgToUsedSizes[mesgItr->first].find("1");
		if (usedIt != sgToUsedSizes[mesgItr->first].end())
			sgToUsedSizes[mesgItr->first].erase(usedIt);
	}
	
	// NOTE: When more than one level of subgraphs is used for threading
	// both sgToMessage and sgToUsedSizes will need to be collapsed
	// down to only their top level graph.
	
	// for each top level thread subgraph create a message structure
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		// not all top level subgraphs are partitioned for parallel
		if (g.subgraphs[i]->sg_iterator.step.compare("1") == 0)
			continue;
			
		subgraph *sg = g.subgraphs[i];
		// for each top level subgraph
		out << "typedef struct {\n";
		
		std::map<string, vertex> &data = sgToMessage[sg];
		for (im=data.begin(); im != data.end(); ++im) {
			if (g.info(im->second).t.k == scalar) {
				// if this scalar is an output it must be a pointer
				if (inOut[sg][im->second])
					out << precision_type << " *" << im->first << ";\n";
				else
					out << precision_type << " " << im->first << ";\n";
			}
			else
				out << precision_type << " *" << im->first << ";\n";
		}
		out << "// usedSizes\n";
		std::set<string>::iterator usedIt = sgToUsedSizes[sg].begin();
		for (;usedIt != sgToUsedSizes[sg].end(); ++usedIt)
			out << "int " << *usedIt << ";\n";
	
		out << "} " << name << "_" << sg->str_id 
			<< "_msg_t;\n\n";
	}
	
	//////////// end struct ////////////////////////
	
	/////////// generate thread body //////////////
	// one thread body for each top level subgraph intended for threads
	for (unsigned int i = 0; i < g.subgraphs.size(); ++i) {
		// not all top level subgraphs are partitioned for parallel
		if (g.subgraphs[i]->sg_iterator.step.compare("1") == 0)
			continue;
			
		subgraph *sg = g.subgraphs[i];
		string uid = sg->str_id;
		string messageType = name + "_" + uid + "_msg_t";
		string threadFunctionName = name + "_body_" + uid;
		
		out << "void *" << threadFunctionName 	<< "(void* mesg) {\n";
		out << messageType + " *msg = (" << messageType + "*)mesg;\n";
	
		// decode message
		// first iterations and size values
		out << "// iterations and size values\n";
		std::set<string>::iterator is = sgToUsedSizes[sg].begin();
		for (; is != sgToUsedSizes[sg].end(); ++is) {
			if (is->compare("1") == 0)
				continue;
			string tmp = *is;
			out << "int " << tmp << " = msg->" << *is << ";\n";
		}
	
		// then pointers
		string scalarOuts = "";
		out << "// pointers\n";
		im = sgToMessage[sg].begin();
		for (; im != sgToMessage[sg].end(); ++im) {
			if (g.info(im->second).t.k == scalar) {
				// if this scalar is an output it must be a pointer
				if (sgDepth[g.find_parent(im->second)] 
					> sgDepth[g.find_parent(g.adj(im->second)[0])]) {
					out << precision_type << "* " << im->first << "_ptr = msg->" 
						<< im->first << ";\n";
					out << precision_type << " " << im->first << " = *" 
						<< im->first << "_ptr;\n";
					scalarOuts += "*" + im->first + "_ptr = " + im->first + ";\n";
				}
				else {
					out << precision_type << " " << im->first << " = msg->" 
					<< im->first << ";\n";
				}
			}
			else {
				out << precision_type << "* " << im->first << " = msg->" 
					<< im->first << ";\n";
			}
		}
	
		// declare iteration vars
		out << "// iteration variables\n";
		int maxd = 0;
		check_depth(1,maxd, g.subgraphs);
		if (maxd > 0) {
			out << "int ii";
			for (int i = threadDepth+1; i <= maxd; ++i)
				out << "," << var_name[i];
			out << ";\n";
		}
		else {
	  		out << "int ii;\n";
		}
	
		// set up temporary structures
		for (unsigned int u = 0; u != g.num_vertices(); ++u) {
    		if (g.find_parent(u) == sg && sgDepth[g.find_parent(u)] == threadDepth 
    			&& (g.adj(u).size() > 0 || g.inv_adj(u).size() > 0)) {
      			translate_tmp_intrin(out, g, u);
    		}
  		}
	
		// clear reduction space
		std::map<vertex, int>::iterator ioItr = inOut[sg].begin();
		for (; ioItr != inOut[sg].end(); ++ioItr) {
			if (g.info(ioItr->first).op != sumto)
				continue;
			
			// do not have to clear inputs
			if (!ioItr->second)
				continue;
			
			// only need to clear those structures at the top level
			// others will already be cleared at appropriate location
			vertex u = ioItr->first;
			if (g.find_parent(u) != NULL)
				continue;
			
			string label = "t" + boost::lexical_cast<string>(u);
				
			if (g.info(u).t.k == scalar)
				out << label << " = 0.0;\n";
			else {
				out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
					<< label << "[ii] = 0.0;\n";
			}
		}
		
		// thread body graph to code
		codeGenThreadBody(out, g, sg, threadDepth, 0);
	
		out << scalarOuts << "\n";
		out << "return NULL;\n}\n\n\n";
	}
	/////////// end thread body //////////////////
	
	/////////// main function body ////////////////////
  	// map for function (required by noPtr, empty here)
	map<string, pair<vertex, type> > data;
	
	out << make_function_signature(name,inputs,outputs,data,"",typeWithName,false);
	//out << "void " << name << function_args(inputs,outputs,data,"",typeWithName,false);
    //out << "{\n";
	
    // string of pointers for scalar output
    string ptrOutputLcl;
    string ptrOutputEnd;
    for (map<string,type*>::const_iterator i = outputs.begin(); i != outputs.end(); ++i) {
        if (i->second->k == scalar) {
            // create local working scalar value
            ptrOutputLcl += type_to_c(*i->second) + " " + i->first + " = ";
            ptrOutputLcl += "*" + i->first + "_ptr;\n";
            // create store back to argument
            ptrOutputEnd +="*" + i->first + "_ptr = " + i->first + ";\n";
        }
    }
  	
	// local copies of scalar outputs
	out << ptrOutputLcl;
	
	// declare iteration vars
	// a top level sg may not be partitioned for threading but may be deeper than
	// threadDepth
	int maxd = threadDepth;
	check_depth(1,maxd, noThreadSg);
	out << "int ii";
	for (int i = 1; i <= maxd; ++i)
		out << "," << var_name[i];
	out << ";\n";

	
	
	// print partition size information
	out << partitionSize;
	
	
	// handle any non declared data at the top level
	// specifically, we are looking for reductions
	map<subgraph*, set<vertex> > sgToReduce;
	for (vertex i = 0; i != g.num_vertices(); ++i) {
		// only sumto, temporary, inputs, outputs should be at this level
		// however we only need to handle sumto and temporary
		if (g.info(i).op == sumto || g.info(i).op == temporary) {
			// only way to find top level non input/outputs
			if (g.find_parent(i) == NULL) {
				
				if (g.info(g.adj(i)[0]).op == output && 
					sumtoSize[i].compare("") == 0) {
					out << precision_type << "* t" << i << " = "
						<< g.info(g.adj(i)[0]).label << ";\n";
				}
				else {					
					// create temporary space for each thread
					out << precision_type << "* t" << i << " = (" << precision_type 
					<< "*) malloc(sizeof(" << precision_type << ") * "
					<< container_size(i, g) << sumtoSize[i]
					<< ");\n";
				}
				if (g.info(i).op == sumto) {
					subgraph *p = g.find_parent(g.inv_adj(i)[0]);
					while (p && !vectorContains(g.subgraphs,p)) {
						// nested subgraph, find top level
						p = p->parent;
					}
					if (p)
						sgToReduce[p].insert(i);
				}
			}
		}
	}
	
	// clear any data structures being reduced to in top level (depth = 0)
	out << "// clear reduction structures\n";
	
	map<string, vertex>::iterator fi = sgToMessage[NULL].begin();
	for (; fi != sgToMessage[NULL].end(); ++fi) {
		vertex u = fi->second;
		
		if (g.info(g.inv_adj(u)[0]).op == sumto &&
			sgDepth[g.find_parent(g.inv_adj(u)[0])] <= threadDepth
			&& g.info(g.adj(u)[0]).op != output)
			continue;
		
		string label;
		if (g.info(g.adj(u)[0]).op == output) {
			u = g.adj(u)[0];
			label = g.info(u).label;
		}
		else {
			label = "t" + boost::lexical_cast<string>(u);
		}	
		if (g.info(u).t.k == scalar) {

			if (sumtoSize[u].compare("") == 0)
				out << label << " = 0.0;\n";
			else {
				out << "for (ii = 0; ii < nparts; ++ii)\n"
					<< label << "[ii] = 0.0;\n";
			}
		}
		else {
			out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
			<< label << "[ii] = 0.0;\n";
		}
	}
	
	// set up dispatch loops and correct pointers
	out << "// dispatch loops\n";
	out << "int disp;\n";

	// topologically sort the subgraphs
  	deque<vertex> order;
  	map<vertex,subgraph*> new_sub;
  	map<vertex,vertex> new_old;
	vector<vertex> vertices;
	for (unsigned int i = 0; i < g.num_vertices(); ++i)
		if (g.find_parent(i) == NULL)
			vertices.push_back(i);
  	order_subgraphs(order, new_sub, new_old, NULL, vertices, g.subgraphs, g);
	
	for (unsigned int i = 0; i != order.size(); ++i) {
    	map<vertex,subgraph*>::iterator iter = new_sub.find(order[i]);
    	if (iter == new_sub.end()) 
			continue;
		
		subgraph* sg = iter->second;
	
		// not all top level subgraphs are partitioned for parallel
		if (sg->sg_iterator.step.compare("1") == 0) {
			translate_graph_intrin(out, sg, sg->vertices, sg->subs, g);
			continue;
		}
			
		out << "\n";
		out << "disp = 0;\n";
		set<vertex>::iterator reIt = sgToReduce[sg].begin();
		
		// dispatch threads
		out << "// dispatch to threads\n";
		codeGenDispatchMPI(out, g, sg, threadDepth, 0, sgToMessage[sg], 
			sgToUsedSizes[sg], name, inOut[sg], sumtoSize);
		
		// wait for threads to end and reduce if appropriate
		string nparts = subToThreadCnt[sg];
		// Going to come back to all this FIXME
		/*
		out << "// wait for threads and reduce when appropriate\n";
		out << "for (disp = 0; disp < " << nparts << "; ++disp) {\n";
		out << "pthread_join(threads[disp],NULL);\n";
		reIt = sgToReduce[sg].begin();
		for (; reIt != sgToReduce[sg].end(); ++reIt) {
			
			vertex u;
			// deterimine if label[0] += or label += is correct.
			bool ptr = false;
			if (sumtoSize[*reIt].compare("") != 0)
				ptr == true;
			//std::cout << *reIt << "\t" << (ptr ? "true" : "false") << "\n";
			
			string label;
			if (g.info(g.adj(*reIt)[0]).op == output) {
				u = g.adj(*reIt)[0];
				label = g.info(u).label;
			}
			else {
				// when there is reduction into the same structure work was performed
				// in, skip the reduction for the first thread, because the data
				// is already in place.
				out << "if (disp) {\n";
				
				u = *reIt;
				label = "t" + boost::lexical_cast<string>(u);
				ptr = true;
			}	
			
			if (g.info(u).t.k == scalar) {
				if (ptr)
					out << label << "[0] += t" << boost::lexical_cast<string>(*reIt)
						<< "[disp];\n";
				else
					out << label << " += t" << boost::lexical_cast<string>(*reIt)
			  			<< "[disp];\n";
			 }
			else {
				out << "for (ii = 0; ii < " << container_size(u,g) << "; ++ii)\n"
				 	 	<< label << "[ii] += t" << boost::lexical_cast<string>(*reIt)
			  			<< "[disp*" << container_size(*reIt,g) << "+ii];\n";
			}
			
			// end if(disp)
			if (g.info(g.adj(*reIt)[0]).op != output)
				out << "}\n";
		}
		out << "}\n\n";
		*/
	
		// clean up
		out << "free(mesg_" << sg->str_id << ");\n";
	}
	//out << "free(threads);\n";
	
	// handle any scalar outputs
	out << ptrOutputEnd;
 
  	out << "}\n";
  	
}
