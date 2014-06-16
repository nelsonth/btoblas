The Build to Order BLAS User Manual 

by Geoffrey Belter, Ian Karlin, Jeremy G. Siek, Thomas Nelson and E.R. Jessup 
University of Colorado at Boulder 
  

1. Introduction 

The Build to Order (BTO) BLAS compiler takes a file that specifies a sequence of matrix and vector operations and creates an implementation in C tuned for memory efficiency.  The implementation may be serial or parallel using PThreads on Shared Memory Systems.  The BTO BLAS compiler enumerates all loop fusion opportunities to decrease memory traffic.  The compiler uses a combination of analytic modeling and empirical testing to quickly make optimization choices and ultimately produce the tuned C code.


2. Installation Instructions 

Currently the BTO compiler is UNIX specific because it relies on gettimeofday() function for timing.  The compiler has been used and tested on both Linux and Mac OSX systems.


1. Download the BTO BLAS distribution file bto.tar.gz. 
2. Uncompress and un-archive the distribution file using the following command: tar -zxvf bto.tar.gz 
3. To compile the BTO BLAS, type ./install.sh in the bto directory. This creates an executable named btoblas in the bin subdirectory. The installation process takes a few minutes because it runs benchmarks to determine hardware characteristics that are necessary for choosing between optimizations.


3. Creating an Input File 

The input to the compiler is a kernel specification file that includes a description of the input and output parameters and a description of a sequence of linear algebra operations. These operations are written using the matrix arithmetic syntax of MATLAB.  Examples of such files can be found in the matlabKernels subdirectory.  Below is an example kernel. 

BiCGK
in
  A : column matrix, p : vector, r : vector
out
  q : vector, s : vector
{
  q = A * p
  s = A' * r
} 
The first line of the kernel is its name. Next is a list of parameters organized into three sections: in, inout, and out. Each section contains a list of parameters and their types separated by commas. Each of the three sections is optional. The inout section is for parameters that are both read from and written to. 

Parameters can have the following types: matrix, vector and scalar.  The matrix and vector types can have an optional orientation specifier of row or column.  Parameters default to column when no specifier is given.  

The specification of the matrix arithmetic is enclosed in braces and consists of a sequence of assignment statements. Expressions follow MATLAB conventions with ' for transpose, * for multiplication, and the variable on the left side of the equal sign (=) is assigned the result of the calculation on the right side.  
3.1 Restrictions and Common Errors 

Addition and subtraction requires both operands to be of the same type and have the same orientation.  For example you can add two row major matrices, but you cannot add a row matrix to a column matrix.  Likewise you can add two column vectors, but you cannot add a column vector and a row vector. Additionally the result type must match the operand types.  For example, result of subtracting two column matrices is a column matrix, not a row matrix.  If these conditions are not met, the compiler will exit with an error. 

Multiplication is restricted to the following patterns.

scalar = scalar * scalar

scalar = row vector * column vector 
matrix = matrix * matrix 
The matrix-matrix multiply handles any combination of orientations so, for example, you can multiply a row major matrix with a column major matrix and produce a column matrix. 

The following is an example of a combination that does not make sense because the dimensions do not match up. That is, a column vector can be thought of having dimension n x 1, so with (n x 1) * (n x 1), the two inner dimensions, 1 and n, do not match. 
scalar = column vector * column vector.


4. Running the Compiler 

To run the compiler type ./bin/btoblas «inputfile» in the bto directory.  The compiler takes one mandatory argument of an input file with suffix .m that specifies a linear algebra kernel as described in Section 3. The output is a file with the same name as the input file, but with suffix .c, and contains an implementation of the specified kernel in the C programming language. The output file is placed in the same directory as the input file. The optional arguments to btoblas are described below.  

4.1 Options 

-h [ --help ]	Will bring up a help menu with the below options.	
-p [ --precision ] precision 	Set the precision type for scalars in the generated output (float or double). The default is double.	
-t [ --threshold ] fraction 	This parameter controls how much empirical testing is performed. For example, the default of 0.01 says that empirical testing will be performed on any version that is predicted to be within %1 the performance of the best predicted version. 	
-l [ --limit ] seconds 	Specifies a time limit in seconds for any empirical search.  Default is unlimited time. 	
-m [--model_off]	Do not use analytic model when testing kernels. If set the compiler will empirically tests all of its optimization choices.  For even moderately complicated computations this can take a long time.  When the model is disabled, the best performance is decided only by empirical testing.  Disabling this and empirical testing will generate many versions, but will not attempt to determine the best performing.	
-e [--empirical_off] 	Disable empirical testing.  Empirical testing is enabled by default.  When empirical testing is disabled, best performance is decided only by analytic model.  Disabling the model and empirical testing will generate many versions, but will not attempt to determine the best performing.  	
-c [--correctness] 	Enable correctness testing.  Correctness testing is disabled by default.  When enabled, the input program is converted to a set of very naive BLAS calls which BTO results are compared to.  This requires having and working BLAS with cblas entry points.  See "Correctness Testing" section for further details. 	
-r [--test_param] start:stop:step 	Set parameters for empirical and correctness testing as: 
         start:stop:step
