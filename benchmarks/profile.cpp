#include "algorithms.hpp"
#include "experiments/common.hpp"
#include "sampling.hpp"

#include <iomanip>
#include <iostream>
#include <string>

#ifdef __linux__
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace {

std::uint64_t digest = 14695981039346656037ULL;

template <typename T>
void consume(const T& value) {
    const auto* data = reinterpret_cast<const unsigned char*>(&value);
    for (std::size_t i = 0; i < sizeof(value); ++i) {
        digest = (digest ^ data[i]) * 1099511628211ULL;
    }
}

template <typename T>
void consume_vector(const std::vector<T>& values) {
    for (const auto& value : values) consume(value);
}

void report(const char* metric, double ms) {
    std::size_t resident = 0, peak = 0;
#ifdef __linux__
    std::ifstream statm("/proc/self/statm");
    std::size_t total = 0;
    if (statm >> total >> resident) resident *= sysconf(_SC_PAGESIZE);
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) peak = usage.ru_maxrss * 1024ULL;
#endif
    std::cout << metric << ',' << std::setprecision(12) << ms << ','
              << resident << ',' << peak << ',' << digest << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 4) {
            throw std::invalid_argument("uso: graph_profile <grafo> <origens> <load|search|all|matrix>");
        }
        const auto count = experiment::parse_positive_size(argv[2], "origens");
        const std::string mode = argv[3];
        if (mode != "load" && mode != "search" && mode != "all" && mode != "matrix") {
            throw std::invalid_argument("modo invalido");
        }
        std::cout << "metric,ms,resident_bytes,peak_bytes,digest\n";
        auto begin = experiment::Clock::now();
        const auto graph = read_graph_from_file(argv[1], mode == "matrix"
            ? GraphRepresentation::AdjacencyMatrix : GraphRepresentation::AdjacencyList);
        const auto load_ms = experiment::elapsed_ms(begin);
        consume(graph->vertex_count());
        consume(graph->edge_count());
        for (std::size_t u = 0; u < graph->vertex_count(); ++u) {
            for (const auto& edge : graph->neighbor_view(u)) {
                consume(edge.vertex);
                consume(edge.weight);
            }
        }
        report("load", load_ms);
        if (mode == "load") return 0;
        begin = experiment::Clock::now();
        const auto statistics = calculate_statistics(*graph);
        const auto statistics_ms = experiment::elapsed_ms(begin);
        consume(statistics.minimum_degree);
        consume(statistics.maximum_degree);
        consume(statistics.average_degree);
        consume(statistics.median_degree);
        report("statistics", statistics_ms);
        for (const auto source : select_sources(graph->vertex_count(), count, 242)) {
            consume(source);
            begin = experiment::Clock::now();
            const auto bfs = breadth_first_search(*graph, source);
            const auto bfs_ms = experiment::elapsed_ms(begin);
            consume_vector(bfs.parent); consume_vector(bfs.level); consume_vector(bfs.visit_order);
            report("bfs", bfs_ms);
            begin = experiment::Clock::now();
            const auto dfs = depth_first_search(*graph, source);
            const auto dfs_ms = experiment::elapsed_ms(begin);
            consume_vector(dfs.parent); consume_vector(dfs.level); consume_vector(dfs.visit_order);
            report("dfs", dfs_ms);
            if (!graph->has_negative_weight()) {
                begin = experiment::Clock::now();
                const auto heap = dijkstra_heap(*graph, source);
                const auto heap_ms = experiment::elapsed_ms(begin);
                consume_vector(heap.distance); consume_vector(heap.parent);
                report("heap", heap_ms);
                if (mode == "all" || mode == "matrix") {
                    begin = experiment::Clock::now();
                    const auto linear = dijkstra_vector(*graph, source);
                    const auto linear_ms = experiment::elapsed_ms(begin);
                    if (heap.distance != linear.distance || heap.parent != linear.parent) {
                        throw std::logic_error("Dijkstras diferentes");
                    }
                    consume_vector(linear.distance); consume_vector(linear.parent);
                    report("vector", linear_ms);
                }
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
