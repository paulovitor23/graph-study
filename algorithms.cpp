#include "algorithms.hpp"

#include <algorithm>
#include <numeric>

std::size_t degree(const Graph& graph, std::size_t vertex) {
    return graph.neighbors(vertex).size();
}

std::vector<std::size_t> degrees(const Graph& graph) {
    std::vector<std::size_t> result;
    result.reserve(graph.vertex_count());

    for (std::size_t vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        result.push_back(degree(graph, vertex));
    }

    return result;
}

GraphStatistics calculate_statistics(const Graph& graph) {
    GraphStatistics statistics;
    statistics.vertex_count = graph.vertex_count();
    statistics.edge_count = graph.edge_count();

    auto graph_degrees = degrees(graph);
    if (graph_degrees.empty()) {
        return statistics;
    }

    const auto [minimum, maximum] = std::minmax_element(
        graph_degrees.begin(),
        graph_degrees.end()
    );
    statistics.minimum_degree = *minimum;
    statistics.maximum_degree = *maximum;

    const std::size_t degree_sum = std::accumulate(
        graph_degrees.begin(),
        graph_degrees.end(),
        std::size_t{0}
    );
    statistics.average_degree = static_cast<double>(degree_sum) /
                                static_cast<double>(graph_degrees.size());

    std::sort(graph_degrees.begin(), graph_degrees.end());
    const std::size_t middle = graph_degrees.size() / 2;
    if (graph_degrees.size() % 2 == 0) {
        statistics.median_degree =
            (static_cast<double>(graph_degrees[middle - 1]) +
             static_cast<double>(graph_degrees[middle])) /
            2.0;
    } else {
        statistics.median_degree = static_cast<double>(graph_degrees[middle]);
    }

    return statistics;
}
