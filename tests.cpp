#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

namespace {

void verify_representation(GraphRepresentation representation) {
    auto graph = make_graph(5, representation);

    graph->add_edge(0, 1);
    graph->add_edge(1, 4);
    graph->add_edge(4, 2);
    graph->add_edge(3, 4);
    graph->add_edge(0, 4);
    graph->add_edge(0, 4); // Arestas repetidas nao devem ser contabilizadas.

    assert(graph->vertex_count() == 5);
    assert(graph->edge_count() == 5);
    assert(graph->has_edge(0, 1));
    assert(graph->has_edge(1, 0));
    assert(!graph->has_edge(0, 2));

    auto adjacent = graph->neighbors(4);
    std::sort(adjacent.begin(), adjacent.end());
    assert((adjacent == std::vector<std::size_t>{0, 1, 2, 3}));
}

void verify_file_reader(GraphRepresentation representation) {
    const char* path = "graph_test_input.txt";
    {
        std::ofstream output(path);
        output << "5\n1 2\n2 5\n5 3\n4 5\n1 5\n";
    }

    auto graph = read_graph_from_file(path, representation);
    assert(graph->vertex_count() == 5);
    assert(graph->edge_count() == 5);
    assert(graph->has_edge(0, 4));
    assert(graph->has_edge(4, 3));

    std::remove(path);
}

void verify_statistics(GraphRepresentation representation) {
    auto graph = make_graph(5, representation);
    graph->add_edge(0, 1);
    graph->add_edge(1, 4);
    graph->add_edge(4, 2);
    graph->add_edge(3, 4);
    graph->add_edge(0, 4);

    assert(degree(*graph, 0) == 2);
    assert(degree(*graph, 4) == 4);
    assert((degrees(*graph) == std::vector<std::size_t>{2, 2, 1, 1, 4}));

    const auto statistics = calculate_statistics(*graph);
    assert(statistics.vertex_count == 5);
    assert(statistics.edge_count == 5);
    assert(statistics.minimum_degree == 1);
    assert(statistics.maximum_degree == 4);
    assert(statistics.average_degree == 2.0);
    assert(statistics.median_degree == 2.0);
}

void verify_even_degree_median(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    graph->add_edge(0, 1);
    graph->add_edge(1, 2);

    const auto statistics = calculate_statistics(*graph);
    assert(statistics.minimum_degree == 0);
    assert(statistics.maximum_degree == 2);
    assert(statistics.average_degree == 1.0);
    assert(statistics.median_degree == 1.0);
}

void verify_empty_graph_statistics(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);
    const auto statistics = calculate_statistics(*graph);

    assert(statistics.vertex_count == 0);
    assert(statistics.edge_count == 0);
    assert(statistics.minimum_degree == 0);
    assert(statistics.maximum_degree == 0);
    assert(statistics.average_degree == 0.0);
    assert(statistics.median_degree == 0.0);
}

} // namespace

int main() {
    verify_representation(GraphRepresentation::AdjacencyList);
    verify_representation(GraphRepresentation::AdjacencyMatrix);
    verify_file_reader(GraphRepresentation::AdjacencyList);
    verify_file_reader(GraphRepresentation::AdjacencyMatrix);
    verify_statistics(GraphRepresentation::AdjacencyList);
    verify_statistics(GraphRepresentation::AdjacencyMatrix);
    verify_even_degree_median(GraphRepresentation::AdjacencyList);
    verify_even_degree_median(GraphRepresentation::AdjacencyMatrix);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyList);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyMatrix);

    std::cout << "Todos os testes passaram.\n";
    return 0;
}
