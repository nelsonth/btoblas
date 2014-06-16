//%error-verbose
%{

#include <iostream>
#include <fstream>
#include <map>
#include "boost/lexical_cast.hpp"
#include "syntax.hpp"
#include "syntax.tab.hpp"
#include "compile.hpp"
#include "boost/program_options.hpp"
#include <string>

int yylex();
extern int yylineno;
void yyerror(std::string s);

std::string out_file_name;
std::string input_file;
extern FILE *yyin; 
boost::program_options::variables_map vm;

    void handleError(std::string errorMsg);
    extern int bto_yy_input(char *b, int maxBuffer);
    
    static int eof = 0;
    static int nRow = 0;
    static int nBuffer = 0;
    static int lBuffer = 0;
    static int nTokenStart = 0;
    static int nTokenLength = 0;
    static int nTokenNextStart = 0;
    static int lMaxBuffer = 1000;
    static char *buffer;
%}

%union {
  expr* expression;
  stmt* statement;
  std::vector<stmt*>* program;
  std::string* variable;
  param* parameter;
  double number;
  std::map<string,type*>* parameter_list;
  type* value_type;
  storage store;
  std::string *orien;
  attrib *attribute;
  std::map<string,string>* attribute_list;
}

%type <expression> expr
%type <statement> stmt
%type <program> prog
%type <parameter> param
%type <parameter_list> param_list
%type <value_type> type
%type <attribute> attrib
%type <attribute_list> attrib_list

%start input
%token NEG TIC IN INOUT OUT VECTOR MATRIX TENSOR SCALAR SQUAREROOT
%token ORIENTATION ROW COLUMN CONTAINER
%token FORMAT GENERAL TRIANGULAR
%token UPLO UPPER LOWER
%token DIAG UNIT NONUNIT
%token <variable> VAR
%token <number> NUM
%left '-' '+'
%left '*'
%nonassoc IN INOUT OUT
%nonassoc NEG     /* negation--unary minus */
%nonassoc TIC    /* transpose */

%%
input: VAR IN param_list INOUT param_list OUT param_list '{' prog '}' 
		{ compile(vm, out_file_name, *$1, *$3, *$5, *$7, *$9); }
	| VAR IN param_list INOUT param_list '{' prog '}'
		{ std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *$1, *$3, *$5, *tmp, *$7); }
	| VAR INOUT param_list OUT param_list '{' prog '}'
		{ std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *$1, *tmp, *$3, *$5, *$7); }
	| VAR IN param_list OUT param_list '{' prog '}'
		{ std::map<string,type*> *tmp = new std::map<string,type*>(); 
		compile(vm, out_file_name, *$1, *$3, *tmp, *$5, *$7); }
    | VAR IN error INOUT { handleError("Error parsing input variables"); }
    | VAR IN error OUT { handleError("Error parsing input variables"); }
    | VAR IN param_list INOUT error OUT { handleError("Error parsing inout variables"); }
    | VAR IN param_list INOUT error '{' { handleError("Error parsing inout variables"); }
    | VAR IN param_list INOUT param_list OUT error '{' { handleError("Error parsing out variables"); }
    | VAR IN param_list OUT error '{' { handleError("Error parsing out variables"); }
    | VAR error '{' { handleError("Error parsing out variables"); }
;
param: VAR ':' type { $$ = new param(*$1,$3); }
    | VAR ':' error { handleError("Error parsing variable declaration"); }
    | VAR error type { handleError("Error: missing \':\' in this declaration"); }
;
param_list: param { $$ = new std::map<string,type*>(); $$->insert(*$1); }
        | param_list ',' param { $$ = $1; $$->insert(*$3); }
        | param_list ',' error { handleError("Error parsing parameters"); }
; 
type: MATRIX '(' attrib_list ')' { $$ = new type("matrix",*$3); }
    | VECTOR '(' attrib_list ')' { $$ = new type("vector",*$3); }
	| TENSOR '(' attrib_list ')' { $$ = new type("tensor",*$3); }
    | SCALAR { $$ = new type(scalar); }
    | MATRIX error { handleError("Error parsing matrix attributes"); }
    | VECTOR error { handleError("Error parsing vector attributes");}
    | TENSOR error { handleError("Error parsing vector attributes");}
