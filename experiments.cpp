#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;

struct Configuration {
    std::string input_path;
    GraphRepresentation representation;
    std::string output_prefix;
    std::size_t search_count = 100;
    std::string diameter_mode = "approx";
};

GraphRepresentation parse_representation(const std::string& value) {
    if (value == "list" || value == "lista") {
        return GraphRepresentation::AdjacencyList;
    }
    if (value == "matrix" || value == "matriz") {
        return GraphRepresentation::AdjacencyMatrix;
    }
    throw std::invalid_argument("representacao deve ser list ou matrix");
}

Configuration parse_arguments(int argc, char* argv[]) {
    if (argc < 4 || argc > 6) {
        throw std::invalid_argument(
            "uso: graph_experiments <grafo.txt> <list|matrix> "
            "<prefixo_saida> [numero_buscas] [exact|approx|skip]"
        );
    }

    Configuration result{argv[1], parse_representation(argv[2]), argv[3]};
    if (argc >= 5) {
        std::size_t consumed = 0;
        result.search_count = std::stoull(argv[4], &consumed);
        if (consumed != std::string(argv[4]).size() || result.search_count == 0) {
            throw std::invalid_argument("quantidade de buscas invalida");
        }
    }
    if (argc == 6) {
        result.diameter_mode = argv[5];
    }
    if (result.diameter_mode != "exact" &&
        result.diameter_mode != "approx" &&
        result.diameter_mode != "skip") {
        throw std::invalid_argument("diametro deve ser exact, approx ou skip");
    }
    return result;
}

std::size_t declared_vertex_count(const std::string& path) {
    std::ifstream input(path);
    std::size_t count = 0;
    if (!input || !(input >> count)) {
        throw std::runtime_error("cabecalho invalido em " + path);
    }
    return count;
}

std::size_t physical_memory_bytes() {
#ifdef __APPLE__
    std::uint64_t value = 0;
    std::size_t size = sizeof(value);
    if (sysctlbyname("hw.memsize", &value, &size, nullptr, 0) == 0) {
        return static_cast<std::size_t>(value);
    }
#elif defined(__linux__)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long page_size = sysconf(_SC_PAGESIZE);
    if (pages > 0 && page_size > 0) {
        return static_cast<std::size_t>(pages) * page_size;
    }
#endif
    return 0;
}

void verify_matrix_viability(
    std::size_t vertices,
    GraphRepresentation representation
) {
    if (representation != GraphRepresentation::AdjacencyMatrix) {
        return;
    }
    if (vertices != 0 &&
        vertices > std::numeric_limits<std::size_t>::max() / vertices) {
        throw std::runtime_error("matriz inviavel: tamanho excede size_t");
    }
    const std::size_t estimate = vertices * vertices;
    const std::size_t physical = physical_memory_bytes();
    constexpr std::size_t absolute_limit =
        static_cast<std::size_t>(16) * 1024 * 1024 * 1024;
    if (estimate > absolute_limit ||
        (physical != 0 && estimate > physical * 6 / 10)) {
        const double gib = static_cast<double>(estimate) /
                           (1024.0 * 1024.0 * 1024.0);
        throw std::runtime_error(
            "matriz inviavel: estimativa de " + std::to_string(gib) +
            " GiB excede 60% da memoria fisica"
        );
    }
}

std::size_t resident_memory_bytes() {
#ifdef __APPLE__
    mach_task_basic_info_data_t info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(
            mach_task_self(), MACH_TASK_BASIC_INFO,
            reinterpret_cast<task_info_t>(&info), &count
        ) == KERN_SUCCESS) {
        return static_cast<std::size_t>(info.resident_size);
    }
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    std::size_t total = 0;
    std::size_t resident = 0;
    if (statm >> total >> resident) {
        return resident * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
    }
#endif
    return 0;
}

std::vector<std::size_t> select_starts(
    std::size_t vertices,
    std::size_t requested
) {
    const std::size_t count = std::min(vertices, requested);
    std::vector<std::size_t> result;
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        result.push_back(index * vertices / count);
    }
    return result;
}

template <typename Search>
double average_time_ms(
    const Graph& graph,
    const std::vector<std::size_t>& starts,
    Search search
) {
    if (starts.empty()) {
        return 0.0;
    }
    std::chrono::nanoseconds total{0};
    std::size_t sink = 0;
    for (const std::size_t start : starts) {
        const auto begin = Clock::now();
        const auto result = search(graph, start);
        const auto end = Clock::now();
        total += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
        sink += result.visit_order.size();
    }
    if (sink == std::numeric_limits<std::size_t>::max()) {
        std::cerr << "";
    }
    return static_cast<double>(total.count()) / starts.size() / 1'000'000.0;
}

std::string external_vertex(std::size_t vertex, std::size_t sentinel) {
    return vertex == sentinel ? "nao alcancado" : std::to_string(vertex + 1);
}

