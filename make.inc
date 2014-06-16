
## settings to build BTO ##
CC = gcc
CPP = g++

## settings to build empirical and correctness tests ##
TCC=gcc
TFLAGS=-w -O3 -msse3 -lpthread -lm 

ifeq ($(TCC),icc)
	TFLAGS += -vec_report=0
endif

## Include build requirements for BLAS library here.  Only the correctness
## tests require a BLAS to be present.
#CORRECT_INC = -I./ -L$(HOME)/CBLAS/lib/LINUX/ -L$(HOME)/BLAS/ -lcblas -lblas
CORRECT_INC = -I./ -lblas

## Build requirements for empirical tests
EMPIRIC_INC = 

