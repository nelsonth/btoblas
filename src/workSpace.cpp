#include <string>
#include <iostream>
#include <fstream>
#include "boost/lexical_cast.hpp"

using namespace std;

int setUpWorkSpace (string &out_file_name, string &fileName,
					string &path, string &tmpPath, string &pathToTop,
                    bool deleteTmp) {
	// set up a temporary directory to place unique implementations.
	// when this fails return 1, on success return 0;
	
	// separate path and file name
	size_t pos = out_file_name.find_last_of('/');
	pos = pos == string::npos ? 0 : pos+1; 
	path.assign(out_file_name,0,pos);
	fileName.assign(out_file_name);
	fileName.erase(0,pos);
	
	// create a temporary working space for all versions
	// this is path of .m file + file name + _tmp
	tmpPath = path + fileName + "_tmp/";
	const char mode[2] = {'r','\0'};
	char buffer[100];
	FILE *tf = popen(("mkdir " + tmpPath + " 2>&1").c_str(), mode);
	if (tf == NULL) {
		std::cout << "Unable to make temporary working directory\n";
		std::cout << tmpPath << "\n";
		return 1;
	}
	// collect the directory creation status
	buffer[0] = '\0';
	while (fgets(buffer, 100, tf) != NULL);
	pclose(tf);
	if (buffer[0] != '\0') {
        if (!deleteTmp) {
            printf("%s\n", buffer);
            std::cout << "Do you want to continue? [y|n]: ";
            char cont;
            std::cin.get(cont);
            if (cont == 'N' || cont == 'n') {
                std::cout << "Remove this directory to continue\n";
                return 1;
            }
        }
		// otherwise remove the directory
		string tmpPathNoSlash = string(tmpPath,0,tmpPath.length()-1);
		FILE *tf = popen(("rm -rf " + tmpPathNoSlash + "*" + " 2>&1").c_str(), mode);
		if (tf == NULL) {
			std::cout << "Unable to remove temporary working directory\n";
			std::cout << tmpPathNoSlash << "\n";
			std::cout << "Attempting to continue\n";
		}
		// collect the delete status
		buffer[0] = '\0';
		while (fgets(buffer, 100, tf) != NULL);
		pclose(tf);
		if (buffer[0] != '\0') {
			printf("%s\n", buffer);
			std::cout << "Attempting to continue\n";
		}
		
		tf = popen(("mkdir " + tmpPath + " 2>&1").c_str(), mode);
		if (tf == NULL) {
			std::cout << "Unable to make temporary working directory\n";
			std::cout << tmpPath << "\n";
			return 1;
		}
		// collect the directory creation status
		buffer[0] = '\0';
		while (fgets(buffer, 100, tf) != NULL);
		pclose(tf);
		if (buffer[0] != '\0') {
			printf("%s\n", buffer);
			std::cout << "Attempting to continue\n";
		}
	}
	
	// copy timing header to working directory
	string timerPath = pathToTop + "src/timer.h";
	tf = popen(("cp -f " + timerPath + " " + tmpPath + " 2>&1").c_str(), mode);
	if (tf == NULL) {
		std::cout << "Unable to copy src/timer.h to temporary working directory "
		<< tmpPath << "\n";
		return 1;
	}
	// collect the copy status
	buffer[0] = '\0';
	while (fgets(buffer, 100, tf) != NULL);
	pclose(tf);
	if (buffer[0] != '\0') {
		printf("%s\n", buffer);
		std::cout << "Unable to copy src/timer.h to temporary working directory\n";
		return 1;
	}	
	
	// copy testing header to working directory
	string testUtilsPath = pathToTop + "src/testUtils.h";
	tf = popen(("cp -f " + testUtilsPath + " " + tmpPath + " 2>&1").c_str(), mode);
	if (tf == NULL) {
		std::cout << "Unable to copy src/testUtils.h to temporary working directory "
		<< tmpPath << "\n";
		return 1;
	}
	// collect the copy status
	buffer[0] = '\0';
	while (fgets(buffer, 100, tf) != NULL);
	pclose(tf);
	if (buffer[0] != '\0') {
		printf("%s\n", buffer);
		std::cout << "Unable to copy src/testUtils.h to temporary working directory\n";
		return 1;
	}
	
	
	// copy cblas header to working directory
	string cblasPath = pathToTop + "src/cblas.h";
	tf = popen(("cp -f " + cblasPath + " " + tmpPath + " 2>&1").c_str(), mode);
	if (tf == NULL) {
		std::cout << "Unable to copy src/cblas.h to temporary working directory "
		<< tmpPath << "\n";
		return 1;
	}
	// collect the copy status
	buffer[0] = '\0';
	while (fgets(buffer, 100, tf) != NULL);
	pclose(tf);
	if (buffer[0] != '\0') {
		printf("%s\n", buffer);
		std::cout << "Unable to copy src/cblas.h to temporary working directory\n";
		return 1;
	}
	
	// create makefile to run correctness and empirical tests
	std::ofstream out((tmpPath+"Makefile").c_str());
	out << "include " << pathToTop << "make.inc\n\n";
	out << "## VER should be gemver__2.c\n";
	out << "## [CE]TESTER should be gemver[CE]Tester.c\n";
	out << "## PREC should be either DREAL or SREAL\n";
	out << "## DEFS are optional, allows for command line defines\n";
	out << "##\tfor example: -DBTO_TILE\n";
	out << "##\n";
	out << "## make correctness VER=gemver__2.c CTESTER=gemverCTester.c PREC=DREAL\n";
	out << "## make empirical VER=gemver__2.c ETESTER=gemverETester.c PREC=DREAL\n";
	out << "## make empirical VER=gemver__2.c ETESTER=gemverETester.c PREC=DREAL DEFS=-DBTO_TILE\n";
	out << "## run with ./a.out\n";
	out << "##\n";
	out << "## build setting are controlled from top level directory - make.inc\n";
	out << "\n\n";
	out << "correctness : $(VER) $(CTESTER)\n";
	out << "\t@$(TCC) $(TFLAGS) $(CORRECT_INC) $(VER) $(CTESTER) -D$(PREC) $(DEFS)\n";
	out << "\n";
	out << "empirical : $(VER) $(ETESTER)\n";
	out << "\t@$(TCC) $(TFLAGS) $(EMPIRIC_INC) $(VER) $(ETESTER) -D$(PREC) $(DEFS)\n";
	out << "\n";
	out.close();
	
	return 0;
}


void handleBestVersion(string &fileName, string &path,
					   string &tmpPath, int bestVersion) {
	const char mode[2] = {'r','\0'};
	char buffer[100];
	bool mvError = false;
	FILE *tf = popen(("cp " + tmpPath + fileName + "__" + boost::lexical_cast<string>(bestVersion) 
					  + ".c " + path + fileName + ".c 2>&1").c_str(), mode);
	if (tf == NULL) {
		mvError = true;
		std::cout << "Unable to move final version\n";
		std::cout << tmpPath << "\n";
		return;
	}
	
	// collect the copy status
	buffer[0] = '\0';
	while (fgets(buffer, 100, tf) != NULL);
	pclose(tf);
	if (buffer[0] != '\0') {
		mvError = true;
		printf("%s\n", buffer);
		std::cout << "Unable to move final version;\n";
		return;
	}
	if (mvError) {
		std::cout << "\nThe best performing version can still be found at\n"
		<< tmpPath + fileName + "__" + boost::lexical_cast<string>(bestVersion) 
		+ ".c\n";
	}
	else {
		std::cout << "\nThe final routine is located here\n"
		<< path +  fileName + ".c\n";
		//std::cout << bestVersion << "\n";
	}
	
}



