//
// Created by gyankos on 24/10/26.
//

#include <matplot/matplot.h>
#include <tabulate/table.hpp>
#include "logds/paramoneur/types/SparseTensor.h"

#include <algorithm>
#include <assert.h>
#include <iomanip>
#include <stdexcept>

namespace logds::types {
    SparseTensor::SparseTensor(): data_type(TensorDataType::DOUBLE), rank(0), default_value(0.0) {}

    SparseTensor::SparseTensor(const std::vector<size_t> &tensor_shape, double default_value): data_type(TensorDataType::DOUBLE), shape(tensor_shape), rank(tensor_shape.size()) {
        this->default_value = default_value;
    }

    SparseTensor::SparseTensor(const std::vector<size_t> &tensor_shape, int64_t default_value): data_type(TensorDataType::INT64), shape(tensor_shape), rank(tensor_shape.size()) {
        this->default_value = default_value;
    }

    SparseTensor::SparseTensor(TensorDataType type, const std::vector<size_t> &tensor_shape): data_type(type), shape(tensor_shape), rank(tensor_shape.size()) {
        if (data_type == TensorDataType::INT64) {
            default_value = int64_t(0);
        } else {
            default_value = double(0.0);
        }
    }

    bool SparseTensor::set_value(const std::vector<size_t> &coord, const TensorValueVariant &val) {
        // Guardrail A: Convalida del tipo di dato in ingresso rispetto al tipo nativo impostato
        if ((data_type == TensorDataType::INT64 && !std::holds_alternative<int64_t>(val)) ||
            (data_type == TensorDataType::DOUBLE && !std::holds_alternative<double>(val))) {
            return false; // Conflitto di tipo rilevato, l'operazione fallisce silenziosamente
        }

        // Guardrail B: Verifica se la coordinata è fuori dalle dimensioni (Out of Bounds)
        if (coord.size() != rank) return false;
        for (size_t i = 0; i < rank; ++i) {
            if (coord[i] >= shape[i]) {
                return false; // Ignorato silenziosamente senza lanciare eccezioni
            }
        }

        // Guardrail C: Verifica se il valore è inferiore alla soglia di tolleranza decimale
        if (data_type == TensorDataType::DOUBLE) {
            double v_double = std::get<double>(val);
            if (std::abs(v_double) <= std::get<double>(default_value)) {
                return false; // Non efficace per preservare la sparsità strutturale
            }
        } else {
            int64_t v_int = std::get<int64_t>(val);
            if (std::abs(static_cast<int64_t>(v_int)) <= std::get<int64_t>(default_value)) {
                return false; // Non efficace per interi vicini o uguali a zero
            }
        }

        // Guardrail D: Gestione sovrascrittura se la coordinata era già presente (Evita duplicazioni COO)
        for (size_t i = 0; i < coordinates.size(); ++i) {
            if (coordinates[i] == coord) {
                if (data_type == TensorDataType::INT64) {
                    if (!std::holds_alternative<std::vector<int64_t>>(values)) {
                        values = std::vector<int64_t>(coordinates.size(), 0);
                    }
                    std::get<std::vector<int64_t>>(values)[i] = std::get<int64_t>(val);
                } else {
                    if (!std::holds_alternative<std::vector<double>>(values)) {
                        values = std::vector<double>(coordinates.size(), 0.0);
                    }
                    std::get<std::vector<double>>(values)[i] = std::get<double>(val);
                }
                return true; // Aggiornamento eseguito sul posto
            }
        }

        // Se non esisteva, eseguiamo l'append in coda (Nuovo elemento COO valido)
        coordinates.push_back(coord);
        if (data_type == TensorDataType::INT64) {
            if (!std::holds_alternative<std::vector<int64_t>>(values)) {
                values = std::vector<int64_t>();
            }
            std::get<std::vector<int64_t>>(values).push_back(std::get<int64_t>(val));
        } else {
            if (!std::holds_alternative<std::vector<double>>(values)) {
                values = std::vector<double>();
            }
            std::get<std::vector<double>>(values).push_back(std::get<double>(val));
        }
        return true;
    }

    SparseTensor::TensorValueVariant SparseTensor::get_value(const std::vector<size_t> &coord) const {
        if (coord.size() != rank) return default_value;

        for (size_t i = 0; i < coordinates.size(); ++i) {
            if (coordinates[i] == coord) {
                if (data_type == TensorDataType::INT64) {
                    return std::get<std::vector<int64_t>>(values)[i];
                } else {
                    return std::get<std::vector<double>>(values)[i];
                }
            }
        }
        return default_value;
    }