This directs the tests to use matrix order and vector sizes between start and stop values using a step size of step. See "Testing" section for additional details. 	
-p [--partition_off] 	Disable partitioning.  Partitioning is enabled by default and generates parallel code that requires Pthreads. 	
-b [--backend] name 	Select the code generator.  Two C code generators are currently available.  One generates C code with pointers and the other generates C code using variable length arrays (C99).
       ptr     : generate pointer code
       noptr  : generate VLA code 	

4.2 Option Pitfalls and Suggestions 

It is possible to create option combinations that generate unwanted results.  The default settings are for routines with operations that contain O(N^2) data and calculations.  For routines that contain less data and/or calculations we suggest using the -r option to increase the size of the calculation modeled and tested.  

Also, turning off both empirical testing and the model using the -e and -m flags will result in the compiler being unable to determine which routine is best and only one of these flags should be used at a time. 

5. Calling the Generated Kernel 

The output file contains one function that implements the specified linear algebra kernel. The name of the C function is the name specified in the input file. The parameters of the C function correspond to the input and output parameters in the following way. For each matrix parameter, there are three parameters to the C function, a pointer to the first element of the matrix, the two integers: the number of rows, and the number of columns. For each vector parameter, there is a pointer to the first element of the vector and an integer that specifies the size of the vector. Each matrix and vector must be represented as a contiguous block of memory. The following is the function declaration for the BiCGK kernels given in Section 3. 
void BiCGK(double* A, int A_nrows, int A_ncols,
           double* p, int p_nrows,
           double* r, int r_nrows,
           double* q, int q_nrows,
           double* s, int s_nrows);

Some of the size parameters are redundant. For example, in the BiCGK kernel, the p_nrows parameter should be the same as A_ncols because the kernel multiplies matrix A by vector p. In a future edition of the compiler we will generate parameter lists that do not include redundant parameters. The following is a simple example use of the above BiCGK function. 

#include «stdio.h»

void BiCGK(double* A, int A_nrows, int A_ncols, double* p, int p_nrows, double* r, int r_nrows, double* q, int q_nrows, double* s, int s_nrows);


int main(int argc, char* argv[]) 
{ 
  double A[] = { 1, 1, 1, 
                 1, 1, 1, 
                 1, 1, 1 }; 
  int m = 3; 
  int n = 3; 
  double p[] = { 1, 2, 3 }; 
  double r[] = { 4, 5, 6 }; 

  double q[] = { 0, 0, 0}; 
  double s[] = { 0, 0, 0}; 
  int i; 

  BiCGK(A, m, n, p, n, r, m, q, m, s, n); 

  printf("q: "); 
  for (i = 0; i != m; ++i) 
    printf("%f ", q[i]); 
  printf("\n"); 

  printf("s: "); 
  for (i = 0; i != n; ++i) 
    printf("%f ", s[i]); 
  printf("\n"); 
} 
The output of this example is 
q: 6.000000 6.000000 6.000000 
s: 15.000000 15.000000 15.000000 



6. Testing 


BTO BLAS has an empirical tester which measures execution time using gettimeofday, and a correctness tester which can be enabled to verify generated code.  The empirical tester is enabled by default and used to identify the best set of optimizations.  The correctness tested is disabled by default, but is enabled with the command line flag -c. 

Both of these tests have a set of user definable features including compiler, compiler flags, libraries, and include paths.  These values are set in the top level directory make.inc file (bto/make.inc).  Shared among both tests is compilers and flags.  These are set in make.inc as TCC and TFLAGS.  Any other compilation command line flags specific to either test can be set using CORRECT_INC or EMPIRIC_INC. 

Sizes used in these testers is set for each run of BTO using the -r flag.  This flag requires start:stop:step to be specified.  The use of these sizes is explained in the following subsections for each type of test. 


6.1 Empirical Testing 

Empirical testing is enabled by default.  This test can be disabled using the command line flag -e.  

Beyond the compilation controls found in make.inc, a user should specify appropriate size information using the -r flag.  The default value for the -r flag is 3000:3000:1 which which will empirically test using square matrices of order 3000 and vectors of size 3000.  This will give valid empirical times for many matrix vector operations but may not be large enough for vector-vector operations, or very large machines. 

