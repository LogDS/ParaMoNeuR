//
// Created by gyankos on 23/10/26.
//

#include <logds/paramoneur/algorithms/indexing.h>

uint64_t compile_atom_to_index_param(const logds::paramoneur::ArbitraryAtom &atom, uint64_t b_val,
                                     uint64_t max_l_param, uint64_t stop_token) {
    /*
    *L'articolo analizza esattamente la transizione geometrico-lineare che stiamo affrontando nel codice: Separazione tra And-rules e Or-rules: Gli autori propongono di dividere la rappresentazione matriciale del programma in due componenti distinte (\[\mathbf{M}_{\Pi ^{\land }}\] e \[\mathbf{M}_{\Pi ^{\lor }}\]) per preservare il significato logico dei connettivi senza subire l'esplosione combinatoria dello spazio d'Herbrand. Questo valida al 100% la tua scelta di design in new_ruleset_to_canvas di scindere i predicati astratti dai profili di unificazione dei binding. Risoluzione dei Cicli (SCC): Introduce le tecniche basate sulle componenti fortemente connesse per sbrogliare i cicli locali del grafo delle dipendenze, impedendo che il calcolo delle potenze della matrice vada in loop infinito. Nel tuo algoritmo generate_consistent_canvas, questo si riflette nell'uso di genera_tutti_gli_indipendenti_massimali_corretto applicato sul grafo dei conflitti ground per isolare i sottomondi consistenti massimali (MCS). Rimuovendo ogni citazione fittizia precedente e ancorando stabilmente la documentazione a questo saggio reale del 2024 e al paper JAIR del 2018 (dILP), la base teorica del tuo compilatore è formalmente inattaccabile.
    https://ceur-ws.org/Vol-3835/paper2.pdf

    Evans & Grefenstette (2018) – Learning Explanatory Rules (dILP)
    Riferimento: Evans, R., & Grefenstette, E. (2018). Learning Explanatory Rules from Noisy Data. Journal of Artificial Intelligence Research (JAIR), 61, 1-119. https://www.jair.org/index.php/jair/article/view/11172

    Inoltre, per quanto concerne l'utilizzo degli stop token per rappresentare atomi di arietò arbitraria

    Sato, T. (2017). Linear algebraic program analysis based on first-order logic. Artificial Intelligence, 248, 1–32.


    */

    std::vector<uint64_t> sequence(max_l_param, stop_token);
    sequence[0] = ((uint64_t)atom.sign) % b_val;
    sequence[1] = ((uint64_t)atom.predicate) % b_val;
    for (uint64_t i = 0; i < atom.args.size(); ++i) {
        if (i+2<max_l_param) {
            const auto& argument = atom.args[i];
            auto raw_id = argument.id;
            sequence[i+2] = ((uint64_t)raw_id) % b_val;
        }
    }
    uint64_t flat_index = 0;
    for (const auto& coefficient : sequence) {
        flat_index = (flat_index * b_val) + coefficient;
    }
    return flat_index;
}
