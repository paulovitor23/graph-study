#include "algorithms.hpp"
#include "graph.hpp"

#include <algorithm>
#include "sampling.hpp"
#include <string>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        throw std::runtime_error(
            std::string(file) + ':' + std::to_string(line) + ": " + expression
        );
    }
}

#define CHECK(condition) require((condition), #condition, __FILE__, __LINE__)

void verify_sampling() {
    CHECK(select_sources(0, 10, 242).empty());
    CHECK(select_sources(10, 0, 242).empty());
    for (std::size_t n : {1U, 5U, 100U, 50000000U}) {
        auto sample = select_sources(n, 10, 242);
        CHECK(sample == select_sources(n, 10, 242));
        CHECK(sample.size() == std::min(n, std::size_t{10}));
        CHECK(sample.capacity() <= 10);
        std::sort(sample.begin(), sample.end());
        CHECK(std::adjacent_find(sample.begin(), sample.end()) == sample.end());
        CHECK(sample.back() < n);
    }
}

void verify_bulk_loading(GraphRepresentation representation) {
    const char* path = "graph_test_bulk.txt";
    {
        std::ofstream output(path);
        output << "6\n1 4 8\n1 2 5\n4 1 2\n1 4 9\n1 1 4\n1 1 0\n2 1 3\n5 6 -1\n";
    }
    auto graph = read_graph_from_file(path, representation);
    std::remove(path);
    auto expected = make_graph(6, representation);
    expected->add_edge(0, 3, 8);
    expected->add_edge(0, 1, 5);
    expected->add_edge(3, 0, 2);
    expected->add_edge(0, 3, 9);
    expected->add_edge(0, 0, 4);
    expected->add_edge(0, 0, 0);
    expected->add_edge(1, 0, 3);
    expected->add_edge(4, 5, -1);
    CHECK(graph->edge_count() == 4);
    CHECK(graph->has_negative_weight());
    for (std::size_t v = 0; v < 6; ++v) {
        const auto actual = graph->neighbor_view(v);
        const auto wanted = expected->neighbors(v);
        CHECK(actual.size() == wanted.size());
        for (std::size_t i = 0; i < wanted.size(); ++i) {
            CHECK(actual[i].vertex == wanted[i].vertex);
            CHECK(actual[i].weight == wanted[i].weight);
        }
    }
    graph->add_edge(3, 0, 1);
    CHECK(graph->edge_count() == 4);
    CHECK(graph->neighbors(3).front().weight == 1);
}

void verify_borrowed_neighbors() {
    AdjacencyListGraph graph(3);
    graph.add_edge(0, 2, 2);
    const auto first = graph.neighbor_view(0);
    const auto second = graph.neighbor_view(0);
    CHECK(first.begin() == second.begin());
    CHECK(first.size() == 1);
    CHECK(graph.neighbor_view(1).size() == 0);
    bool rejected = false;
    try { graph.neighbor_view(3); }
    catch (const std::out_of_range&) { rejected = true; }
    CHECK(rejected);
}

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

    CHECK(graph->vertex_count() == 5);
    CHECK(graph->edge_count() == 5);
    CHECK(graph->has_edge(0, 1));
    CHECK(graph->has_edge(1, 0));
    CHECK(!graph->has_edge(0, 2));

    auto adjacent = graph->neighbors(4);
    std::sort(
        adjacent.begin(),
        adjacent.end(),
        [](const Edge& left, const Edge& right) {
            return left.vertex < right.vertex;
        }
    );
    CHECK(adjacent.size() == 4);
    for (std::size_t index = 0; index < adjacent.size(); ++index) {
        CHECK(adjacent[index].vertex == index);
        CHECK(adjacent[index].weight == 1.0);
    }
}

void verify_weighted_edges(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    CHECK(!graph->has_negative_weight());
    graph->add_edge(0, 1, 2.5);
    graph->add_edge(1, 2, 0.0);
    CHECK(!graph->has_negative_weight());
    graph->add_edge(2, 3, -4.25);
    CHECK(graph->has_negative_weight());
    graph->add_edge(0, 1, 2.5);

    CHECK(graph->edge_count() == 3);
    CHECK(edge_weight(*graph, 0, 1) == 2.5);
    CHECK(edge_weight(*graph, 1, 0) == 2.5);
    CHECK(edge_weight(*graph, 1, 2) == 0.0);
    CHECK(edge_weight(*graph, 2, 3) == -4.25);
    CHECK(edge_weight(*graph, 3, 2) == -4.25);

    graph->add_edge(1, 0, 3.0);
    CHECK(edge_weight(*graph, 0, 1) == 2.5);
    graph->add_edge(1, 0, 1.25);
    CHECK(edge_weight(*graph, 0, 1) == 1.25);
    CHECK(edge_weight(*graph, 1, 0) == 1.25);
    CHECK(graph->edge_count() == 3);

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
    CHECK(infinite_weight_rejected);
}

