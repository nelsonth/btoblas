#include "translate_to_code.hpp"
#include "translate_utils.hpp"
#include "modelInfo.hpp"
#include <list>

using std::list;

#include "cost_estimate_common.hpp"

bool evalGraphTC(graph &g, int vid, string root, modelMsg &mMsg, 
				 versionData *verData) {
	// create the data structure
	modelData *newData = new modelData();
	verData->add_model(tempCount,newData);
	
	unsigned int tds = 0;
	int tmpCnt = 0;
	for (;tds < g.num_vertices(); ++tds) {
		if (g.info(tds).op == temporary 
			|| (g.info(tds).op == sumto 
				&& g.info(tds).t.k != scalar))
			tmpCnt++;
	}
	
	for (int start = mMsg.start; start <= mMsg.stop; start += mMsg.step)
		newData->add_data(start,(double)tmpCnt);
	
	return true;
}

bool insertOrderedTC(list<versionData*> &verList, modelMsg &mMsg,
					 double threshold, versionData *verData) {

	if (verList.size() == 0) {
		verList.push_back(verData);
		return true;
	}
	
	versionData *currBest = verList.front();
	modelData *bestTmp = currBest->get_model_by_name(tempCount);
	modelData *newTmp = verData->get_model_by_name(tempCount);
	
	if (bestTmp == NULL || newTmp == NULL) {
		std::cout << "Error in the temporary structure count model\n";
		verList.push_back(verData);
		return false;
	}
	
	int numTmps = (int)bestTmp->get_front();
	int tmpCnt = (int)newTmp->get_front();
	
	if (threshold == 1.0) {
		verList.push_back(verData);
	}
	else if (tmpCnt < numTmps) {
		// if a new low number of tmps has been found remove
		// everything else and keep only the new low version
		cleanVersionList(verList);
		verList.push_back(verData);
	}
	else if (tmpCnt == numTmps)
		verList.push_back(verData);
	
	return true;
}

