cmake_minimum_required(VERSION 4.0)
project(Paramoneur2 LANGUAGES C CXX CUDA) # RISOLUTIVO: Abilita CUDA come linguaggio nativo

# 1. Configurazione dello Standard C++20 / C++26 per Host e Device
#set(CMAKE_CXX_STANDARD 23)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CUDA_STANDARD 23)
#set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
# Forza C++17 e CUDA17 (standard stabili e supportati globalmente)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # IMPORTANTE: Impedisce a CMake di cercare estensioni C++23 instabili

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF) # IMPORTANTE: Risolve l'errore "requires the language dialect CUDA23"

# Rileva automaticamente l'architettura della GPU (es. sm_75, sm_80, ecc.)
set(CMAKE_CUDA_ARCHITECTURES native)


include_directories(submodules/yaucl/submodules/antlr4/runtime/Cpp/runtime/src)
include_directories(sources/antlr4)
include_directories(submodules/yaucl/submodules/json/include)
include_directories(submodules/matplotpp/source)
include_directories(submodules/tabulate/include)

add_subdirectory(submodules/yaucl/submodules/antlr4/runtime/Cpp)
add_subdirectory(submodules/matplotpp)

# 2. Localizzazione del CUDA Toolkit (Contiene CCCL di default)
find_package(CUDAToolkit REQUIRED)


# --- CONFIGURAZIONE MANUALE DI CUTENSOR PER INSTALLAZIONI .DEB ---
if(NOT TARGET CUDA::cutensor)
    # Cerca l'header cutensor.h nei percorsi di sistema (/usr/include o /usr/local/cuda/include)
    find_path(CUDA_cutensor_INCLUDE_DIR
            NAMES cutensor.h
            HINTS ${CUDAToolkit_INCLUDE_DIRS}
            PATHS /usr/include /usr/local/cuda/include
    )

    # Cerca la libreria dinamica libcutensor.so nei percorsi di sistema
    find_library(CUDA_cutensor_LIBRARY
            NAMES cutensor
            HINTS ${CUDAToolkit_LIBRARY_DIR}
            PATHS /usr/lib/x86_64-linux-gnu /usr/local/cuda/lib64
    )

    # Se entrambi i componenti vengono trovati, crea il target mancante
    if(CUDA_cutensor_INCLUDE_DIR AND CUDA_cutensor_LIBRARY)
        add_library(CUDA::cutensor UNKNOWN IMPORTED
                include/logds/paramoneur/propertygraph/pg.h)
        set_target_properties(CUDA::cutensor PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${CUDA_cutensor_INCLUDE_DIR}"
                IMPORTED_LOCATION "${CUDA_cutensor_LIBRARY}"
        )
        message(STATUS "Found cuTENSOR (.deb): ${CUDA_cutensor_LIBRARY}")
    else()
        message(FATAL_ERROR "cuTENSOR non trovato. Assicurati che il pacchetto di sviluppo (es. libcutensor-dev) sia installato.")
    endif()
endif()
# -----------------------------------------------------------------


#set(CMAKE_CXX_STANDARD 26)

include_directories(include)
include_directories(submodules/yaucl/include)
include_directories(submodules/field-reflection/include)

add_library(yaucl_hashing OBJECT
        submodules/yaucl/include/yaucl/hashing/evaluateHashes.h
        submodules/yaucl/src/yaucl/hashing/evaluateHashes.c
        submodules/yaucl/src/yaucl/hashing/hash_combine.cpp
        submodules/yaucl/include/yaucl/hashing/hash_combine.h
        submodules/yaucl/src/yaucl/hashing/hashing.cpp
        submodules/yaucl/include/yaucl/hashing/hashing.h
        submodules/yaucl/include/yaucl/hashing/pair_hash.h
        submodules/yaucl/include/yaucl/hashing/vector_hash.h
        submodules/yaucl/include/yaucl/hashing/uset_hash.h)

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
        sources/logds/paramoneur/algorithms/indexing.cpp
        include/logds/paramoneur/propertygraph/pg_types.h
        include/logds/paramoneur/propertygraph/pg_store.h
        sources/logds/paramoneur/propertygraph/pg_store.cpp
        sources/logds/paramoneur/propertygraph/pg.cpp
        sources/logds/paramoneur/types/SparseTensor.cpp
        include/logds/paramoneur/types/SparseTensor.h



        sources/antlr4/extended_prologBaseListener.cpp
        sources/antlr4/extended_prologBaseVisitor.cpp
        sources/antlr4/extended_prologLexer.cpp
        sources/antlr4/extended_prologListener.cpp
        sources/antlr4/extended_prologParser.cpp
        sources/antlr4/extended_prologVisitor.cpp
        sources/logds/paramoneur/language/VisitProlog.cpp
        include/logds/paramoneur/language/VisitProlog.h
        sources/logds/paramoneur/environment/DatasetBuilder.cpp
        include/logds/paramoneur/environment/DatasetBuilder.h
)
target_link_libraries(Paramoneur2 CUDA::cutensor
        CUDA::cudart             # Runtime di CUDA (Richiesto da CCCL/Thrust)
        CUDA::cuda_driver        # Driver CUDA core
        antlr4_static
        yaucl_hashing
        Matplot++::matplot
    )

add_executable(cuda_select_demo
        example_prln.cpp
        sources/logds/paramoneur/engine/prln_step.cu
        include/logds/paramoneur/engine/prln_step.cuh
)
set_target_properties(cuda_select_demo PROPERTIES
        CUDA_SEPARABLE_COMPILATION OFF
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
        CUDA_SEPARABLE_COMPILATION OFF
)