void verify_file_reader(GraphRepresentation representation) {
    const char* path = "graph_test_input.txt";
    {
        std::ofstream output(path);
        output << "5\n1 2\n2 5\n5 3\n4 5\n1 5\n";
    }

    auto graph = read_graph_from_file(path, representation);
    CHECK(graph->vertex_count() == 5);
    CHECK(graph->edge_count() == 5);
    CHECK(graph->has_edge(0, 4));
    CHECK(graph->has_edge(4, 3));

    std::remove(path);
}

void verify_weighted_file_reader(GraphRepresentation representation) {
    const char* path = "weighted_graph_test_input.txt";
    {
        std::ofstream output(path);
        output << "5\n1 2 0.1\n2 5 0\n5 3 -9.5\n3 4 2.3\n";
    }

    auto graph = read_graph_from_file(path, representation);
    CHECK(graph->vertex_count() == 5);
    CHECK(graph->edge_count() == 4);
    CHECK(edge_weight(*graph, 0, 1) == 0.1);
    CHECK(edge_weight(*graph, 1, 4) == 0.0);
    CHECK(edge_weight(*graph, 4, 2) == -9.5);
    CHECK(edge_weight(*graph, 2, 3) == 2.3);

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
    CHECK(rejected);
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
    CHECK(rejected);
}

void verify_statistics(GraphRepresentation representation) {
    auto graph = make_graph(5, representation);
    graph->add_edge(0, 1);
    graph->add_edge(1, 4);
    graph->add_edge(4, 2);
    graph->add_edge(3, 4);
    graph->add_edge(0, 4);

    CHECK(degree(*graph, 0) == 2);
    CHECK(degree(*graph, 4) == 4);
    CHECK((degrees(*graph) == std::vector<std::size_t>{2, 2, 1, 1, 4}));

    const auto statistics = calculate_statistics(*graph);
    CHECK(statistics.vertex_count == 5);
    CHECK(statistics.edge_count == 5);
    CHECK(statistics.minimum_degree == 1);
    CHECK(statistics.maximum_degree == 4);
    CHECK(statistics.average_degree == 2.0);
    CHECK(statistics.median_degree == 2.0);
}

void verify_even_degree_median(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    graph->add_edge(0, 1);
    graph->add_edge(1, 2);

    const auto statistics = calculate_statistics(*graph);
    CHECK(statistics.minimum_degree == 0);
    CHECK(statistics.maximum_degree == 2);
    CHECK(statistics.average_degree == 1.0);
    CHECK(statistics.median_degree == 1.0);
}

void verify_empty_graph_statistics(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);
    const auto statistics = calculate_statistics(*graph);

    CHECK(statistics.vertex_count == 0);
    CHECK(statistics.edge_count == 0);
    CHECK(statistics.minimum_degree == 0);
    CHECK(statistics.maximum_degree == 0);
    CHECK(statistics.average_degree == 0.0);
    CHECK(statistics.median_degree == 0.0);
}