;
attrib: ORIENTATION '=' ROW { $$ = new attrib("orientation","row"); }
    | ROW { $$ = new attrib("orientation","row"); }
    | ORIENTATION '=' COLUMN { $$ = new attrib("orientation","column"); }
	| CONTAINER '=' VAR { $$ = new attrib("container", *$3); }
    | COLUMN { $$ = new attrib("orientation","column"); }
    | FORMAT '=' GENERAL { $$ = new attrib("format","general"); }
    | FORMAT '=' TRIANGULAR { $$ = new attrib("format","triangular"); }
    | GENERAL { $$ = new attrib("format","general"); }
    | TRIANGULAR { $$ = new attrib("format","triangular"); }
    | UPLO '=' UPPER { $$ = new attrib("uplo","upper"); }
    | UPLO '=' LOWER { $$ = new attrib("uplo","lower"); }
    | UPPER { $$ = new attrib("uplo","upper"); }
    | LOWER { $$ = new attrib("uplo","lower"); }
    | DIAG '=' UNIT { $$ = new attrib("diag","unit"); }
    | DIAG '=' NONUNIT { $$ = new attrib("diag","nonunit"); }
    | UNIT { $$ = new attrib("diag","unit"); }
    | NONUNIT { $$ = new attrib("diag","nonunit"); }
;
attrib_list: attrib { $$ = new std::map<string,string>(); $$->insert(*$1); }
    | attrib_list ',' attrib { $$ = $1; $$->insert(*$3); }
;
prog: { $$ = new std::vector<stmt*>(); }
    | prog stmt { $$ = $1; $$->push_back($2); }
    | prog error {handleError("Error in program body"); }
;
stmt: VAR '=' expr { $$ = new stmt($1,$3); }
    | VAR '=' error { handleError("Error parsing operations"); }
;
expr: NUM { $$ = new scalar_in($1); }
    | VAR { $$ = new variable($1); }
    | expr '+' expr { $$ = new operation(add, $1, $3); }
    | expr '-' expr { $$ = new operation(subtract, $1, $3); }
    | expr '*' expr { $$ = new operation(multiply, $1, $3); }
    | '-' expr %prec NEG { $$ = new operation(negate_op, $2); }
    | expr '\'' %prec TIC { $$ = new operation(trans, $1); }
    | '(' expr ')' { $$ = $2; }
    | SQUAREROOT '(' expr ')' { $$ = new operation(squareroot, $3); }
    | expr '/' expr { $$ = new operation(divide, $1, $3); }
    | '|' expr '|' { $$ = new operation(squareroot, new operation(multiply, new operation(trans, $2), $2)); }
