#include "algorithms.hpp"

#include <algorithm>
#include <numeric>
#include <queue>
#include <stdexcept>

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

bool BreadthFirstSearchResult::was_visited(std::size_t vertex) const {
    if (vertex >= level.size()) {
        throw std::out_of_range("vertice fora do resultado da busca");
    }
    return level[vertex] != not_visited;
}

BreadthFirstSearchResult breadth_first_search(
    const Graph& graph,
    std::size_t source
) {
    if (source >= graph.vertex_count()) {
        throw std::out_of_range("vertice inicial fora do intervalo do grafo");
    }

    BreadthFirstSearchResult result;
    result.source = source;
    result.parent.assign(
        graph.vertex_count(),
        BreadthFirstSearchResult::not_visited
    );
    result.level.assign(
        graph.vertex_count(),
        BreadthFirstSearchResult::not_visited
    );
    result.visit_order.reserve(graph.vertex_count());

    std::queue<std::size_t> pending;
    result.parent[source] = source;
    result.level[source] = 0;
    pending.push(source);

    while (!pending.empty()) {
        const std::size_t current = pending.front();
        pending.pop();
        result.visit_order.push_back(current);

        for (const std::size_t adjacent : graph.neighbors(current)) {
            if (result.level[adjacent] != BreadthFirstSearchResult::not_visited) {
                continue;
            }

            result.parent[adjacent] = current;
            result.level[adjacent] = result.level[current] + 1;
            pending.push(adjacent);
        }
    }

    return result;
}

std::optional<std::size_t> distance(
    const Graph& graph,
    std::size_t source,
    std::size_t destination
) {
    if (destination >= graph.vertex_count()) {
        throw std::out_of_range("vertice de destino fora do intervalo do grafo");
    }

    const auto search = breadth_first_search(graph, source);
    if (!search.was_visited(destination)) {
        return std::nullopt;
    }
    return search.level[destination];
}
