#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
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

void verify_incomplete_edge_is_rejected(GraphRepresentation representation) {
    const char* path = "graph_test_incomplete_edge.txt";
    {
        std::ofstream output(path);
        output << "3\n1 2\n2\n";
    }

    bool rejected = false;
    try {
        read_graph_from_file(path, representation);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    std::remove(path);
    if (!rejected) {
        throw std::runtime_error("leitor aceitou uma aresta incompleta");
    }
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

void verify_breadth_first_search(GraphRepresentation representation) {
    auto graph = make_graph(6, representation);
    graph->add_edge(0, 1);
    graph->add_edge(0, 2);
    graph->add_edge(1, 3);
    graph->add_edge(2, 4);

    const auto search = breadth_first_search(*graph, 0);

    assert(search.source == 0);
    assert(search.parent[0] == 0);
    assert(search.parent[1] == 0);
    assert(search.parent[2] == 0);
    assert(search.parent[3] == 1);
    assert(search.parent[4] == 2);
    assert(search.parent[5] == BreadthFirstSearchResult::not_visited);

    assert(search.level[0] == 0);
    assert(search.level[1] == 1);
    assert(search.level[2] == 1);
    assert(search.level[3] == 2);
    assert(search.level[4] == 2);
    assert(search.level[5] == BreadthFirstSearchResult::not_visited);

    assert(search.was_visited(4));
    assert(!search.was_visited(5));
    assert((search.visit_order == std::vector<std::size_t>{0, 1, 2, 3, 4}));

    assert(distance(*graph, 0, 4) == std::optional<std::size_t>{2});
    assert(distance(*graph, 3, 4) == std::optional<std::size_t>{4});
    assert(distance(*graph, 0, 5) == std::nullopt);
}

void verify_depth_first_search(GraphRepresentation representation) {
    auto graph = make_graph(6, representation);
    graph->add_edge(0, 1);
    graph->add_edge(0, 2);
    graph->add_edge(1, 3);
    graph->add_edge(2, 4);

    const auto search = depth_first_search(*graph, 0);

    assert(search.source == 0);
    assert(search.parent[0] == 0);
    assert(search.parent[1] == 0);
    assert(search.parent[2] == 0);
    assert(search.parent[3] == 1);
    assert(search.parent[4] == 2);
    assert(search.parent[5] == DepthFirstSearchResult::not_visited);

    assert(search.level[0] == 0);
    assert(search.level[1] == 1);
    assert(search.level[2] == 1);
    assert(search.level[3] == 2);
    assert(search.level[4] == 2);
    assert(search.level[5] == DepthFirstSearchResult::not_visited);

    assert(search.was_visited(4));
    assert(!search.was_visited(5));
    assert((search.visit_order == std::vector<std::size_t>{0, 1, 3, 2, 4}));
}

void verify_connected_components(GraphRepresentation representation) {
    auto graph = make_graph(8, representation);

    // Componente {0, 1, 2, 3}.
    graph->add_edge(0, 1);
    graph->add_edge(1, 2);
    graph->add_edge(2, 3);
    graph->add_edge(3, 0);

    // Componente {4, 5}; 6 e 7 sao vertices isolados.
    graph->add_edge(4, 5);

    const auto components = connected_components(*graph);

    assert(components.size() == 4);
    assert(components[0].size() == 4);
    assert((components[0].vertices == std::vector<std::size_t>{0, 1, 2, 3}));
    assert(components[1].size() == 2);
    assert((components[1].vertices == std::vector<std::size_t>{4, 5}));
    assert((components[2].vertices == std::vector<std::size_t>{6}));
    assert((components[3].vertices == std::vector<std::size_t>{7}));
}

void verify_empty_graph_components(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);
    assert(connected_components(*graph).empty());
}

void verify_diameter(GraphRepresentation representation) {
    auto path = make_graph(5, representation);
    path->add_edge(0, 1);
    path->add_edge(1, 2);
    path->add_edge(2, 3);
    path->add_edge(3, 4);

    const auto exact_path = exact_diameter(*path);
    assert(exact_path.is_exact);
    assert(exact_path.value == 4);
    assert(exact_path.first_vertex == 0);
    assert(exact_path.second_vertex == 4);

    const auto approximate_path = approximate_diameter(*path);
    assert(!approximate_path.is_exact);
    assert(approximate_path.value == 4);
    assert(approximate_path.first_vertex == 4);
    assert(approximate_path.second_vertex == 0);

    auto disconnected = make_graph(7, representation);
    // Componente triangular com diametro 1.
    disconnected->add_edge(0, 1);
    disconnected->add_edge(1, 2);
    disconnected->add_edge(2, 0);
    // Componente em caminho com diametro 2; vertice 6 e isolado.
    disconnected->add_edge(3, 4);
    disconnected->add_edge(4, 5);

    const auto exact_disconnected = exact_diameter(*disconnected);
    assert(exact_disconnected.value == 2);
    assert(exact_disconnected.first_vertex == 3);
    assert(exact_disconnected.second_vertex == 5);

    const auto approximate_disconnected = approximate_diameter(*disconnected);
    assert(approximate_disconnected.value == 2);
}

void verify_empty_graph_diameter(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);

    const auto exact = exact_diameter(*graph);
    assert(exact.is_exact);
    assert(exact.value == 0);
    assert(exact.first_vertex == DiameterResult::no_vertex);
    assert(exact.second_vertex == DiameterResult::no_vertex);

    const auto approximate = approximate_diameter(*graph);
    assert(!approximate.is_exact);
    assert(approximate.value == 0);
    assert(approximate.first_vertex == DiameterResult::no_vertex);
    assert(approximate.second_vertex == DiameterResult::no_vertex);
}

} // namespace

int main() {
    verify_representation(GraphRepresentation::AdjacencyList);
    verify_representation(GraphRepresentation::AdjacencyMatrix);
    verify_file_reader(GraphRepresentation::AdjacencyList);
    verify_file_reader(GraphRepresentation::AdjacencyMatrix);
    verify_incomplete_edge_is_rejected(GraphRepresentation::AdjacencyList);
    verify_incomplete_edge_is_rejected(GraphRepresentation::AdjacencyMatrix);
    verify_statistics(GraphRepresentation::AdjacencyList);
    verify_statistics(GraphRepresentation::AdjacencyMatrix);
    verify_even_degree_median(GraphRepresentation::AdjacencyList);
    verify_even_degree_median(GraphRepresentation::AdjacencyMatrix);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyList);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyMatrix);
    verify_breadth_first_search(GraphRepresentation::AdjacencyList);
    verify_breadth_first_search(GraphRepresentation::AdjacencyMatrix);
    verify_depth_first_search(GraphRepresentation::AdjacencyList);
    verify_depth_first_search(GraphRepresentation::AdjacencyMatrix);
    verify_connected_components(GraphRepresentation::AdjacencyList);
    verify_connected_components(GraphRepresentation::AdjacencyMatrix);
    verify_empty_graph_components(GraphRepresentation::AdjacencyList);
    verify_empty_graph_components(GraphRepresentation::AdjacencyMatrix);
    verify_diameter(GraphRepresentation::AdjacencyList);
    verify_diameter(GraphRepresentation::AdjacencyMatrix);
    verify_empty_graph_diameter(GraphRepresentation::AdjacencyList);
    verify_empty_graph_diameter(GraphRepresentation::AdjacencyMatrix);

    std::cout << "Todos os testes passaram.\n";
    return 0;
}