;
%%
int main(int argc, char *argv[]) 
{
    // buffer to keep current line copy while parsing for 
    // improved error handling
    buffer = (char*)malloc(lMaxBuffer);
    if (buffer == NULL) {
        printf("cannot allocate %d bytes of memory\n", lMaxBuffer);
        fclose(yyin);
        return 12;
    }
    
  namespace po = boost::program_options;
  
  string config_file;
  
  po::options_description cmdOnly("Command Line Only Options");
  cmdOnly.add_options() 
        ("help,h","this help message")
        ("config,f", po::value<std::string>(&config_file)->default_value(""),
                "Name of configuration file.")
        ;
  
  po::options_description vis("Options");
  vis.add_options()
    
		("precision,a",po::value<std::string>()->default_value("double"),
  				"Set precision type for generated output [float|double]\n"
  				"  double is default")
		("empirical_off,e","Disable empirical testing.  Empirical testing\n"
				"is enabled by default.")
		("correctness,c","Enable correctness testing.  Correctness testing\n"
				"is disabled by default.  Enabling requires a BLAS library\n"
				"to be present.  (Set path in top level make.inc. See \n"
				"documentation for further details")
		("use_model,m","Enable the analytic model when testing kernels. If "
				"set the compiler will use a memory model to help with "
				"optimization choices.  Not recommended for most users. ")
		("threshold,t",po::value<double>()->default_value(0.01),
				"This parameter controls how much empirical testing is "
				 "performed. For example, the default of 0.01 says that "
				"empirical testing will be performed on any version "
				"that is predicted to be within %1 the performance of "
				"the best predicted version.  A value of 1 will "
				"rank all versions. A value of 0 will select only the "
				"best version. Only used if use_model is on.")
		//("level1",po::value<std::string>()->default_value("thread 2:12:2"),
		("level1",po::value<std::string>()->default_value(""),
				"Choose thread or cache tiling for outer level, "
				"and input parameter search range. "
				"Example \"thread 2:12:2\" or \"cache 64:512:8\".")
		("level2",po::value<std::string>()->default_value(""),
				"Choose thread or cache tiling for inner level, "
				"and input parameter search range. "
				"Example \"thread 2:12:2\" or \"cache 64:512:8\".")
		("test_param,r",po::value<std::string>()->default_value("3000:3000:1"),
  				"Set parameters for empirical and correctness tests as\n"
  				"start:stop:step")
		("search,s",po::value<std::string>()->default_value("ga"),
				"select the search strategy:\n"
				"  [ga|ex|random|orthogonal|debug|thread] \n"
				"    ga is the default \n"
				" ga: genetic algorithm \n"
				"  orthogonal: \tsearch one parameter completely first, then"
				" search next.  For example find best fusion, and then find"
				" best partition strategy\n"
				"  random: \trandomly search the space\n"
				"  exhaustive: \texhaustively search the space (very long run times)\n"
				"thread: thread search only \n"
				"debug: use the versions specified in debug.txt \n")
		("ga_timelimit",po::value<int>()->default_value(10),
				"Specifies the number of GA versions to be tested")
		("empirical_reps",po::value<int>()->default_value(5),
				"the number of empirical test repetitions for each point")
        ("delete_tmp","delete *_tmp directory without prompting")
		("ga_popsize",po::value<int>()->default_value(10),"population size")
		("ga_nomaxfuse","genetic algorithm without max fuse")
		("ga_noglobalthread","genetic algorithm without global thread search")
		("ga_exthread", "genetic algorithm with additional exhaustive thread search")
  		;
  
  po::options_description hidden("Hidden options");
  hidden.add_options()
		("distributed,d","Enable distributed calculation via MPI.")
  		("input-file", po::value<std::string>(), "input file")
		("run_all","allow compiler to run with model and empirical tests off")
		("limit,l",po::value<int>()->default_value(-1),
				"Specifies a time limit in minutes for the search "
				"Default is unlimited time.")
		("partition_off,p","Disable partitioning.  Enabled by default, generates\n"
				"parallel code that requires a Pthreads library.")
		("backend,b",po::value<std::string>()->default_value("ptr"),
				"select the code generation backend:\n"
				"  [ptr|noptr] \tptr is default\n"
				"  ptr: \tproduce c code using pointers\n"
				"  noptr: \tproduce c code using variable length arrays\n"
				"\nSelecting noptr requires partitioning to be disabled")
  		;
  po::options_description cmdline_ops;
  cmdline_ops.add(cmdOnly).add(vis).add(hidden);
  
  po::options_description config_file_options;
  config_file_options.add(vis).add(hidden);
  
  po::positional_options_description p;
  p.add("input-file", -1);
  
  try {
    
  po::store(po::command_line_parser(argc,argv).options(cmdline_ops).positional(p).run(), vm);
  po::notify(vm);

  if (vm.count("help") || !(vm.count("input-file"))) {
  	std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
  	return 1;
  }	
  
  if (!config_file.empty()) {
	  std::fstream ifs(config_file.c_str());
	  if (!ifs) {
		std::cout << "Can not open config file: " << config_file << "\n";
		return 1;
	  } else {
		store(parse_config_file(ifs, config_file_options), vm);
	  }
  }
   
  if (!(vm["precision"].as<std::string>().compare("float") == 0 ||
  			vm["precision"].as<std::string>().compare("double") == 0)) {
  		std::cout << "ERROR:" << std::endl;
  		std::cout << "\t" << vm["precision"].as<std::string>() << " is not a valid "
  				  << "option for --precision (-p)" << std::endl;
  		std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis
  				  << std::endl;
  		return 1;		
  }

  if (!(vm["backend"].as<std::string>().compare("ptr") == 0 ||
			vm["backend"].as<std::string>().compare("noptr") == 0)) {
		std::cout << "ERROR:" << std::endl;
        std::cout << "\t" << vm["backend"].as<std::string>() << " is not a valid option "
                  << "for --backend (-b)" << std::endl;
        std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
        return 1;

  }
  
  if ((vm["backend"].as<std::string>().compare("noptr") == 0) && !vm.count("partition_off")) {
	std::cout << "ERROR:" << std::endl;
    std::cout << "\t" << "Cannot enable partitioning with noptr backend selected.\n";
    std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
    return 1;
  }

	//if (vm.count("model_off") && vm.count("empirical_off")  && !vm.count("run_all")) {
		//std::cout << "ERROR:" << std::endl;
	    //std::cout << "\t" << "Either model or empirical testing or both required.\n";
	    //std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
	    //return 1;
	//}

  input_file = vm["input-file"].as<std::string>();
  } catch (std::exception &e) {
    std::cout << e.what() << "\n";
    return 1;
  }
  
  out_file_name = input_file;
  size_t loc = out_file_name.rfind(".m");
  if (loc != string::npos)
  	out_file_name.replace(loc,2,"");
  else
  	std::cout << "WARNING:\n\tinput file \"" <<  input_file << "\" may not be a .m file\n";
  
  yyin = fopen(input_file.c_str(), "r");
  if (yyin == NULL) {
  	std::cout << "ERROR:\n\tunable to open \"" << input_file << "\"" << std::endl;;
  	std::cout << "\nUSAGE: btoblas matlab_input_file.m [options]\n\n" << vis << std::endl;
  	return 1;	
  } 
  
  yyparse(); 
  
  fclose(yyin);
  return 0;
} 
void yyerror (std::string s) /* Called by yyparse on error */ 
{ 
  std::cout << "line " << yylineno << ": " << s << std::endl;
} 

