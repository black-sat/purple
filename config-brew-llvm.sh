#!/bin/bash

CC=$(brew --prefix llvm)/bin/clang \
CXX=$(brew --prefix llvm)/bin/clang++ \
LDFLAGS="$LDFLAGS -L$(brew --prefix llvm)/lib -Wl,-rpath,$(brew --prefix llvm)/lib" \
CXXFLAGS="$CXXFLAGS -I$(brew --prefix llvm)/include" \
cmake "$@"