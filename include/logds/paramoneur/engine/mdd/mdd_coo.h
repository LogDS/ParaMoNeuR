//
// Created by gyankos on 28/10/26.
//

#ifndef PARAMONEUR2_MDD_COO_H
#define PARAMONEUR2_MDD_COO_H

#include <vector>
#include <cstdint>

// Struttura accessoria per emulare il layout di una matrice sparsa COO
struct COOTuple {
    long long row;
    long long col;
    int value;
    bool active;
};

class CUDAMddCOO {
private:
    int num_vars;
    int domain_size;
    long long total_assignments;
    bool* d_mask; // Puntatore alla memoria GPU (Device)

    // Helper per convertire l'assegnamento in indice lineare flat (Base D)
    long long getLinearIndex(const std::vector<int>& assignment) const;

public:
    CUDAMddCOO(int vars, int domain);
    ~CUDAMddCOO();

    // Disattiva un assegnamento specifico (Rimozione logica)
    void removeAssignment(const std::vector<int>& assignment);

    // Query singola di ammissibilità
    bool isAdmissible(const std::vector<int>& assignment) const;

    // Query massiva parallela su GPU
    std::vector<uint8_t> isAdmissibleBatch(const std::vector<int> &h_queries, int num_queries);

    // Accessore virtuale in stile coordinate COO della matrice
    COOTuple getCOOAccessor(const std::vector<int>& assignment) const;

    // NUOVO: Rimuove massivamente tutti i cammini dove la variabile 'target_var' (0-indexed) è pari a 'constant_val'
    void removeVariableConstant(int target_var, int constant_val);
};


#endif //PARAMONEUR2_MDD_COO_H
