#ifndef GRAPH_STUDY_ALGORITHMS_HPP
#define GRAPH_STUDY_ALGORITHMS_HPP

#include "graph.hpp"

#include <cstddef>
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

#endif