    void SparseTensor::convert_nested_coo_to_csr_cpu(std::vector<int> &csrRowPtr, std::vector<int> &csrColInd) {
        if (rank != 2) {
            throw std::runtime_error("ERROR: convert_nested_coo_to_csr_cpu requires to have a rank 2 tensor, but you're having "+std::to_string(rank)+" instead.");
        }
        auto cooMatrix = coordinates;
        size_t nnz = cooMatrix.size();

        // --- GESTIONE CASO MATRICE VUOTA ---
        // Se la matrice in ingresso non ha elementi, il CSR valido prevede:
        // csrRowPtr = {0} (dimensione 1, num_rows = 0) e csrColInd vuoto.
        if (nnz == 0) {
            csrRowPtr.assign(1, 0); // Forza la dimensione a 1 e inserisce lo 0
            csrColInd.clear();      // Svuota completamente l'array delle colonne
            return;
        }

        // 1. PASSO DI ORDINAMENTO LESSICOGRAFICO
        std::sort(cooMatrix.begin(), cooMatrix.end(), [](const std::vector<size_t>& a, const std::vector<size_t>& b) {
            if (a[0] != b[0]) return a[0] < b[0];
            return a[1] < b[1];
        });

        // 2. DETERMINAZIONE DINAMICA DI num_rows
        size_t max_row = 0;
        for (size_t i = 0; i < nnz; ++i) {
            if (cooMatrix[i][0] > max_row) {
                max_row = cooMatrix[i][0];
            }
        }
        int num_rows = static_cast<int>(max_row) + 1;

        // 3. INIZIALIZZAZIONE E ALLOCAZIONE FORZATA DEGLI OUTPUT
        // .assign pulisce qualsiasi dato pregresso e alloca esattamente la dimensione num_rows + 1
        csrRowPtr.assign(num_rows + 1, 0);
        // .resize adegua la dimensione a nnz (se era più grande lo contrae, se era più piccolo lo espande)
        csrColInd.resize(nnz);

        // 4. PASSO ISTOGRAMMA: Conta quanti elementi non-zero ha ogni riga
        for (size_t i = 0; i < nnz; ++i) {
            size_t r = cooMatrix[i][0];
            csrRowPtr[r + 1]++;
        }

        // 5. PASSO SCAN (Somma Prefissa): Costruisce i puntatori CSR stabili
        for (int r = 0; r < num_rows; ++r) {
            csrRowPtr[r + 1] += csrRowPtr[r];
        }

        // 6. MAPPA DELLE COLONNE: Inserisce le colonne nelle posizioni corrette
        std::vector<int> row_next_pos = csrRowPtr;

        for (size_t i = 0; i < nnz; ++i) {
            size_t r = cooMatrix[i][0];
            size_t c = cooMatrix[i][1];

            int dest_idx = row_next_pos[r];
            csrColInd[dest_idx] = static_cast<int>(c);

            row_next_pos[r]++;
        }
    }

    void SparseTensor::save_tensor_heatmaps_to_file(
        const std::string &output_filepath, const LabelProvider &label_provider) {
        if (rank < 2) {
            std::cerr << "Errore: Rango insufficiente per una mappa bidimensionale.\n";
            return;
        }

        // Calcoliamo il numero totale di slice 2D generate dalle dimensioni superiori (0 a N-3)
        size_t total_slices = 1;
        for (size_t i = 0; i < rank - 2; ++i) {
            total_slices *= shape[i];
        }

        size_t cols = std::ceil(std::sqrt(total_slices));
        size_t rows = std::ceil(static_cast<double>(total_slices) / cols);

        // Creazione del contesto figura Matplot++
        auto f = matplot::figure(true);
        f->size(400 * cols, 400 * rows);

        size_t h_dim_size = shape[rank - 2];
        size_t w_dim_size = shape[rank - 1];

        // Configurazione assi per le ultime due dimensioni
        std::vector<std::string> x_labels, y_labels;
        for (size_t j = 0; j < w_dim_size; ++j) x_labels.push_back(label_provider(rank - 1, j));
        for (size_t i = 0; i < h_dim_size; ++i) y_labels.push_back(label_provider(rank - 2, i));

        // Generazione delle singole slice (Ipotizziamo per semplicità un tensore 3D di forma [Slices, H, W])
        for (size_t s = 0; s < total_slices; ++s) {
            std::vector<size_t> current_dims = { s };
            auto slice_matrix = extract_2d_slice( current_dims, rank - 2, rank - 1, h_dim_size, w_dim_size);

            matplot::subplot(rows, cols, s + 1);
            auto h_plot = matplot::heatmap(slice_matrix);
            h_plot->always_hide_labels(true);
            auto ax = matplot::gca();
            ax->x_axis().ticklabels(x_labels);
            ax->y_axis().ticklabels(y_labels);

            std::string title = "Slice (" + label_provider(0, s) + ", :, :)";
            matplot::title(title);
        }

        // RISOLUTIVO HEADLESS: Salvataggio diretto su file system senza chiamare matplot::show()
        std::cout << "[Matplot++] Salvataggio grafico headless in corso in: " << output_filepath << " ...\n";
        matplot::save(output_filepath);
        std::cout << "[Matplot++] Grafico salvato con successo.\n";
    }

