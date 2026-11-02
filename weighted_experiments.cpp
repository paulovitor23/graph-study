#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

enum class BenchmarkMode {
    Both,
    Vector,
    Heap
};

struct Configuration {
    std::string input_path;
    std::string output_prefix;
    std::size_t source_count = 100;
    std::uint64_t seed = 242;
    BenchmarkMode mode = BenchmarkMode::Both;
};

std::size_t parse_positive_size(const char* value, const char* field) {
    std::size_t consumed = 0;
    const std::size_t result = std::stoull(value, &consumed);
    if (consumed != std::string(value).size() || result == 0) {
        throw std::invalid_argument(std::string(field) + " invalido");
    }
    return result;
}

std::uint64_t parse_seed(const char* value) {
    std::size_t consumed = 0;
    const std::uint64_t result = std::stoull(value, &consumed);
    if (consumed != std::string(value).size()) {
        throw std::invalid_argument("semente invalida");
    }
    return result;
}

BenchmarkMode parse_mode(const std::string& value) {
    if (value == "both") {
        return BenchmarkMode::Both;
    }
    if (value == "vector") {
        return BenchmarkMode::Vector;
    }
    if (value == "heap") {
        return BenchmarkMode::Heap;
    }
    throw std::invalid_argument("modo deve ser both, vector ou heap");
}

Configuration parse_arguments(int argc, char* argv[]) {
    if (argc < 3 || argc > 6) {
        throw std::invalid_argument(
            "uso: weighted_graph_experiments <grafo.txt> <prefixo_saida> "
            "[numero_origens] [semente] [both|vector|heap]"
        );
    }

    Configuration result{argv[1], argv[2]};
    if (argc >= 4) {
        result.source_count = parse_positive_size(argv[3], "numero de origens");
    }
    if (argc == 5) {
        result.seed = parse_seed(argv[4]);
    }
    if (argc == 6) {
        result.seed = parse_seed(argv[4]);
        result.mode = parse_mode(argv[5]);
    }
    return result;
}

std::vector<std::size_t> select_sources(
    std::size_t vertex_count,
    std::size_t requested,
    std::uint64_t seed
) {
    std::vector<std::size_t> sources(vertex_count);
    std::iota(sources.begin(), sources.end(), std::size_t{0});
    std::mt19937_64 generator(seed);
    std::shuffle(sources.begin(), sources.end(), generator);
    sources.resize(std::min(vertex_count, requested));
    return sources;
}

struct BenchmarkResult {
    double vector_total_ms = 0.0;
    double heap_total_ms = 0.0;
};

template <typename Algorithm>
ShortestPathResult measure(
    const Graph& graph,
    std::size_t source,
    Algorithm algorithm,
    double& total_ms
) {
    const auto begin = Clock::now();
    auto result = algorithm(graph, source);
    total_ms += std::chrono::duration<double, std::milli>(
        Clock::now() - begin
    ).count();
    return result;
}

BenchmarkResult benchmark(
    const Graph& graph,
    const std::vector<std::size_t>& sources,
    BenchmarkMode mode
) {
    BenchmarkResult timing;
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const std::size_t source = sources[index];
        ShortestPathResult vector_result;
        ShortestPathResult heap_result;
        const bool run_vector = mode != BenchmarkMode::Heap;
        const bool run_heap = mode != BenchmarkMode::Vector;
        if (mode == BenchmarkMode::Both && index % 2 != 0) {
            heap_result = measure(
                graph,
                source,
                dijkstra_heap,
                timing.heap_total_ms
            );
            vector_result = measure(
                graph,
                source,
                dijkstra_vector,
                timing.vector_total_ms
            );
        } else {
            if (run_vector) {
                vector_result = measure(
                    graph,
                    source,
                    dijkstra_vector,
                    timing.vector_total_ms
                );
            }
            if (run_heap) {
                heap_result = measure(
                    graph,
                    source,
                    dijkstra_heap,
                    timing.heap_total_ms
                );
            }
        }

        if (mode == BenchmarkMode::Both &&
            vector_result.distance != heap_result.distance) {
            throw std::logic_error(
                "Dijkstra com vetor e heap produziram distancias diferentes"
            );
        }
    }
    return timing;
}

