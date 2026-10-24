#include <iostream>

#include "logds/paramoneur/propertygraph/pg.h"
#include "logds/paramoneur/propertygraph/pg_store.h"


// =========================================================================
// NUOVI TIPI DI DATO IN CALCE PER L'ESEMPIO ETEROGENEO RIFLESSO
// =========================================================================
struct User {
    std::string username;
    int64_t age;
    bool is_verified;
};

struct Device {
    std::string hardware_model;
    double battery_level;
};

struct Follows {
    int64_t close_interaction_score;
    bool mute_notifications;
};

struct ConnectedTo {
    double latency_ms;
    int64_t bandwidth_mbps;
};

// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon sources="AllIcons.Actions.Execute"/> icon in the gutter.

int main() {
    // Variabili per mantenere gli ID persistenti lungo le diverse fasi del programma
    size_t id_stazione_a = 0;
    size_t id_stazione_b = 1;
    size_t edge_id = 0;

    size_t id_user_alice = 2;
    size_t id_device_iot = 3;
    size_t edge_follows = 1;
    size_t edge_connect = 2;
    {
        std::cout << "Initializing Property Graph Disk Engine..." << std::endl;

    // Inizializza lo storage usando come prefisso di base "my_graph_store"
    logds::pg::PropertyGraphStore graph("my_graph_store");



    // Convalida preliminare per determinare se il database contiene già dei dati
    bool db_exists = false;
    {
        // Chiediamo alla transazione quanti nodi sono registrati correntemente su disco
        auto check_tx = graph.begin_transaction();
        if (check_tx.get_node_type_string(0) != "" || check_tx.is_node_type_compatible<User>(0) || check_tx.node_exists_context(0)) {
            db_exists = true;
        }
        check_tx.rollback();
    }

    // =========================================================================
    // FASE 1: APERTURA TRANSAZIONE E CREAZIONE (Eseguita solo se il grafo non esiste)
    // =========================================================================
    if (!db_exists) {
        std::cout << "[DB Status] Empty database detected. Initializing graph elements..." << std::endl;
        auto tx1 = graph.begin_transaction();

        // Creazione esplicita controllata di nodi isolati (Stazioni preesistenti)
        id_stazione_a = tx1.create_node(); // ID 0
        id_stazione_b = tx1.create_node(); // ID 1

        // Assegnamento proprieta primitive tramite sintassi operator[] nativa
        tx1[id_stazione_a]["name"] = "Milan Central Station";
        tx1[id_stazione_a]["capacity"] = int64_t(45000);
        tx1[id_stazione_a]["active"] = true;
        tx1[id_stazione_b]["name"] = "Rome Termini Station";

        // Costruzione e inserimento di una proprieta complessa: Tensor Sparso COO
        logds::types::SparseTensor matrix({100, 100, 50}, -1.0);
        // matrix.data_type = logds::pg::TensorDataType::DOUBLE;
        // matrix.rank = 3;                       // Tensore 3D
        // matrix.shape = {100, 100, 50};          // Dimensioni degli assi
        // matrix.default_value = -1.0;            // Valore di background (non zero)
        matrix.set_value({0, 14, 5}, 74.3);
        matrix.set_value({85, 2, 41}, 192.8);
        // matrix.coordinates = { {0, 14, 5}, {85, 2, 41} }; // Elementi non-default espliciti
        // matrix.values = std::vector<double>{ 74.3, 192.8 };

        tx1[id_stazione_a]["traffic_tensor"] = matrix;

        // Creazione controllata di un arco relazionale (Verifica l'esistenza di 0 e 1)
        edge_id = tx1.create_edge(id_stazione_a, id_stazione_b); // ID 0
        tx1.get_edge(edge_id)["distance_km"] = 572.5;

        // -----------------------------------------------------------------
        // INSERIMENTO IN CALCE: NUOVE STRUTTURE ETEROGENEE (NODI E ARCHI)
        // -----------------------------------------------------------------
        id_user_alice = tx1.create_node(); // ID 2
        id_device_iot = tx1.create_node(); // ID 3

        User alice{"Alice_Dev", 25, true};
        Device iot_node{"Raspberry Pi 4", 98.4};

        // Persistenza riflessa biunivoca protetta sul nome della struct
        tx1.set_node_struct(id_user_alice, alice);
        tx1.set_node_struct(id_device_iot, iot_node);

        // Creazione archi con strutture tipizzate associate
        edge_follows = tx1.create_edge(id_user_alice, id_stazione_a); // ID 1 (Alice segue la stazione)
        edge_connect = tx1.create_edge(id_device_iot, id_stazione_b); // ID 2 (Il dispositivo invia dati a Roma)

        tx1.set_edge_struct(edge_follows, Follows{85, false});
        tx1.set_edge_struct(edge_connect, ConnectedTo{12.4, 1000});

        std::cout << "Committing transaction 1 to secondary memory..." << std::endl;
        tx1.commit();
    } else {
        std::cout << "[DB Status] Existing database found. Skipping structural creation phases." << std::endl;
    }

    // ==========================================
    // FASE 2: VERIFICA DI ESISTENZA E SICUREZZA
    // ==========================================
    auto tx2 = graph.begin_transaction();

    try {
        // Questa riga deve lanciare un'eccezione poiche il nodo 99 non e mai stato creato
        tx2[99]["name"] = "Ghost";
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected safety guard exception: " << e.what() << std::endl;
    }

    try {
        // Questa riga deve fallire perche il nodo destinazione 8888 non esiste nella topologia
        tx2.create_edge(id_stazione_b, 8888);
    } catch (const std::runtime_error& e) {
        std::cout << "Caught expected topology guard exception: " << e.what() << std::endl;
    }
    tx2.rollback();

    // ==========================================
    // FASE 3: LETTURA E VERIFICA DAL DISCO
    // ==========================================
    std::cout << "Opening read transaction to verify data..." << std::endl;
    auto tx3 = graph.begin_transaction();

    // Lettura in tempo costante O(1) con parsing automatico dei byte
    logds::pg::PropertyValue p_name = tx3[id_stazione_a]["name"];
    logds::pg::PropertyValue p_cap = tx3[id_stazione_a]["capacity"];
    logds::pg::PropertyValue p_edge_dist = tx3.get_edge(edge_id)["distance_km"];

    std::cout << "Retrieved Node 0 Name: " << std::get<std::string>(p_name.value) << std::endl;
    std::cout << "Retrieved Node 0 Capacity: " << std::get<int64_t>(p_cap.value) << std::endl;
    std::cout << "Retrieved Edge 0 Distance: " << std::get<double>(p_edge_dist.value) << " km" << std::endl;

    // Estrazione e convalida dei metadati e dei vettori del tensore sparso
    logds::pg::PropertyValue p_tensor = tx3[id_stazione_a]["traffic_tensor"];
    logds::types::SparseTensor r_tensor = std::get<logds::types::SparseTensor>(p_tensor.value);

    std::cout << "Retrieved Sparse Tensor Rank: " << r_tensor.get_rank() << std::endl;
    std::cout << "Retrieved Sparse Tensor Background Value: " << std::get<double>(r_tensor.get_default_value()) << std::endl;

    auto tensor_vals = std::get<std::vector<double>>(r_tensor.get_values());
    std::cout << "Explicit Element [0, 14, 5] Value: " << tensor_vals[0] << std::endl;
    std::cout << "Explicit Element [85, 2, 41] Value: " << tensor_vals[1] << std::endl;

    // -----------------------------------------------------------------
    // CONTROLLI IN CALCE: VERIFICA VALORI E NON-BLOCCANTI (OPTIONAL)
    // -----------------------------------------------------------------
    std::cout << "\n--- Commencing Heterogeneous & Optional Type Verifications ---" << std::endl;

    // 1. Ispezione tramite std::type_index standard per verificare l'identità nativa del tipo
    std::optional<std::type_index> forse_tipo_nodo_2 = tx3.get_node_type_index(id_user_alice);
    std::cout << "Node 2 Type Verification Status: ";

    if (forse_tipo_nodo_2.has_value() && *forse_tipo_nodo_2 == std::type_index(typeid(User))) {
        std::cout << "Natively identified as C++ 'User' struct." << std::endl;
    } else {
        std::cout << "Type not recognized or missing." << std::endl;
    }

    // =========================================================================
    // ISPEZIONE ARCHI TRAMITE OPERATORI DI TIPO STANDARD C++ (STD::TYPE_INDEX)
    // =========================================================================
    std::optional<std::type_index> forse_tipo_arco = tx3.get_edge_type_index(edge_connect);
    std::cout << "Edge " << edge_connect << " Type Verification Status: ";

    if (forse_tipo_arco.has_value() && *forse_tipo_arco == std::type_index(typeid(ConnectedTo))) {
        std::cout << "Natively identified as C++ 'ConnectedTo' relation structure." << std::endl;

        // Stampiamo il nome leggendolo dal registro tramite l'indice ottenuto
        std::cout << "   Registered under C++ class name: "
                  << logds::pg::PropertyGraphTypeRegistry::instance().get_type_name(*forse_tipo_arco) << std::endl;
    } else {
        std::cout << "Type not recognized or missing." << std::endl;
    }

    // 2. Lettura non bloccante con tipo corretto (Ritorna un optional pieno)
    std::optional<User> forse_alice = tx3.try_get_node_struct<User>(id_user_alice);
    if (forse_alice) {
        std::cout << ">> [Optional Success] User Object loaded." << std::endl;
        std::cout << "   Username: " << forse_alice->username << " | Age: " << forse_alice->age << std::endl;
    }

    // 3. Lettura condizionata con tipo errato (Ritorna std::nullopt anti-mismatch senza crash)
    std::optional<Device> err_mismatch_nodo = tx3.try_get_node_struct<Device>(id_user_alice);
    if (!err_mismatch_nodo.has_value()) {
        std::cout << ">> [Optional Guard Success] Prevented parsing User node into Device structure (nullopt returned)." << std::endl;
    }

    // 4. Verifica simmetrica non bloccante applicata agli ARCHI (Edges) del grafo
    std::optional<ConnectedTo> forse_connessione = tx3.try_get_edge_struct<ConnectedTo>(edge_connect);
    if (forse_connessione) {
        std::cout << ">> [Optional Edge Success] Edge 2 loaded correctly as 'ConnectedTo'." << std::endl;
        std::cout << "   Latency: " << forse_connessione->latency_ms << " ms | Bandwidth: " << forse_connessione->bandwidth_mbps << " Mbps" << std::endl;
    }

    std::optional<Follows> err_mismatch_arco = tx3.try_get_edge_struct<Follows>(edge_connect);
    if (!err_mismatch_arco) {
        std::cout << ">> [Optional Edge Guard Success] Prevented parsing ConnectedTo edge into Friendship (nullopt returned)." << std::endl;
    }

    tx3.rollback();
    std::cout << "\nAll storage engine constraints successfully verified." << std::endl;
    }
    // =========================================================================
    // FASE 4: USO DEL WRAPPER PROPERTYGRAPH E SERIALIZZAZIONE SU FILE (CORRETTO)
    // =========================================================================
    std::cout << "\n--- Commencing Phase 4: NetworkX-style PropertyGraph Wrapper & Disk Export ---" << std::endl;

    logds::pg::PropertyGraph G("my_graph_store");

    std::cout << "Navigating outgoing connections of Node 0 via NetworkX-style facade:" << std::endl;
    for (size_t target_node : G.successors(id_stazione_a)) {
        // CORREZIONE: Assegnamento implicito del Proxy a PropertyValue per bypassare l'attributo assente '.value'
        logds::pg::PropertyValue name_prop = G.nodes[target_node]["name"];
        std::string target_name = std::get<std::string>(name_prop.value);

        std::cout << " -> Hub Node " << id_stazione_a << " points directly to Node " << target_node
                  << " [Target Name: " << target_name << "]" << std::endl;
    }

    std::string graphml_buffer = G.to_graphml();
    std::string dot_buffer = G.to_dot();

    std::string graphml_filename = "exported_graph.graphml";
    std::string dot_filename = "exported_graph.dot";

    std::ofstream graphml_file(graphml_filename);
    if (graphml_file.is_open()) {
        graphml_file << graphml_buffer;
        graphml_file.close();
        std::cout << ">> [Disk Export Success] Saved GraphML schema to: " << graphml_filename << std::endl;
    }

    std::ofstream dot_file(dot_filename);
    if (dot_file.is_open()) {
        dot_file << dot_buffer;
        dot_file.close();
        std::cout << ">> [Disk Export Success] Saved Graphviz DOT layout to: " << dot_filename << std::endl;
    }

    std::cout << "\nAll storage engine, reflection, type-checking and networkx constraints verified." << std::endl;
}