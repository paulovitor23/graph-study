#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

double edge_weight(
    const Graph& graph,
    std::size_t source,
    std::size_t destination
) {
    for (const Edge& edge : graph.neighbors(source)) {
        if (edge.vertex == destination) {
            return edge.weight;
        }
    }
    throw std::runtime_error("aresta esperada nao encontrada no teste");
}

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
    std::sort(
        adjacent.begin(),
        adjacent.end(),
        [](const Edge& left, const Edge& right) {
            return left.vertex < right.vertex;
        }
    );
    assert(adjacent.size() == 4);
    for (std::size_t index = 0; index < adjacent.size(); ++index) {
        assert(adjacent[index].vertex == index);
        assert(adjacent[index].weight == 1.0);
    }
}

void verify_weighted_edges(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    assert(!graph->has_negative_weight());
    graph->add_edge(0, 1, 2.5);
    graph->add_edge(1, 2, 0.0);
    assert(!graph->has_negative_weight());
    graph->add_edge(2, 3, -4.25);
    assert(graph->has_negative_weight());
    graph->add_edge(0, 1, 2.5);

    assert(graph->edge_count() == 3);
    assert(edge_weight(*graph, 0, 1) == 2.5);
    assert(edge_weight(*graph, 1, 0) == 2.5);
    assert(edge_weight(*graph, 1, 2) == 0.0);
    assert(edge_weight(*graph, 2, 3) == -4.25);
    assert(edge_weight(*graph, 3, 2) == -4.25);

    graph->add_edge(1, 0, 3.0);
    assert(edge_weight(*graph, 0, 1) == 2.5);
    graph->add_edge(1, 0, 1.25);
    assert(edge_weight(*graph, 0, 1) == 1.25);
    assert(edge_weight(*graph, 1, 0) == 1.25);
    assert(graph->edge_count() == 3);

    bool infinite_weight_rejected = false;
    try {
        graph->add_edge(
            0,
            3,
            std::numeric_limits<double>::infinity()
        );
    } catch (const std::invalid_argument&) {
        infinite_weight_rejected = true;
    }
    assert(infinite_weight_rejected);
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

void verify_weighted_file_reader(GraphRepresentation representation) {
    const char* path = "weighted_graph_test_input.txt";
    {
        std::ofstream output(path);
        output << "5\n1 2 0.1\n2 5 0\n5 3 -9.5\n3 4 2.3\n";
    }

    auto graph = read_graph_from_file(path, representation);
    assert(graph->vertex_count() == 5);
    assert(graph->edge_count() == 4);
    assert(edge_weight(*graph, 0, 1) == 0.1);
    assert(edge_weight(*graph, 1, 4) == 0.0);
    assert(edge_weight(*graph, 4, 2) == -9.5);
    assert(edge_weight(*graph, 2, 3) == 2.3);

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

void verify_invalid_weight_is_rejected(GraphRepresentation representation) {
    const char* path = "graph_test_invalid_weight.txt";
    {
        std::ofstream output(path);
        output << "3\n1 2 peso\n";
    }

    bool rejected = false;
    try {
        read_graph_from_file(path, representation);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    std::remove(path);
    assert(rejected);
}

void verify_extra_column_is_rejected(GraphRepresentation representation) {
    const char* path = "graph_test_extra_column.txt";
    {
        std::ofstream output(path);
        output << "3\n1 2 4.5 extra\n";
    }

    bool rejected = false;
    try {
        read_graph_from_file(path, representation);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    std::remove(path);
    assert(rejected);
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

void verify_dijkstra_algorithms(GraphRepresentation representation) {
    auto graph = make_graph(7, representation);
    graph->add_edge(0, 1, 7.0);
    graph->add_edge(0, 2, 9.0);
    graph->add_edge(0, 5, 14.0);
    graph->add_edge(1, 2, 10.0);
    graph->add_edge(1, 3, 15.0);
    graph->add_edge(2, 3, 11.0);
    graph->add_edge(2, 5, 2.0);
    graph->add_edge(3, 4, 6.0);
    graph->add_edge(4, 5, 9.0);

    const auto result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    assert(heap_result.source == result.source);
    assert(heap_result.distance == result.distance);
    assert(heap_result.parent == result.parent);

    assert(result.source == 0);
    assert(result.distance[0] == 0.0);
    assert(result.distance[1] == 7.0);
    assert(result.distance[2] == 9.0);
    assert(result.distance[3] == 20.0);
    assert(result.distance[4] == 20.0);
    assert(result.distance[5] == 11.0);
    assert(std::isinf(result.distance[6]));

    assert(result.parent[0] == 0);
    assert((result.path_to(0) == std::vector<std::size_t>{0}));
    assert((result.path_to(3) == std::vector<std::size_t>{0, 2, 3}));
    assert((result.path_to(4) == std::vector<std::size_t>{0, 2, 5, 4}));
    assert(result.path_to(6).empty());
    assert(result.was_reached(5));
    assert(!result.was_reached(6));
    assert(heap_result.path_to(4) == result.path_to(4));
    assert(heap_result.path_to(6).empty());
}

void verify_dijkstra_zero_weight(GraphRepresentation representation) {
    auto graph = make_graph(3, representation);
    graph->add_edge(0, 1, 0.0);
    graph->add_edge(1, 2, 2.0);
    graph->add_edge(0, 2, 5.0);

    const auto result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    assert(heap_result.distance == result.distance);
    assert(heap_result.parent == result.parent);
    assert(result.distance[1] == 0.0);
    assert(result.distance[2] == 2.0);
    assert((result.path_to(2) == std::vector<std::size_t>{0, 1, 2}));
}

void verify_dijkstra_real_weights(GraphRepresentation representation) {
    auto graph = make_graph(3, representation);
    graph->add_edge(0, 1, 0.1);
    graph->add_edge(1, 2, 0.2);
    graph->add_edge(0, 2, 0.5);

    const auto result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    assert(heap_result.distance == result.distance);
    assert(heap_result.parent == result.parent);
    assert(std::abs(result.distance[2] - 0.3) < 1e-12);
    assert((result.path_to(2) == std::vector<std::size_t>{0, 1, 2}));
}

void verify_dijkstra_tie_break(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    graph->add_edge(0, 2, 1.0);
    graph->add_edge(0, 1, 1.0);
    graph->add_edge(2, 3, 1.0);
    graph->add_edge(1, 3, 1.0);

    const auto vector_result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    assert(vector_result.distance == heap_result.distance);
    assert(vector_result.parent == heap_result.parent);
    assert(vector_result.parent[3] == 1);
}

void verify_dijkstra_rejects_negative_weight(
    GraphRepresentation representation
) {
    auto graph = make_graph(3, representation);
    graph->add_edge(0, 1, 1.0);
    graph->add_edge(1, 2, -0.5);

    bool rejected = false;
    try {
        dijkstra_vector(*graph, 0);
    } catch (const std::domain_error&) {
        rejected = true;
    }
    assert(rejected);

    rejected = false;
    try {
        dijkstra_heap(*graph, 0);
    } catch (const std::domain_error&) {
        rejected = true;
    }
    assert(rejected);
}

void verify_dijkstra_invalid_vertices(GraphRepresentation representation) {
    auto empty_graph = make_graph(0, representation);
    bool empty_graph_rejected = false;
    try {
        dijkstra_vector(*empty_graph, 0);
    } catch (const std::out_of_range&) {
        empty_graph_rejected = true;
    }
    assert(empty_graph_rejected);

    empty_graph_rejected = false;
    try {
        dijkstra_heap(*empty_graph, 0);
    } catch (const std::out_of_range&) {
        empty_graph_rejected = true;
    }
    assert(empty_graph_rejected);

    auto graph = make_graph(2, representation);
    graph->add_edge(0, 1, 1.0);

    bool invalid_source_rejected = false;
    try {
        dijkstra_vector(*graph, 2);
    } catch (const std::out_of_range&) {
        invalid_source_rejected = true;
    }
    assert(invalid_source_rejected);

    invalid_source_rejected = false;
    try {
        dijkstra_heap(*graph, 2);
    } catch (const std::out_of_range&) {
        invalid_source_rejected = true;
    }
    assert(invalid_source_rejected);

    const auto result = dijkstra_vector(*graph, 0);
    bool invalid_target_rejected = false;
    try {
        result.path_to(2);
    } catch (const std::out_of_range&) {
        invalid_target_rejected = true;
    }
    assert(invalid_target_rejected);
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
    verify_weighted_edges(GraphRepresentation::AdjacencyList);
    verify_weighted_edges(GraphRepresentation::AdjacencyMatrix);
    verify_file_reader(GraphRepresentation::AdjacencyList);
    verify_file_reader(GraphRepresentation::AdjacencyMatrix);
    verify_weighted_file_reader(GraphRepresentation::AdjacencyList);
    verify_weighted_file_reader(GraphRepresentation::AdjacencyMatrix);
    verify_incomplete_edge_is_rejected(GraphRepresentation::AdjacencyList);
    verify_incomplete_edge_is_rejected(GraphRepresentation::AdjacencyMatrix);
    verify_invalid_weight_is_rejected(GraphRepresentation::AdjacencyList);
    verify_invalid_weight_is_rejected(GraphRepresentation::AdjacencyMatrix);
    verify_extra_column_is_rejected(GraphRepresentation::AdjacencyList);
    verify_extra_column_is_rejected(GraphRepresentation::AdjacencyMatrix);
    verify_statistics(GraphRepresentation::AdjacencyList);
    verify_statistics(GraphRepresentation::AdjacencyMatrix);
    verify_even_degree_median(GraphRepresentation::AdjacencyList);
    verify_even_degree_median(GraphRepresentation::AdjacencyMatrix);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyList);
    verify_empty_graph_statistics(GraphRepresentation::AdjacencyMatrix);
    verify_breadth_first_search(GraphRepresentation::AdjacencyList);
    verify_breadth_first_search(GraphRepresentation::AdjacencyMatrix);
    verify_dijkstra_algorithms(GraphRepresentation::AdjacencyList);
    verify_dijkstra_algorithms(GraphRepresentation::AdjacencyMatrix);
    verify_dijkstra_zero_weight(GraphRepresentation::AdjacencyList);
    verify_dijkstra_zero_weight(GraphRepresentation::AdjacencyMatrix);
    verify_dijkstra_real_weights(GraphRepresentation::AdjacencyList);
    verify_dijkstra_real_weights(GraphRepresentation::AdjacencyMatrix);
    verify_dijkstra_tie_break(GraphRepresentation::AdjacencyList);
    verify_dijkstra_tie_break(GraphRepresentation::AdjacencyMatrix);
    verify_dijkstra_rejects_negative_weight(
        GraphRepresentation::AdjacencyList
    );
    verify_dijkstra_rejects_negative_weight(
        GraphRepresentation::AdjacencyMatrix
    );
    verify_dijkstra_invalid_vertices(GraphRepresentation::AdjacencyList);
    verify_dijkstra_invalid_vertices(GraphRepresentation::AdjacencyMatrix);
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