std::string format_path(const std::vector<std::size_t>& path) {
    std::string result;
    for (const std::size_t vertex : path) {
        if (!result.empty()) {
            result += '-';
        }
        result += std::to_string(vertex + 1);
    }
    return result;
}

void write_paths(const std::string& path, const Graph& graph) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + path);
    }

    output << "source,target,status,distance,path\n";
    constexpr std::size_t external_source = 10;
    if (external_source > graph.vertex_count()) {
        for (const std::size_t target : {20U, 30U, 40U, 50U, 60U}) {
            output << external_source << ',' << target
                   << ",vertice_inexistente,,\n";
        }
        return;
    }

    const auto result = dijkstra_heap(graph, external_source - 1);
    for (const std::size_t target : {20U, 30U, 40U, 50U, 60U}) {
        output << external_source << ',' << target << ',';
        if (target > graph.vertex_count()) {
            output << "vertice_inexistente,,\n";
            continue;
        }

        if (!result.was_reached(target - 1)) {
            output << "sem_caminho,,\n";
            continue;
        }
        output << "ok," << std::setprecision(12)
               << result.distance[target - 1] << ','
               << format_path(result.path_to(target - 1)) << '\n';
    }
}

void write_sources(
    const std::string& path,
    const std::vector<std::size_t>& sources
) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + path);
    }
    output << "order,source\n";
    for (std::size_t index = 0; index < sources.size(); ++index) {
        output << index + 1 << ',' << sources[index] + 1 << '\n';
    }
}

void write_benchmark(
    const std::string& path,
    const Graph& graph,
    const std::vector<std::size_t>& sources,
    std::uint64_t seed,
    const BenchmarkResult& timing,
    BenchmarkMode mode
) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("nao foi possivel criar " + path);
    }

    output << "algorithm,vertices,edges,sources,seed,total_ms,average_ms\n"
           << std::fixed << std::setprecision(6);
    const double count = static_cast<double>(sources.size());
    if (mode != BenchmarkMode::Heap) {
        output << "vector," << graph.vertex_count() << ',' << graph.edge_count()
               << ',' << sources.size() << ',' << seed << ','
               << timing.vector_total_ms << ','
               << timing.vector_total_ms / count << '\n';
    }
    if (mode != BenchmarkMode::Vector) {
        output << "heap," << graph.vertex_count() << ',' << graph.edge_count()
               << ',' << sources.size() << ',' << seed << ','
               << timing.heap_total_ms << ',' << timing.heap_total_ms / count
               << '\n';
    }
}

void run(const Configuration& configuration) {
    const auto graph = read_graph_from_file(
        configuration.input_path,
        GraphRepresentation::AdjacencyList
    );
    if (graph->vertex_count() == 0) {
        throw std::runtime_error("nao e possivel medir um grafo vazio");
    }
    if (graph->has_negative_weight()) {
        throw std::domain_error(
            "Dijkstra nao suporta arestas com peso negativo"
        );
    }

    const auto sources = select_sources(
        graph->vertex_count(),
        configuration.source_count,
        configuration.seed
    );
    const auto timing = benchmark(*graph, sources, configuration.mode);

    const std::filesystem::path prefix(configuration.output_prefix);
    if (prefix.has_parent_path()) {
        std::filesystem::create_directories(prefix.parent_path());
    }

    const std::string paths_file = configuration.output_prefix + "_paths.csv";
    const std::string sources_file = configuration.output_prefix + "_sources.csv";
    const std::string benchmark_file =
        configuration.output_prefix + "_benchmark.csv";
    write_paths(paths_file, *graph);
    write_sources(sources_file, sources);
    write_benchmark(
        benchmark_file,
        *graph,
        sources,
        configuration.seed,
        timing,
        configuration.mode
    );

    std::cout << "Experimento ponderado concluido.\n"
              << "Caminhos: " << paths_file << '\n'
              << "Origens: " << sources_file << '\n'
              << "Tempos: " << benchmark_file << '\n';
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
