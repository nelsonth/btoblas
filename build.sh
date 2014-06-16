#!/bin/bash
make clean
cd src/
bison -d syntax.y
flex syntax.l
mv lex.yy.c lex.yy.cpp
mv syntax.tab.c syntax.tab.cpp
mv syntax.tab.h syntax.tab.hpp
cd ../
autoreconf --install
./configure --prefix=$(pwd) CXX='clang++' CXXFLAGS='-g -O0 -std=c++11 -stdlib=libc++'
make -j2
make install
