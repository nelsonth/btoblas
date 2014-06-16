bin_PROGRAMS = btoblas
btoblas_SOURCES = syntax.hpp graph.hpp analyze_graph.hpp \
	build_graph.hpp compile.hpp translate_to_code.hpp optimizers.hpp \
	partition.hpp type_analysis.hpp work.hpp md5.h \
	md5wrapper.h enumerate.hpp test_generator.hpp workSpace.hpp \
	search_exhaustive.hpp generate_code.hpp opt_decision.hpp graphQuery.hpp \
	evolve.hpp iterator.hpp enums.hpp optimize_analysis.hpp code_gen.hpp \
	syntax.cpp build_graph.cpp analyze_graph.cpp compile.cpp \
	translate_utils.cpp optimizers.cpp partition.cpp type_analysis.cpp \
	translate_to_code.cpp work.cpp md5wrapper.cpp md5.cpp enumerate.cpp \
	test_generator.cpp workSpace.cpp \
	search_exhaustive.cpp \
	generate_code.cpp opt_decision.cpp graphQuery.cpp evolve.cpp iterator.cpp \
	optimize_analysis.cpp code_gen.cpp syntax.tab.cpp syntax.tab.hpp lex.yy.cpp

# Boost settings
# BOOST_CPPFLAGS = @BOOST_CPPFLAGS@
BOOST_LIB_DIR = $(BOOST_ROOT)/stage/lib
BOOST_LIBS = -lboost_program_options

# Additional includes
AM_CPPFLAGS = -I$(BOOST_ROOT)

# Additional libraries
btoblas_LDFLAGS = -L$(BOOST_LIB_DIR)
# btoblas_LDADD = $(BOOST_LIBS)
btoblas_LDADD = -lboost_program_options