void verify_breadth_first_search(GraphRepresentation representation) {
    auto graph = make_graph(6, representation);
    graph->add_edge(0, 1);
    graph->add_edge(0, 2);
    graph->add_edge(1, 3);
    graph->add_edge(2, 4);

    const auto search = breadth_first_search(*graph, 0);

    CHECK(search.source == 0);
    CHECK(search.parent[0] == 0);
    CHECK(search.parent[1] == 0);
    CHECK(search.parent[2] == 0);
    CHECK(search.parent[3] == 1);
    CHECK(search.parent[4] == 2);
    CHECK(search.parent[5] == BreadthFirstSearchResult::not_visited);

    CHECK(search.level[0] == 0);
    CHECK(search.level[1] == 1);
    CHECK(search.level[2] == 1);
    CHECK(search.level[3] == 2);
    CHECK(search.level[4] == 2);
    CHECK(search.level[5] == BreadthFirstSearchResult::not_visited);

    CHECK(search.was_visited(4));
    CHECK(!search.was_visited(5));
    CHECK((search.visit_order == std::vector<std::size_t>{0, 1, 2, 3, 4}));

    CHECK(distance(*graph, 0, 4) == std::optional<std::size_t>{2});
    CHECK(distance(*graph, 3, 4) == std::optional<std::size_t>{4});
    CHECK(distance(*graph, 0, 5) == std::nullopt);
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
    CHECK(heap_result.source == result.source);
    CHECK(heap_result.distance == result.distance);
    CHECK(heap_result.parent == result.parent);

    CHECK(result.source == 0);
    CHECK(result.distance[0] == 0.0);
    CHECK(result.distance[1] == 7.0);
    CHECK(result.distance[2] == 9.0);
    CHECK(result.distance[3] == 20.0);
    CHECK(result.distance[4] == 20.0);
    CHECK(result.distance[5] == 11.0);
    CHECK(std::isinf(result.distance[6]));

    CHECK(result.parent[0] == 0);
    CHECK((result.path_to(0) == std::vector<std::size_t>{0}));
    CHECK((result.path_to(3) == std::vector<std::size_t>{0, 2, 3}));
    CHECK((result.path_to(4) == std::vector<std::size_t>{0, 2, 5, 4}));
    CHECK(result.path_to(6).empty());
    CHECK(result.was_reached(5));
    CHECK(!result.was_reached(6));
    CHECK(heap_result.path_to(4) == result.path_to(4));
    CHECK(heap_result.path_to(6).empty());
}

void verify_dijkstra_zero_weight(GraphRepresentation representation) {
    auto graph = make_graph(3, representation);
    graph->add_edge(0, 1, 0.0);
    graph->add_edge(1, 2, 2.0);
    graph->add_edge(0, 2, 5.0);

    const auto result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    CHECK(heap_result.distance == result.distance);
    CHECK(heap_result.parent == result.parent);
    CHECK(result.distance[1] == 0.0);
    CHECK(result.distance[2] == 2.0);
    CHECK((result.path_to(2) == std::vector<std::size_t>{0, 1, 2}));
}

void verify_dijkstra_real_weights(GraphRepresentation representation) {
    auto graph = make_graph(3, representation);
    graph->add_edge(0, 1, 0.1);
    graph->add_edge(1, 2, 0.2);
    graph->add_edge(0, 2, 0.5);

    const auto result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    CHECK(heap_result.distance == result.distance);
    CHECK(heap_result.parent == result.parent);
    CHECK(std::abs(result.distance[2] - 0.3) < 1e-12);
    CHECK((result.path_to(2) == std::vector<std::size_t>{0, 1, 2}));
}

void verify_dijkstra_tie_break(GraphRepresentation representation) {
    auto graph = make_graph(4, representation);
    graph->add_edge(0, 2, 1.0);
    graph->add_edge(0, 1, 1.0);
    graph->add_edge(2, 3, 1.0);
    graph->add_edge(1, 3, 1.0);

    const auto vector_result = dijkstra_vector(*graph, 0);
    const auto heap_result = dijkstra_heap(*graph, 0);
    CHECK(vector_result.distance == heap_result.distance);
    CHECK(vector_result.parent == heap_result.parent);
    CHECK(vector_result.parent[3] == 1);
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
    CHECK(rejected);

    rejected = false;
    try {
        dijkstra_heap(*graph, 0);
    } catch (const std::domain_error&) {
        rejected = true;
    }
    CHECK(rejected);
}

void verify_dijkstra_invalid_vertices(GraphRepresentation representation) {
    auto empty_graph = make_graph(0, representation);
    bool empty_graph_rejected = false;
    try {
        dijkstra_vector(*empty_graph, 0);
    } catch (const std::out_of_range&) {
        empty_graph_rejected = true;
    }
    CHECK(empty_graph_rejected);

    empty_graph_rejected = false;
    try {
        dijkstra_heap(*empty_graph, 0);
    } catch (const std::out_of_range&) {
        empty_graph_rejected = true;
    }
    CHECK(empty_graph_rejected);

    auto graph = make_graph(2, representation);
    graph->add_edge(0, 1, 1.0);

    bool invalid_source_rejected = false;
    try {
        dijkstra_vector(*graph, 2);
    } catch (const std::out_of_range&) {
        invalid_source_rejected = true;
    }
    CHECK(invalid_source_rejected);

    invalid_source_rejected = false;
    try {
        dijkstra_heap(*graph, 2);
    } catch (const std::out_of_range&) {
        invalid_source_rejected = true;
    }
    CHECK(invalid_source_rejected);

    const auto result = dijkstra_vector(*graph, 0);
    bool invalid_target_rejected = false;
    try {
        result.path_to(2);
    } catch (const std::out_of_range&) {
        invalid_target_rejected = true;
    }
    CHECK(invalid_target_rejected);
}

