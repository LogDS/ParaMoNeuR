cmake_minimum_required(VERSION 4.0)
project(Paramoneur2)

set(CMAKE_CXX_STANDARD 26)

include_directories(include)
include_directories(submodules/yaucl/include)

add_executable(Paramoneur2 main.cpp
        sources/logds/paramoneur/algorithms/canonical_form.cpp
        sources/logds/paramoneur/algorithms/unification.cpp
        include/logds/paramoneur/constants/constants.h
        include/logds/paramoneur/types/aliases.h
        include/logds/paramoneur/language/parsed_structures.h
        include/logds/paramoneur/types/shared.h
        include/logds/paramoneur/language/compiled_structures.h
        sources/logds/paramoneur/language/compiled_structures.cpp
        sources/logds/paramoneur/language/parsed_structures.cpp
        sources/logds/paramoneur/environment/GlobalEnvironmentProxy.cpp
        include/logds/paramoneur/environment/GlobalEnvironmentProxy.h
        sources/logds/paramoneur/algorithms/SkepticalFramework.cpp
        include/logds/paramoneur/algorithms/ParaconsistentTheoryContraction.h
        sources/logds/paramoneur/algorithms/ParaconsistentTheoryContraction.cpp
        include/logds/paramoneur/algorithms/indexing.h
        sources/logds/paramoneur/algorithms/indexing.cpp)