    void SparseTensor::terminal_plot_mytensor(double threshold,
        const LabelProvider &label_provider) {


        // Inizializzazione della tabella p-ranav/tabulate
        tabulate::Table table;

        // 1. Definizione dell'Intestazione (Header delle Colonne)
        tabulate::Table::Row_t header_row;
        for (size_t d = 0; d < rank; ++d) {
            header_row.push_back("Dimensione " + std::to_string(d));
        }
        header_row.push_back("Valore Cella");
        table.add_row(header_row);

        size_t added_cells = 0;

        // 2. Iterazione condizionale sugli elementi sparsi COO
        for (size_t i = 0; i < coordinates.size(); ++i) {
            const auto& coord = coordinates[i];

            // Estrazione del valore reale
            double val = 0.0;
            if (std::holds_alternative<std::vector<int64_t>>(values)) {
                val = static_cast<double>(std::get<std::vector<int64_t>>(values)[i]);
            } else {
                val = std::get<std::vector<double>>(values)[i];
            }

            // Applicazione della soglia di scarto (t) decimale
            if (std::abs(val) <= threshold) {
                continue;
            }

            // Costruzione della riga con la specifica stringa di codifica per ogni dimensione
            tabulate::Table::Row_t data_row;
            for (size_t d = 0; d < rank; ++d) {
                data_row.push_back(label_provider(d, coord[d]));
            }

            // Formattazione stringa del valore
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << val;
            data_row.push_back(ss.str());

            table.add_row(data_row);
            added_cells++;
        }

        // 3. Stilizzazione e Formattazione Estetica con tabulate
        if (added_cells > 0) {
            // Applica stili all'intestazione (Row 0)
            table[0].format()
                    .font_style({tabulate::FontStyle::bold})
                    .font_align(tabulate::FontAlign::center)
                    .font_color(tabulate::Color::cyan);

            // Allineamento a destra della colonna dei valori
            table.column(rank).format().font_align(tabulate::FontAlign::right);

            // Configurazione bordi puliti in stile Unicode Box Drawing
            table.use_unicode_borders(true); // Abilita allineamenti per tabelle lunghe
        } else {
            table.add_row({"Nessun elemento presente sopra la soglia selezionata.", "", ""});
        }

        // Stampa a terminale tramite operatore di stream standard <<
        std::cout << "\n" << table << std::endl;
    }

    std::vector<std::vector<double>> SparseTensor::extract_2d_slice(const std::vector<size_t> &current_dims,
                                                                    size_t dim_h, size_t dim_w, size_t size_h, size_t size_w) {
        std::vector<std::vector<double>> slice_data(size_h, std::vector<double>(size_w, 0.0));
        size_t rank = current_dims.size() + 2;

        for (size_t i = 0; i < coordinates.size(); ++i) {
            const auto& coord = coordinates[i];
            bool match = true;

            // Verifica se l'elemento appartiene alla slice corrente nelle dimensioni superiori
            for (size_t d = 0; d < current_dims.size(); ++d) {
                if (coord[d] != current_dims[d]) {
                    match = false;
                    break;
                }
            }

            if (match) {
                size_t h_idx = coord[rank - 2];
                size_t w_idx = coord[rank - 1];

                double val = 0.0;
                if (std::holds_alternative<std::vector<int64_t>>(values)) {
                    val = static_cast<double>(std::get<std::vector<int64_t>>(values)[i]);
                } else {
                    val = std::get<std::vector<double>>(values)[i];
                }
                slice_data[h_idx][w_idx] = val;
            }
        }
        return slice_data;
    }
}