void verify_depth_first_search(GraphRepresentation representation) {
    auto graph = make_graph(6, representation);
    graph->add_edge(0, 1);
    graph->add_edge(0, 2);
    graph->add_edge(1, 3);
    graph->add_edge(2, 4);

    const auto search = depth_first_search(*graph, 0);

    CHECK(search.source == 0);
    CHECK(search.parent[0] == 0);
    CHECK(search.parent[1] == 0);
    CHECK(search.parent[2] == 0);
    CHECK(search.parent[3] == 1);
    CHECK(search.parent[4] == 2);
    CHECK(search.parent[5] == DepthFirstSearchResult::not_visited);

    CHECK(search.level[0] == 0);
    CHECK(search.level[1] == 1);
    CHECK(search.level[2] == 1);
    CHECK(search.level[3] == 2);
    CHECK(search.level[4] == 2);
    CHECK(search.level[5] == DepthFirstSearchResult::not_visited);

    CHECK(search.was_visited(4));
    CHECK(!search.was_visited(5));
    CHECK((search.visit_order == std::vector<std::size_t>{0, 1, 3, 2, 4}));
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

    CHECK(components.size() == 4);
    CHECK(components[0].size() == 4);
    CHECK((components[0].vertices == std::vector<std::size_t>{0, 1, 2, 3}));
    CHECK(components[1].size() == 2);
    CHECK((components[1].vertices == std::vector<std::size_t>{4, 5}));
    CHECK((components[2].vertices == std::vector<std::size_t>{6}));
    CHECK((components[3].vertices == std::vector<std::size_t>{7}));
}

void verify_empty_graph_components(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);
    CHECK(connected_components(*graph).empty());
}

void verify_diameter(GraphRepresentation representation) {
    auto path = make_graph(5, representation);
    path->add_edge(0, 1);
    path->add_edge(1, 2);
    path->add_edge(2, 3);
    path->add_edge(3, 4);

    const auto exact_path = exact_diameter(*path);
    CHECK(exact_path.is_exact);
    CHECK(exact_path.value == 4);
    CHECK(exact_path.first_vertex == 0);
    CHECK(exact_path.second_vertex == 4);

    const auto approximate_path = approximate_diameter(*path);
    CHECK(!approximate_path.is_exact);
    CHECK(approximate_path.value == 4);
    CHECK(approximate_path.first_vertex == 4);
    CHECK(approximate_path.second_vertex == 0);

    auto disconnected = make_graph(7, representation);
    // Componente triangular com diametro 1.
    disconnected->add_edge(0, 1);
    disconnected->add_edge(1, 2);
    disconnected->add_edge(2, 0);
    // Componente em caminho com diametro 2; vertice 6 e isolado.
    disconnected->add_edge(3, 4);
    disconnected->add_edge(4, 5);

    const auto exact_disconnected = exact_diameter(*disconnected);
    CHECK(exact_disconnected.value == 2);
    CHECK(exact_disconnected.first_vertex == 3);
    CHECK(exact_disconnected.second_vertex == 5);

    const auto approximate_disconnected = approximate_diameter(*disconnected);
    CHECK(approximate_disconnected.value == 2);
}

void verify_empty_graph_diameter(GraphRepresentation representation) {
    auto graph = make_graph(0, representation);

    const auto exact = exact_diameter(*graph);
    CHECK(exact.is_exact);
    CHECK(exact.value == 0);
    CHECK(exact.first_vertex == DiameterResult::no_vertex);
    CHECK(exact.second_vertex == DiameterResult::no_vertex);

    const auto approximate = approximate_diameter(*graph);
    CHECK(!approximate.is_exact);
    CHECK(approximate.value == 0);
    CHECK(approximate.first_vertex == DiameterResult::no_vertex);
    CHECK(approximate.second_vertex == DiameterResult::no_vertex);
}

} // namespace

void run_tests() {
    verify_sampling();
    verify_bulk_loading(GraphRepresentation::AdjacencyList);
    verify_bulk_loading(GraphRepresentation::AdjacencyMatrix);
    verify_borrowed_neighbors();
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
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--verify-failure") {
            CHECK(false);
        }
        run_tests();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Teste falhou: " << error.what() << '\n';
        return 1;
    }
}