Currently the empirical tester only handles square matrices and all vectors sizes and matrix orders are the same. 

Currently the empirical tester uses the largest available size for determining best performance.  For example if -r500:1000:500 is specified, the empirical test will be run for sizes of 500 and 1000, but only the performance from 1000 will be used.  For compilation speed it would be best to specify that as -r1000:1000:1. 

The empirical tester is available in a temporary working directory.  If PATH/bicg.m was just run, the empirical tester is located here: PATH/bicg_tmp/bicgETester.c 


6.2 Correctness Testing 

Correctness testing is disabled by default.  This test can be enabled using the command line flag -c.  Enabling this test requires that a BLAS library with cblas entry points be included in bto/make.inc using CORRECT_INC.  If a publicly available BLAS is present on a machine, it is likely that the following will work. 
CORRECT_INC = -I./ -lblas 
In some cases cblas must be specified explicity 
CORRECT_INC = -I./ -lcblas -lblas 
If BLAS is located in the users home directory 
CORRECT_INC = -I./ -L$(HOME)/BLAS/ -lcblas -lblas 

The input program is converted to naive cblas calls.  The sequence of calls is not an ideal use of BLAS and performance comparison against the correctness tester would not be a fair comparison to BLAS. 

The correctness tester runs only on for those tests that would otherwise be empirically tested.  This can be controlled using the -t flag.  Setting -t 1 will test all versions produced.  If -t is not used, model settings and differences in input programs will determine what is sent to the correctness tester.


Beyond the settings in make.inc, a user should specify appropriate size information using the -r flag.  The correctness tester will test sizes within the specified range including rectangular matrices.  For example, 
./bin/btoblas bicg.m -t1 -c -r 25:100:25 
This will test all versions produced for bicg.m using combinations of sizes 25, 50, 75, and 100.  The combinations are determined based on the input program.  For given a simple input program that adds matrices, there will be two common sizes, M and N.  The test generator will look like: 
for (M = 25; M «= 100; M += 25) 
    for (N = 25; N «= 100; N += 25) 
        perform test 

The correctness tester is available in a temporary working directory.  If PATH/bicg.m was just run, the correctness tester is located here: PATH/bicg_tmp/bicgCTester.c 




7. For More Information Please See the Following


The BTO BLAS project has generated numerous publications avalible on our website http://ecee.colorado.edu/wpmu/btoblas/.  If you wish to cite the project the following papers contain the most recent and comprehensive overview of the project.  In particular please read the below publications if you wish to learn details about the algorithms and software design used in the project. 

If citing the project in general and for details about the serial compiler, model and how the two interact please read: 

Geoffrey Belter, Ian Karlin, Elizabeth Jessup, Jeremy Siek. Automating the Generation of Composed Linear Algebra Kernels[pdf]. In SC09: the International Conference on High Performance Computing, Networking, Storage, and Analysis. November, 2009.


For information about parallel code generation please read: 

G. Belter, J. G. Siek, I. Karlin and E. R. Jessup.  Automatic Generation of Tiled and Parallel Linear Algebra Routines: A partitioning framework for the BTO Compiler. In the Fifth International Workshop on Automatic Performance Tuning (iWAPT‘10), Berkeley, CA, June 2010, pp. 1-15. 

For information about the parallel memory model please read: 

I. Karlin, E. R. Jessup, G. Belter and J. G. Siek.  Parallel Memory Prediction for Fused Linear Algebra Routines. In the 1st International Workshop on Performance Modeling, Benchmarking and Simulation of High Performance Computing Systems (PBMS 10), New Orleans, LA, November 2010, pp. 1-8.



Appendix 

A. Syntax and Grammar 
The syntax for parameters is given by the following grammar: 
NAME  = [a-Z][a-Z0-9]*

orientation ::= "row" | "column" 
type ::= "scalar" | [orientation] "matrix" | [orientation] "vector" 
parameter ::= NAME ":" type 
parameter_list ::= parameter | parameter "," parameter_list 
The specification of the matrix arithmetic is enclosed in braces and consists of a sequence of assignment statements. Expressions follow MATLAB conventions with ' for transpose, * for multiplication, and the variable on the left side of the equal sign (=) is assigned the result of the calculation on the right side.  The grammar for the subset of MATLAB is shown below. 
NUM = [0-9]+

body ::= "{" stmt* "}"

stmt ::= NAME "=" expr

expr ::= NUM 

        | NAME 
        | expr "+" expr 
        | expr "-" expr 
        | expr "*" expr 
        | "-" expr 
        | expr "'" 
        | "(" expr ")" 
The grammar for the entire input file is as follows. 
file ::= NAME ["in" parameter_list] ["inout" parameter_list] ["out" parameter_list] body