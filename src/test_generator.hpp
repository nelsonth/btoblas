#ifndef TEST_GEN_HPP
#define TEST_GEN_HPP

#include "syntax.hpp"
#include "modelInfo.hpp"
#include <list>

void createEmpiricalTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 bool noPtr, int reps);

void createMPIEmpiricalTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 bool noPtr, int reps);

void createEmpiricalTest_bto(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
						 bool noPtr, int reps);

void createCorrectnessTest(graph &g, std::string routine_name,
						 std::string out_file_name,
						 map<string,type*>const& inputs,
						 map<string,type*>const& outputs,
					     modelMsg &msg, bool noPtr);

int runEmpiricalTest(std::string &path, std::string &fileName,
					 std::list<versionData*> &versionList, 
					 int timeLimit, modelMsg &msg);

int runEmpiricalTest_bto(graph &g, std::string &path, std::string &fileName,
					 std::list<versionData*> &versionList, 
					 int timeLimit, modelMsg &msg,
						 std::string routine_name);

void runCorrectnessTest(std::string &path, std::string &fileName,
					 std::list<versionData*> &versionList);


#endif // TEST_GEN_HPP