void write_queries(std::ostream& output, const Graph& graph) {
    output << "\nPAIS SOLICITADOS (numeracao iniciada em 1)\n";
    for (const std::size_t source : {1U, 2U, 3U}) {
        if (source > graph.vertex_count()) {
            continue;
        }
        const auto bfs = breadth_first_search(graph, source - 1);
        const auto dfs = depth_first_search(graph, source - 1);
        output << "Origem " << source << ":\n";
        for (const std::size_t target : {10U, 20U, 30U}) {
            if (target > graph.vertex_count()) {
                output << "  Vertice " << target << ": inexistente\n";
                continue;
            }
            output << "  Vertice " << target
                   << " | pai BFS: "
                   << external_vertex(
                          bfs.parent[target - 1],
                          BreadthFirstSearchResult::not_visited
                      )
                   << " | pai DFS: "
                   << external_vertex(
                          dfs.parent[target - 1],
                          DepthFirstSearchResult::not_visited
                      ) << '\n';
        }
    }

    output << "\nDISTANCIAS SOLICITADAS\n";
    for (const auto [source, target] :
         {std::pair{10U, 20U}, std::pair{10U, 30U}, std::pair{20U, 30U}}) {
        output << '(' << source << ',' << target << "): ";
        if (target > graph.vertex_count()) {
            output << "vertices inexistentes\n";
            continue;
        }
        const auto value = distance(graph, source - 1, target - 1);
        output << (value ? std::to_string(*value) : "sem caminho") << '\n';
    }
}

void write_components(
    const std::string& path,
    const std::vector<ConnectedComponent>& components
) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + path);
    }
    output << "numero_de_componentes=" << components.size() << '\n';
    for (std::size_t index = 0; index < components.size(); ++index) {
        output << "componente=" << index + 1
               << " tamanho=" << components[index].size()
               << " vertices=";
        for (const std::size_t vertex : components[index].vertices) {
            output << vertex + 1 << ' ';
        }
        output << '\n';
    }
}

void run(const Configuration& configuration) {
    const std::filesystem::path prefix(configuration.output_prefix);
    if (prefix.has_parent_path()) {
        std::filesystem::create_directories(prefix.parent_path());
    }

    verify_matrix_viability(
        declared_vertex_count(configuration.input_path),
        configuration.representation
    );
    const auto graph = read_graph_from_file(
        configuration.input_path,
        configuration.representation
    );
    const std::size_t memory = resident_memory_bytes();
    const auto statistics = calculate_statistics(*graph);

    const auto components_begin = Clock::now();
    const auto components = connected_components(*graph);
    const double components_ms = std::chrono::duration<double, std::milli>(
        Clock::now() - components_begin
    ).count();

    const auto starts = select_starts(
        graph->vertex_count(), configuration.search_count
    );
    const double bfs_ms = average_time_ms(*graph, starts, breadth_first_search);
    const double dfs_ms = average_time_ms(*graph, starts, depth_first_search);

    std::optional<DiameterResult> diameter;
    double diameter_ms = 0.0;
    if (configuration.diameter_mode != "skip") {
        const auto begin = Clock::now();
        diameter = configuration.diameter_mode == "exact"
            ? exact_diameter(*graph)
            : approximate_diameter(*graph);
        diameter_ms = std::chrono::duration<double, std::milli>(
            Clock::now() - begin
        ).count();
    }

    const std::string summary_path = configuration.output_prefix + "_summary.txt";
    std::ofstream output(summary_path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + summary_path);
    }
    output << std::fixed << std::setprecision(3);
    output << "arquivo=" << configuration.input_path << '\n'
           << "representacao="
           << (configuration.representation == GraphRepresentation::AdjacencyList
                   ? "lista" : "matriz") << '\n'
           << "vertices=" << statistics.vertex_count << '\n'
           << "arestas=" << statistics.edge_count << '\n'
           << "grau_minimo=" << statistics.minimum_degree << '\n'
           << "grau_maximo=" << statistics.maximum_degree << '\n'
           << "grau_medio=" << statistics.average_degree << '\n'
           << "mediana_grau=" << statistics.median_degree << '\n'
           << "memoria_apos_carga_mb="
           << static_cast<double>(memory) / (1024.0 * 1024.0) << '\n'
           << "numero_componentes=" << components.size() << '\n'
           << "maior_componente="
           << (components.empty() ? 0 : components.front().size()) << '\n'
           << "menor_componente="
           << (components.empty() ? 0 : components.back().size()) << '\n'
           << "tempo_componentes_ms=" << components_ms << '\n'
           << "buscas_cronometradas=" << starts.size() << '\n'
           << "tempo_medio_bfs_ms=" << bfs_ms << '\n'
           << "tempo_medio_dfs_ms=" << dfs_ms << '\n';

    write_queries(output, *graph);
    output << "\nDIAMETRO\n";
    if (!diameter) {
        output << "modo=nao executado\n";
    } else {
        output << "modo=" << (diameter->is_exact ? "exato" : "aproximado")
               << '\n'
               << "valor=" << diameter->value << '\n'
               << "vertice_1="
               << external_vertex(diameter->first_vertex, DiameterResult::no_vertex)
               << '\n'
               << "vertice_2="
               << external_vertex(diameter->second_vertex, DiameterResult::no_vertex)
               << '\n'
               << "tempo_diametro_ms=" << diameter_ms << '\n';
    }

    write_components(configuration.output_prefix + "_components.txt", components);
    std::cout << "Experimento concluido.\nResumo: " << summary_path
              << "\nComponentes: " << configuration.output_prefix
              << "_components.txt\n";
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        run(parse_arguments(argc, argv));
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
