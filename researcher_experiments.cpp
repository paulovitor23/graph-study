#include "algorithms.hpp"
#include "graph.hpp"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::array<const char*, 6> researcher_names = {
    "Edsger W. Dijkstra",
    "Alan M. Turing",
    "J. B. Kruskal",
    "Jon M. Kleinberg",
    "Éva Tardos",
    "Daniel R. Figueiredo"
};

std::size_t parse_external_id(const std::string& value) {
    std::size_t consumed = 0;
    const std::size_t id = std::stoull(value, &consumed);
    if (consumed != value.size() || id == 0) {
        throw std::runtime_error("identificador de pesquisador invalido");
    }
    return id;
}

struct Researchers {
    std::vector<std::string> names;
    std::array<std::size_t, researcher_names.size()> required_ids;
};

Researchers read_researchers(
    const std::string& path,
    std::size_t vertex_count
) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("nao foi possivel abrir " + path);
    }

    Researchers result;
    result.names.resize(vertex_count);
    result.required_ids.fill(std::numeric_limits<std::size_t>::max());

    std::string line;
    std::size_t line_number = 0;
    std::size_t researcher_count = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const std::size_t comma = line.find(',');
        if (comma == std::string::npos) {
            throw std::runtime_error(
                "linha invalida no arquivo de pesquisadores: " +
                std::to_string(line_number)
            );
        }

        const std::size_t external_id = parse_external_id(line.substr(0, comma));
        if (external_id > vertex_count) {
            throw std::runtime_error(
                "pesquisador fora do intervalo na linha " +
                std::to_string(line_number)
            );
        }

        const std::size_t vertex = external_id - 1;
        if (!result.names[vertex].empty()) {
            throw std::runtime_error(
                "identificador de pesquisador repetido: " +
                std::to_string(external_id)
            );
        }
        std::string name = line.substr(comma + 1);
        if (!name.empty() && name.back() == '\r') {
            name.pop_back();
        }
        if (name.empty()) {
            throw std::runtime_error(
                "nome de pesquisador vazio na linha " +
                std::to_string(line_number)
            );
        }
        result.names[vertex] = std::move(name);
        ++researcher_count;

        for (std::size_t i = 0; i < researcher_names.size(); ++i) {
            if (result.names[vertex] == researcher_names[i]) {
                if (result.required_ids[i] !=
                    std::numeric_limits<std::size_t>::max()) {
                    throw std::runtime_error(
                        "nome de pesquisador repetido: " +
                        result.names[vertex]
                    );
                }
                result.required_ids[i] = vertex;
            }
        }
    }

    if (researcher_count != vertex_count) {
        throw std::runtime_error(
            "quantidade de pesquisadores diferente da quantidade de vertices"
        );
    }

    for (std::size_t i = 0; i < researcher_names.size(); ++i) {
        if (result.required_ids[i] ==
            std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error(
                "pesquisador nao encontrado: " +
                std::string(researcher_names[i])
            );
        }
    }
    return result;
}

std::string format_ids(const std::vector<std::size_t>& path) {
    std::string result;
    for (const std::size_t vertex : path) {
        if (!result.empty()) {
            result += '-';
        }
        result += std::to_string(vertex + 1);
    }
    return result;
}

std::string format_names(
    const std::vector<std::size_t>& path,
    const std::vector<std::string>& names
) {
    std::string result;
    for (const std::size_t vertex : path) {
        if (!result.empty()) {
            result += " -> ";
        }
        result += names[vertex];
    }
    return result;
}

std::string quote_csv(const std::string& value) {
    std::string result = "\"";
    for (const char character : value) {
        result += character;
        if (character == '"') {
            result += '"';
        }
    }
    return result + '"';
}

void run(
    const std::string& graph_path,
    const std::string& researchers_path,
    const std::string& output_path
) {
    const auto graph = read_graph_from_file(
        graph_path,
        GraphRepresentation::AdjacencyList
    );
    if (graph->has_negative_weight()) {
        throw std::domain_error(
            "Dijkstra nao suporta arestas com peso negativo"
        );
    }

    const Researchers researchers = read_researchers(
        researchers_path,
        graph->vertex_count()
    );
    const auto shortest_paths = dijkstra_heap(
        *graph,
        researchers.required_ids[0]
    );

    std::ofstream output(output_path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + output_path);
    }
    output << "source,target,source_id,target_id,status,distance,path_ids,"
              "path_names\n";

    for (std::size_t i = 1; i < researcher_names.size(); ++i) {
        const std::size_t target = researchers.required_ids[i];
        output << quote_csv(researcher_names[0]) << ','
               << quote_csv(researcher_names[i]) << ','
               << researchers.required_ids[0] + 1 << ',' << target + 1 << ',';
        if (!shortest_paths.was_reached(target)) {
            output << "sem_caminho,,,\n";
            continue;
        }

        const auto path = shortest_paths.path_to(target);
        output << "ok," << std::setprecision(12)
               << shortest_paths.distance[target] << ','
               << quote_csv(format_ids(path)) << ','
               << quote_csv(format_names(path, researchers.names)) << '\n';
    }

    std::cout << "Estudo da rede de pesquisadores concluido: "
              << output_path << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: researcher_experiments <grafo.txt> "
                     "<pesquisadores.csv> <saida.csv>\n";
        return EXIT_FAILURE;
    }

    try {
        run(argv[1], argv[2], argv[3]);
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
