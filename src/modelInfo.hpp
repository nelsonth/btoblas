#ifndef VERINFO_HPP
#define VERINFO_HPP


#include <vector>
#include <map>
#include "syntax.hpp"
#include <list>

using std::vector;
using std::map;
using std::pair;
using std::list;

struct modelMsg;
class modelData;
class versionData;
#include "cost_estimate_common.hpp"
#include "memmodel_par/build_machine.h"

// available models
enum model_enum {analyticSerial, tempCount, analyticParallel, empirical, symbolic, noModel};
typedef enum model_enum model_typ;
string getModelName(model_typ model);

// info the model will need to return a cost
struct modelMsg {
	
	modelMsg() : start(1024), stop(2048), step(1024) ,
	threadDepth(0), tileDepth(0), pathToTop("./") {}
	
	modelMsg(int start, int stop, int step) : start(start),
	stop(stop), step(step), threadDepth(0), tileDepth(0),
	pathToTop("./") {}
	
	// range  of sizes to test
	int start;
	int stop;
	int step;	
	
	// subgraph info
	int threadDepth;
	int tileDepth;
	
	// path info
	string pathToTop;
	
	// architecture info
	struct machine* parallelArch;
	struct machine* serialArch;
};


// data from a single model for a single version
class modelData {
public:

	modelData() {data = vector<pair<int, vector <double> > > ();}
	
	void add_data(int size, double value) {
		vector<double> tmp;
		tmp.push_back(value);
		data.push_back(std::make_pair(size,tmp));
	}
	
	void add_dataSet(int size, vector<double> value) {
		data.push_back(std::make_pair(size,value));
	}
	
	// getting front or back will be the consistent across versions
	// because they will alway be added in the same order.
	// could add a get_by_size() if this were important.
	double get_back() {
		if (data.size() < 1)
			return -1.0;
		
		return data.back().second[0];
	}
	
	double get_front() {
		if (data.size() < 1)
			return -1.0;
		
		return data.front().second[0];
	}
	
	double get_back_avg() {
		if (data.size() < 1)
			return -1.0;
		
		double accum = 0.0;
		vector<double> &d = data.back().second;
		for (unsigned int i = 0; i <d.size(); ++i)
			accum += d[i];
		return accum/d.size();
	}
	
	void printWithSize() {
		for (unsigned int i = 0; i < data.size(); ++i) {
			if (data[i].second.size() <= 0)
				continue;
			
			std::cout << data[i].first;
			
			
			for (unsigned int j = 0; j < data[i].second.size(); ++j)
				std::cout << "," << data[i].second[j];
			
			std::cout << "\n";
		}
	}
	
	void extractData(map<int, vector<double> > &dataMap, vector<string> &names, string name) {
		
		if (data.size() < 1)
			return;
		
		for (unsigned int i = 0; i < data[0].second.size(); ++i) 
			names.push_back(name);
		
		for (unsigned int i = 0; i < data.size(); ++i) {
			if (data[i].second.size() <= 0)
				continue;
			
			for (unsigned int j = 0; j < data[i].second.size(); ++j)
				dataMap[data[i].first].push_back(data[i].second[j]);
		}
	}
	
	void del() {
		data.clear();
	}
	
	int dataSize() {
		return data.size();
	}
	
private:
	// multiple data points when more than one size is measured,
	// else single data entry.
	// each data point is (size, [data])
	vector<pair<int, vector <double> > > data;
};

// all data measured for a given version
class versionData {
public:
	vertex vid;
	
	versionData(vertex verID) : vid(verID) {}
	
	void add_model(model_typ modelName, modelData *data) {
		if (model.find(modelName) != model.end()) {
			std::cout << "Model by that name already exists\n";
		}
		model[modelName] = data;
	}
		
	// look for a model with a given name and return that
	// data or NULL for not found.
	modelData *get_model_by_name(model_typ modelName) { 
		map<model_typ, modelData*>::iterator it = model.find(modelName);
		if (it != model.end())
			return it->second;

		return NULL;
	}
	
	void del() {
		map<model_typ, modelData*>::iterator i;
		for (i = model.begin(); i != model.end(); ++i) {
			(*i).second->del();
			delete (*i).second;
		}
		model.clear();
	}
	
	int modelCnt() {
		return model.size();
	}	
	
	vector<model_typ> getAllModels() {
		map<model_typ, modelData*>::iterator it;
		vector<model_typ> names;
		for (it = model.begin(); it != model.end(); ++it) {
			names.push_back(it->first);
		}
		return names;
	}
	
private:
	// support more than one model and for each model
	// keep the pair
	//	(model name, model data)
	map<model_typ, modelData*> model;
};


void cleanVersionList(list<versionData*> &li);
versionData* findByVersion(list<versionData*> &li, int vid);
bool removeByVersion(list<versionData*> &li, int vid);
void printData(list<versionData*> &li, string fileName, bool toFile=false);
void modelVersion(list<versionData*> &orderedVersions, versionData *verData, 
				  vector<model_typ> &models, graph &g, modelMsg &msg,
				  double threshold, int vid, string routine_name);
bool modelVersionSingle(graph &g, vector<model_typ> &models, int vid, string routine_name,
						modelMsg &msg, versionData *verData);
#endif
