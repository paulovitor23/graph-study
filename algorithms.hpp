#ifndef GRAPH_STUDY_ALGORITHMS_HPP
#define GRAPH_STUDY_ALGORITHMS_HPP

#include "graph.hpp"

#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

struct GraphStatistics {
    std::size_t vertex_count = 0;
    std::size_t edge_count = 0;
    std::size_t minimum_degree = 0;
    std::size_t maximum_degree = 0;
    double average_degree = 0.0;
    double median_degree = 0.0;
};

std::size_t degree(const Graph& graph, std::size_t vertex);
std::vector<std::size_t> degrees(const Graph& graph);
GraphStatistics calculate_statistics(const Graph& graph);

struct BreadthFirstSearchResult {
    static constexpr std::size_t not_visited =
        std::numeric_limits<std::size_t>::max();

    std::size_t source = not_visited;
    std::vector<std::size_t> parent;
    std::vector<std::size_t> level;
    std::vector<std::size_t> visit_order;

    bool was_visited(std::size_t vertex) const;
};

BreadthFirstSearchResult breadth_first_search(
    const Graph& graph,
    std::size_t source
);

std::optional<std::size_t> distance(
    const Graph& graph,
    std::size_t source,
    std::size_t destination
);

struct DepthFirstSearchResult {
    static constexpr std::size_t not_visited =
        std::numeric_limits<std::size_t>::max();

    std::size_t source = not_visited;
    std::vector<std::size_t> parent;
    std::vector<std::size_t> level;
    std::vector<std::size_t> visit_order;

    bool was_visited(std::size_t vertex) const;
};

DepthFirstSearchResult depth_first_search(
    const Graph& graph,
    std::size_t source
);

#endif
