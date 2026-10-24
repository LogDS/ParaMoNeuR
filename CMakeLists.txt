cmake_minimum_required(VERSION 4.0)
project(Paramoneur2 LANGUAGES CXX CUDA) # RISOLUTIVO: Abilita CUDA come linguaggio nativo

# 1. Configurazione dello Standard C++20 / C++26 per Host e Device
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 23)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)


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
        SparseTensor.cpp
        include/logds/paramoneur/types/SparseTensor.h)
target_link_libraries(Paramoneur2 CUDA::cutensor
        CUDA::cudart             # Runtime di CUDA (Richiesto da CCCL/Thrust)
        CUDA::cuda_driver        # Driver CUDA core
    )