void handleError(std::string errorMsg) {    
    
    int start=nTokenStart;
    int end=start + nTokenLength - 1;
    int i;
    std::cout << errorMsg << std::endl;
    std::cout << buffer;
    
    if (!eof) {
        for (i=1; i<start; i++)
            std::cout << ".";
        for (i=start; i<=end; i++)
            std::cout << "^";
        for (i=end+1; i<lBuffer; i++)
        std::cout << ".";
        std::cout << std::endl;
    }
    
    exit(-1);
}

static int getNextLine(void) {
    
    // reset global variables storing local buffer information.
    // the local buffer is used to that bto can better report errors
    // encountered when parsing.
    nBuffer = 0;
    nTokenStart = -1;
    nTokenNextStart = 1;
    eof = 0;
    
    // read next line from input file.  if not eof or other error
    // store line in global buffer.
    char *p = fgets(buffer, lMaxBuffer, yyin);
    if (p == NULL) {
        if (ferror(yyin))
            return -1;
        eof = true;
        return 1;
    }
    
    nRow += 1;
    lBuffer = strlen(buffer);
    
    return 0;
}

extern int bto_yy_input(char *b, int maxBuffer) {
    // custom reader so bto can track errors better
    // must read next maxBuffer characeters from input file (global yyin)
    // into buffer b.  This will return number of characters read
    // or 0 for no characters of eof.
    
    if (eof)
        return 0;
    
    while (nBuffer >= lBuffer) {
        if (getNextLine())
            return 0;
    }
    
    b[0] = buffer[nBuffer];
    nBuffer += 1;
    
    return b[0]==0?0:1;
}

extern void BeginToken(char *t) {
    // each time a token is encountered, stored the current location
    nTokenStart = nTokenNextStart;
    nTokenLength = strlen(t);
    nTokenNextStart = nBuffer + 1;
}
