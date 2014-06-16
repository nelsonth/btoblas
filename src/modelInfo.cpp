#include "modelInfo.hpp"
#include <list>
#include <fstream>
#include <iostream>

using std::list;
using std::vector;

bool modelVersionSingle(graph &g, vector<model_typ> &models, int vid, string routine_name,
						modelMsg &msg, versionData *verData) {
	bool modelSucc = false;
	model_typ model;
	
	for (int mds = 0; mds < models.size(); ++mds) {
		
		model = models[mds];
		
		switch (model) {
			case analyticSerial:
				modelSucc |= evalGraphAS(g, vid, routine_name, msg, verData);
				break;
				
			case tempCount:
				modelSucc |= evalGraphTC(g, vid, routine_name, msg, verData);
				break;
				
			case analyticParallel:
				modelSucc |= evalGraphAP(g, vid, routine_name, msg, verData);
				break;
				
			case symbolic:
				//if (evalGraphSYM(*current.g, vid, routine_name, msg, verData))
				//	modelSucc = insertOrderedSYM(orderedVersions,msg,threshold,verData);
				//else
				modelSucc |= false;
				break;
				
			case noModel:
				modelSucc |= false;
				break;
				
			default:
				std::cout << "Unexpected model\n";
				modelSucc = false;
		}
	}
	return modelSucc;
}

void modelVersion(list<versionData*> &orderedVersions, versionData *verData, 
				  vector<model_typ> &models, graph &g, modelMsg &msg,
				  double threshold, int vid, string routine_name) {
	model_typ model;

	bool modelSucc = modelVersionSingle(g, models, vid, routine_name, msg, verData);
	
	if (!modelSucc) {
		if (models[0] != noModel)
			std::cout << "Model failure, keeping all versions\n";
		orderedVersions.push_back(verData);
	}
	else {
		// just taking first model for now...
		model = models[0];
		switch (model) {
			case analyticSerial:
				insertOrderedAS(orderedVersions,msg,threshold,verData);
				break;
				
			case tempCount:
				insertOrderedTC(orderedVersions,msg,threshold,verData);
				break;
				
			case analyticParallel:
				insertOrderedAP(orderedVersions,msg,threshold,verData);
				break;
				
			case symbolic:
				orderedVersions.push_back(verData);
				break;
				
			case noModel:
				orderedVersions.push_back(verData);
				break;
				
			default:
				std::cout << "Unexpected model\n";
				orderedVersions.push_back(verData);
		}					
	}
}

void cleanVersionList(list<versionData*> &li) {
	list<versionData*>::iterator i = li.begin();
	for (; i != li.end(); ++i) {
		(*i)->del();
		delete (*i);
	}
	li.clear();
}


versionData* findByVersion(list<versionData*> &li, int vid) {
	list<versionData*>::iterator i = li.begin();
	for (; i != li.end(); ++i) {
		if ((*i)->vid == vid)
			return (*i);
	}
	return NULL;
}


bool removeByVersion(list<versionData*> &li, int vid) {
	list<versionData*>::iterator i = li.begin();
	for (; i != li.end(); ++i) {
		if ((*i)->vid == vid) {
			li.erase(i);
			return true;
		}
	}
	return false;
}

string getModelName(model_typ model) {
	
	switch (model) {
		case analyticSerial:
			return "analytic-serial";
		case tempCount:
			return "temporary-count";
		case analyticParallel: 
			return "analytic-parallel";
		case empirical:
			return "empirical";
		case symbolic:
			return "symbolic";
		case noModel:
			return "no-model";
		default: 
			return "un-named";
	}
}

void printData(list<versionData*> &li, string fileName, bool toFile) {
	list<versionData*>::iterator i;

	std::ostream &out = std::cout;
	std::ofstream file;
	std::streambuf *backup;
	
	if (toFile) {
		backup = std::cout.rdbuf();
		file.open(fileName.c_str());
		out.rdbuf(file.rdbuf());
	}

	
	if (li.size() == 0)
		return;
	
	versionData *currV = *li.begin();
	int modelCnt = currV->modelCnt();
	
	vector<model_typ> models = currV->getAllModels();
	
	bool printHeaderForEachVersion = false;		// put header at start of each version
	bool separateVersions = false;				// put space between versions
	
	i = li.begin();
	for (i; i != li.end(); ++i) {
		currV = *i;
		
		if (separateVersions)
			out << "\n";

		map<int, vector<double> > data;
		vector<string> names;
		data.clear();
		names.clear();
		
		for (int j = 0; j < modelCnt; ++j) {
			(currV->get_model_by_name(models[j]))->extractData(data,names,getModelName(models[j]));
		}
		
		if (printHeaderForEachVersion || i == li.begin()) {
			if (names.size() > 0)
				out << "vid,size," << names[0];
		
			for (int j = 1; j < names.size(); ++j)
				out << "," << names[j];
			out << "\n";
		}
		
		map<int, vector<double> >::iterator dmItr = data.begin();
		for (; dmItr != data.end(); ++dmItr) {
			vector<double> &d = dmItr->second;
			
			if (d.size() > 0)
				out << currV->vid << "," << dmItr->first << "," << d[0];
			for (int j = 1; j < d.size(); ++j)
				out << "," << d[j];
			out << "\n";
		}
	}
	
	if (toFile) {
		file.close();
		std::cout.rdbuf(backup);
	}
	
}



