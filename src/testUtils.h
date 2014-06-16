
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "cblas.h"

#ifdef SREAL
#define REAL
#define PTYPE float
#define ERR 0x1.0p-21f

#elif defined (DREAL)
#define REAL
#define PTYPE double
#define ERR 0x1.0p-50f

#elif defined (SCPLX)
#define CPLX
#define PTYPE float
#define ERR 0x1.0p-21f

#elif defined (DCPLX)
#define CPLX
#define PTYPE double
#define ERR 0x1.0p-50f

#endif

#define END_EARLY	// stop at first failure
//#define USE_ONE	// all generated values will be 1
#define OUT stderr // stderr or stdout

//////////////////////// general utilities ////////////////////////
#ifdef TESTFROM
#include "dlfcn.h"
void testingFrom() {
	Dl_info info;
    dladdr((const void*)cblas_ddot, &info);
    printf("Testing from %s\n", info.dli_fname);
}
#endif

void reportStatus(int testCnt, int failedCnt, char *mesg) {

	// testCnt - number of tests
	// failedCnt - number that failed
	// mesg - optional message
	
	if (!failedCnt)
		printf("\tPASSED %d tests\n",testCnt);
	else {
#ifdef END_EARLY
		printf("\tEND_EARLY (testUtils.h) is set; stopping tests at first failure\n");
#endif
		printf("\tFAILED %d tests of %d total\n",failedCnt,testCnt);
	}
	
	if (mesg != NULL)
		printf("\t** %s **\n",mesg);
	
}

/////////////////////// precision specific utilities ///////////////

PTYPE val() {
#ifdef USE_ONE
	return 1.0;
#else
	return 1.0+(PTYPE)rand()/RAND_MAX;
#endif
}

int checkError(PTYPE testResult, PTYPE refResult, long int ops) {
	// determine if error is within expected ranges
	// ops : number of operations required to get result
	// tid : test id

	//printf("%f\t%f\n",testResult,refResult);
	if (ops < 0) {
		fprintf(OUT,"\tWARNING: check error tolerance, overflow in number of ops\n");
		fprintf(OUT,"\ttestReal.h; checkError()\n");
	}
	
    if (testResult == refResult) return 0; // Early out if error == zero
	PTYPE absoluteError = fabs(testResult - refResult);
    
    PTYPE scale = fabs(testResult) > fabs(refResult) ? fabs(testResult) : fabs(refResult);
    
    PTYPE relativeError = absoluteError / scale;
	
	if (relativeError > ERR*ops || isnan(absoluteError)) {
#ifndef SUMMARY
		fprintf(OUT,"FAILED with a larger than expected error.\n");
		fprintf(OUT,"\tExpected result was %a (%g)\n", refResult, refResult);
		fprintf(OUT,"\tObserved result was %a (%g)\n", testResult, testResult);
		fprintf(OUT,"\tTolerated relative error is %g\n", ERR*ops);
		fprintf(OUT,"\tObserved relative error was %g\n", relativeError);
#endif
		return 1;
	}
	return 0;
}

int checkMatrix(PTYPE *testResult, PTYPE *refResult, int m, int n, int ld, 
				enum CBLAS_ORDER order, long int ops) {
	// compare two matrices, both working set and padding
	// return 1 for fail; 0 for pass
	// m : number of rows
	// n : number of column
	// ld : leading dimension
	// order : row or column major
	// ops : number of operations required to get result
	
	int i, j, ii, jj;
	int padding = 0;		// set to if padding exists
	if (order == CblasRowMajor) {
		i = m; j = n;
		if (n != ld) padding = 1;
	}
	else {
		i = n; j = m;
		if (m != ld) padding = 1;
	}
	--i; --j;
	
	// check the matrix
	ii = i;
	for (;ii>=0;--ii) {
		jj = j;
		PTYPE *test = testResult + ii * ld;
		PTYPE *ref = refResult + ii * ld;
		for (;jj>=0;--jj) {
			if (checkError(test[jj],ref[jj],ops)) {
#ifndef SUMMARY
				fprintf(OUT,"\tMatrix failed at (%d,%d)\n",ii,jj);
#endif
				return 1;
			}
		}
	}
	
	if (!padding)
		return 0;
	
	//check any padding
	ii = i;
	for (;ii >= 0;--ii) {
		jj = ld-1;
		PTYPE *test = testResult + ii * ld;
		PTYPE *ref = refResult + ii * ld;
		for (;jj > j; --jj) {
			if (test[jj] != ref[jj]) {
#ifndef SUMMARY
				fprintf(OUT,"\tFAILED with matrix padding corruption.\n");
#endif
				return 1;
			}
		}
	}
	return 0;
}


int checkVector(PTYPE *testResult, PTYPE *refResult, int m, int inc, 
				int ops) {
	// compare two vectors; both data and space inbetween data when inc != [+-]1
	// return 1 for fail; 0 for pass
	// m : size
	// inc : increment
	// ops : number of operations required to get result
	
	int i;
	
	int indx = 0;
	int dir = 1;
	if (inc < 0) {
		indx = -inc*(m-1);
		dir = -1;
	}
	
	int next = indx + inc;
	
	// check the vector
	for (i=0;i<m;++i) {
		if (checkError(testResult[indx],refResult[indx],ops)) {
#ifndef SUMMARY
			fprintf(OUT,"\tVector failed at index %d\n",indx);
#endif
			return 1;
		}
		indx += dir;
		while (indx != next && indx >= 0) {
			if (testResult[indx] != refResult[indx]) {
#ifndef SUMMARY
				fprintf(OUT,"FAILED - vector with non unit increment corruption.\n");
#endif
				fprintf(OUT,"%d\n",indx);
				return 1;
			}
			indx += dir;
		}
		next += inc;
	}
	
	return 0;
}

void printMatrix(enum CBLAS_ORDER order, PTYPE *A, int m, int n, int lda) {
	int outer, inner;
	if (order == CblasColMajor) {
		outer = 1;	inner = lda;
	}
	else {
		outer = lda;	inner = 1;
	}
	
	int i,j;
	for (i=0;i<m;++i) {
		PTYPE *tA = A + i * outer;
		for (j=0;j<n;++j) {
			printf("%.2f ",tA[j*inner]);
		}
		printf("\n");
	}
	printf("\n");
}
