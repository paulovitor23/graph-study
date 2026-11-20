#include "algorithms.hpp"
#include "check.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <tuple>

namespace {

constexpr auto directed = GraphDirection::Directed;
constexpr double infinity = std::numeric_limits<double>::infinity();
std::size_t verified_pairs = 0;

template <typename Exception, typename Function>
void expect_throw(Function function) {
    bool thrown = false;
    try { function(); }
    catch (const Exception&) { thrown = true; }
    CHECK(thrown);
}

double weight(const Graph& graph, std::size_t u, std::size_t v) {
    for (const auto& edge : graph.neighbor_view(u)) {
        if (edge.vertex == v) return edge.weight;
    }
    throw std::runtime_error("arco ausente no caminho");
}

void check_distance(double actual, double expected) {
    if (std::isinf(expected)) CHECK(actual == expected);
    else CHECK(std::abs(actual - expected) <= 1e-10 * std::max(1.0, std::abs(expected)));
}

void check_path(const Graph& graph, const std::vector<std::size_t>& path,
                std::size_t source, std::size_t target, double distance) {
    CHECK(!path.empty());
    CHECK(path.front() == source);
    CHECK(path.back() == target);
    CHECK(path.size() <= graph.vertex_count());
    double total = 0;
    for (std::size_t i = 1; i < path.size(); ++i) total += weight(graph, path[i - 1], path[i]);
    check_distance(total, distance);
}

void representation_and_loading() {
    const char* path = "directed_test_input.txt";
    {
        std::ofstream output(path);
        output << "6\n1 4 8\n1 2 5\n4 1 2\n1 4 9\n1 1 4\n1 1 0\n2 1 3\n5 6 -1\n";
    }
    auto graph = read_graph_from_file(path, GraphRepresentation::AdjacencyList, directed);
    auto old = read_graph_from_file(path, GraphRepresentation::AdjacencyList);
    std::remove(path);
    CHECK(graph->is_directed());
    CHECK(!old->is_directed());
    CHECK(graph->edge_count() == 6);
    CHECK(old->edge_count() == 4);
    CHECK(graph->has_negative_weight());
    CHECK(!graph->has_edge(5, 4));
    CHECK(weight(*graph, 0, 3) == 8);
    CHECK(weight(*graph, 3, 0) == 2);
    const auto view = graph->neighbor_view(0);
    CHECK(view.size() == 3);
    CHECK(view[0].vertex == 3 && view[1].vertex == 1 && view[2].vertex == 0);
    CHECK(weight(*graph, 0, 0) == 0);
    graph->add_edge(3, 0, 1);
    CHECK(weight(*graph, 0, 3) == 8);
    CHECK(weight(*graph, 3, 0) == 1);
    CHECK(graph->edge_count() == 6);
    CHECK(degree(*graph, 4) == 1 && degree(*graph, 5) == 0);
    CHECK(calculate_statistics(*graph).average_degree == 1);
    expect_throw<std::domain_error>([&] { connected_components(*graph); });
    expect_throw<std::domain_error>([&] { exact_diameter(*graph); });
    expect_throw<std::domain_error>([&] { approximate_diameter(*graph); });
    expect_throw<std::invalid_argument>([] { AdjacencyMatrixGraph g(25000, directed); });
    expect_throw<std::invalid_argument>([] {
        make_graph(25000, GraphRepresentation::AdjacencyMatrix, directed);
    });
    expect_throw<std::invalid_argument>([] { AdjacencyListGraph g(1, static_cast<GraphDirection>(7)); });
    expect_throw<std::invalid_argument>([] { make_graph(1, static_cast<GraphRepresentation>(7)); });

    AdjacencyListGraph inserted(6, directed);
    for (const auto& [u, v, w] : std::vector<std::tuple<std::size_t, std::size_t, double>>{
        {0,3,8}, {0,1,5}, {3,0,2}, {0,3,9}, {0,0,4}, {0,0,0}, {1,0,3}, {4,5,-1}
    }) inserted.add_edge(u, v, w);
    inserted.add_edge(3, 0, 1);
    for (std::size_t u = 0; u < 6; ++u) {
        const auto actual = graph->neighbor_view(u);
        const auto expected = inserted.neighbor_view(u);
        CHECK(actual.size() == expected.size());
        for (std::size_t i = 0; i < actual.size(); ++i) {
            CHECK(actual[i].vertex == expected[i].vertex && actual[i].weight == expected[i].weight);
        }
    }
    { std::ofstream output(path); output << "3\n3 1 -2\n"; }
    const auto descending = read_graph_from_file(path, GraphRepresentation::AdjacencyList, directed);
    std::remove(path);
    CHECK(descending->edge_count() == 1 && descending->has_negative_weight());
    CHECK(descending->has_edge(2, 0) && !descending->has_edge(0, 2));
}

void directed_searches() {
    auto graph = make_graph(6, GraphRepresentation::AdjacencyList, directed);
    for (const auto& [u, v, w] : std::vector<std::tuple<std::size_t, std::size_t, double>>{
        {0,2,2}, {0,1,1}, {2,3,0}, {1,3,1}, {3,4,2}, {4,2,0}
    }) graph->add_edge(u, v, w);
    const auto bfs = breadth_first_search(*graph, 0);
    const auto dfs = depth_first_search(*graph, 0);
    CHECK((bfs.visit_order == std::vector<std::size_t>{0,2,1,3,4}));
    CHECK((dfs.visit_order == std::vector<std::size_t>{0,2,3,4,1}));
    CHECK(bfs.parent[3] == 2 && bfs.level[4] == 3);
    CHECK(!breadth_first_search(*graph, 4).was_visited(0));
    CHECK(!depth_first_search(*graph, 4).was_visited(0));
    CHECK(!distance(*graph, 4, 0));
    CHECK(distance(*graph, 0, 4) == 3);
    const auto linear = dijkstra_vector(*graph, 0);
    const auto heap = dijkstra_heap(*graph, 0);
    CHECK(linear.distance == heap.distance);
    CHECK(linear.parent == heap.parent);
    CHECK(heap.distance[4] == 4 && !heap.was_reached(5));
    CHECK(!dijkstra_heap(*graph, 4).was_reached(0));
    graph->add_edge(5, 5, -1);
    expect_throw<std::domain_error>([&] { dijkstra_heap(*graph, 0); });
    expect_throw<std::domain_error>([&] { dijkstra_vector(*graph, 0); });
}

void known_paths_and_optimizations() {
    AdjacencyListGraph graph(6, directed);
    graph.add_edge(0, 1, 2); graph.add_edge(1, 3, -5);
    graph.add_edge(0, 2, 1); graph.add_edge(2, 3, 4); graph.add_edge(3, 4, 7);
    const auto bf = bellman_ford_to_target(graph, 3);
    CHECK((bf.distance == std::vector<double>{-3, -5, 4, 0, infinity, infinity}));
    CHECK((bf.path_from(0) == std::vector<std::size_t>{0,1,3}));
    CHECK((bf.path_from(3) == std::vector<std::size_t>{3}));
    CHECK(bf.path_from(4).empty());
    CHECK(!bf.has_negative_cycle);
    CHECK(bf.relaxation_passes == 3 && bf.examined_edges == 4);
    CHECK(bf.status(4) == PathStatus::Unreachable);
    const auto fw = floyd_warshall(graph);
    CHECK(!fw.has_negative_cycle);
    CHECK(fw.distance_between(0, 3) == -3);
    CHECK(fw.distance_between(0, 4) == 4);
    CHECK(fw.path(4, 0).empty());
    check_path(graph, fw.path(0, 4), 0, 4, 4);
    const auto tree = fw.predecessors_from(0);
    CHECK(tree[0] == 0 && tree[1] == 0 && tree[3] == 1 && tree[4] == 3);
    CHECK(tree[5] == FloydWarshallResult::no_vertex);
    expect_throw<std::out_of_range>([&] { bellman_ford_to_target(graph, 6); });
    expect_throw<std::out_of_range>([&] { bf.status(6); });
    expect_throw<std::out_of_range>([&] { bf.path_from(6); });
    expect_throw<std::out_of_range>([&] { fw.status(6, 0); });
    expect_throw<std::out_of_range>([&] { fw.distance_between(0, 6); });
    expect_throw<std::out_of_range>([&] { fw.path(0, 6); });
    expect_throw<std::out_of_range>([&] { fw.predecessors_from(6); });

    AdjacencyListGraph isolated(2000, directed);
    isolated.add_edge(0, 1, 1);
    const auto stopped = bellman_ford_to_target(isolated, 1999);
    CHECK(stopped.relaxation_passes == 1 && stopped.examined_edges == 0);
    CHECK(stopped.distance[1999] == 0 && stopped.status(0) == PathStatus::Unreachable);
    AdjacencyListGraph real(3, directed);
    real.add_edge(0, 1, .1); real.add_edge(1, 2, .2); real.add_edge(0, 2, .8);
    check_distance(bellman_ford_to_target(real, 2).distance[0], .3);
    check_distance(floyd_warshall(real).distance_between(0, 2), .3);

    AdjacencyListGraph chain(40, directed);
    for (std::size_t u = 1; u < 40; ++u) chain.add_edge(u, u - 1, -1);
    chain.add_edge(0, 39, 40); // Ciclo positivo, caminho minimo usa V-1 arestas.
    const auto last_pass = bellman_ford_to_target(chain, 0);
    CHECK(!last_pass.has_negative_cycle && last_pass.distance[39] == -39);
    CHECK(last_pass.relaxation_passes == 39);
    CHECK(last_pass.path_from(39).size() == 40);
    chain.add_edge(0, 39, 38); // A reducao torna o mesmo ciclo negativo.
    const auto negative = bellman_ford_to_target(chain, 0);
    CHECK(negative.has_negative_cycle);
    CHECK(std::all_of(negative.distance.begin(), negative.distance.end(),
                      [](double value) { return value == -infinity; }));
}

void negative_cycles() {
    AdjacencyListGraph graph(8, directed);
    graph.add_edge(0, 1, 1); graph.add_edge(1, 2, -3); graph.add_edge(2, 1, 1);
    graph.add_edge(2, 3, 4); graph.add_edge(3, 4, 1);
    graph.add_edge(5, 6, -1); graph.add_edge(6, 5, 0);
    const auto bf = bellman_ford_to_target(graph, 4);
    CHECK(bf.has_negative_cycle);
    for (const auto u : {0U, 1U, 2U}) {
        CHECK(bf.status(u) == PathStatus::NegativeCycle);
        CHECK(bf.distance[u] == -infinity && bf.next[u] == BellmanFordResult::no_vertex);
        expect_throw<std::domain_error>([&] { bf.path_from(u); });
    }
    CHECK(bf.distance[3] == 1 && bf.distance[4] == 0);
    CHECK(bf.status(5) == PathStatus::Unreachable);
    const auto isolated = bellman_ford_to_target(graph, 7);
    CHECK(isolated.has_negative_cycle);
    CHECK(isolated.status(7) == PathStatus::Finite && isolated.distance[7] == 0);
    CHECK(isolated.status(0) == PathStatus::Unreachable);
    const auto fw = floyd_warshall(graph);
    CHECK(fw.has_negative_cycle);
    CHECK(fw.status(0, 4) == PathStatus::NegativeCycle);
    CHECK(fw.status(5, 6) == PathStatus::NegativeCycle);
    CHECK(fw.distance_between(3, 4) == 1 && fw.distance_between(7, 7) == 0);
    CHECK(fw.status(4, 0) == PathStatus::Unreachable);
    expect_throw<std::domain_error>([&] { fw.path(0, 4); });
    CHECK(fw.predecessors_from(0)[4] == FloydWarshallResult::no_vertex);

    AdjacencyListGraph zero(4, directed);
    zero.add_edge(0, 1, -1); zero.add_edge(1, 0, 1);
    zero.add_edge(1, 2, 2); zero.add_edge(2, 3, 0); zero.add_edge(3, 2, 0);
    const auto no_cycle = bellman_ford_to_target(zero, 3);
    CHECK(!no_cycle.has_negative_cycle);
    check_path(zero, no_cycle.path_from(0), 0, 3, 1);
    CHECK(!floyd_warshall(zero).has_negative_cycle);

    for (const auto representation : {GraphRepresentation::AdjacencyList, GraphRepresentation::AdjacencyMatrix}) {
        auto undirected = make_graph(3, representation);
        undirected->add_edge(0, 1, -1);
        CHECK(bellman_ford_to_target(*undirected, 2).has_negative_cycle);
        CHECK(bellman_ford_to_target(*undirected, 1).status(0) == PathStatus::NegativeCycle);
        const auto result = floyd_warshall(*undirected);
        CHECK(result.status(0, 1) == PathStatus::NegativeCycle);
        CHECK(result.status(2, 2) == PathStatus::Finite);
    }
    for (const auto loop_weight : {-std::numeric_limits<double>::max(), -1., 0., 2.}) {
        AdjacencyListGraph single(1, directed);
        single.add_edge(0, 0, loop_weight);
        const auto bf_loop = bellman_ford_to_target(single, 0);
        const auto fw_loop = floyd_warshall(single);
        CHECK(bf_loop.has_negative_cycle == (loop_weight < 0));
        CHECK(fw_loop.has_negative_cycle == (loop_weight < 0));
        CHECK(bf_loop.status(0) == fw_loop.status(0, 0));
        if (loop_weight >= 0) CHECK((fw_loop.path(0, 0) == std::vector<std::size_t>{0}));
    }
    AdjacencyListGraph dense_negative(128, directed);
    for (std::size_t u = 0; u < 128; ++u) {
        for (std::size_t v = 0; v < 128; ++v) {
            if (u != v) dense_negative.add_edge(u, v, -1);
        }
    }
    const auto dense_fw = floyd_warshall(dense_negative);
    CHECK(dense_fw.has_negative_cycle);
    CHECK(std::all_of(dense_fw.distance.begin(), dense_fw.distance.end(),
                      [](double value) { return value == -infinity; }));
}

class ShapeGraph final : public Graph {
public:
    explicit ShapeGraph(std::size_t n) : Graph(n, directed) {}
    mutable std::size_t neighbor_calls = 0;
    void add_edge(std::size_t, std::size_t, double) override { throw std::logic_error("shape only"); }
    bool has_edge(std::size_t, std::size_t) const override { return false; }
    std::vector<Edge> neighbors(std::size_t) const override { ++neighbor_calls; return {}; }
};

void resource_and_numeric_limits() {
    ShapeGraph large(25000);
    expect_throw<std::length_error>([&] { floyd_warshall(large); });
    CHECK(large.neighbor_calls == 0);
    expect_throw<std::length_error>([] { floyd_warshall_memory_bytes(std::numeric_limits<std::size_t>::max()); });
    ShapeGraph small(2);
    const auto required = 4 * (sizeof(double) + sizeof(std::size_t)) + 2 * sizeof(std::size_t);
    CHECK(floyd_warshall_memory_bytes(2) == required);
    expect_throw<std::length_error>([&] { floyd_warshall(small, required - 1); });
    CHECK(small.neighbor_calls == 0);
    CHECK(floyd_warshall(small, required).vertex_count == 2);
    AdjacencyListGraph empty(0, directed);
    const auto fw_empty = floyd_warshall(empty, 0);
    CHECK(fw_empty.distance.empty() && !fw_empty.has_negative_cycle);
    CHECK(floyd_warshall_memory_bytes(0) == 0);
    expect_throw<std::out_of_range>([&] { bellman_ford_to_target(empty, 0); });
    expect_throw<std::out_of_range>([&] { fw_empty.path(0, 0); });
    AdjacencyListGraph overflow(3, directed);
    overflow.add_edge(0, 1, std::numeric_limits<double>::max());
    overflow.add_edge(1, 2, std::numeric_limits<double>::max());
    expect_throw<std::overflow_error>([&] { bellman_ford_to_target(overflow, 2); });
    expect_throw<std::overflow_error>([&] { floyd_warshall(overflow); });
}

void progress_observers() {
    AdjacencyListGraph graph(4, directed);
    graph.add_edge(0, 1, 2); graph.add_edge(1, 3, -1);
    std::vector<AlgorithmProgress> updates;
    const auto observed_bf = bellman_ford_to_target(graph, 3, [&](const auto& update) {
        CHECK(update.completed <= update.total);
        updates.push_back(update);
    });
    CHECK(observed_bf.distance == bellman_ford_to_target(graph, 3).distance);
    CHECK(!updates.empty() && updates.front().completed == 0);
    CHECK(std::any_of(updates.begin(), updates.end(), [](const auto& update) {
        return std::string(update.phase) == "global";
    }));
    updates.clear();
    const auto observed_fw = floyd_warshall(graph, default_floyd_warshall_memory_budget,
        [&](const auto& update) { updates.push_back(update); });
    CHECK(observed_fw.distance == floyd_warshall(graph).distance);
    CHECK(updates.size() == 5 && updates.back().completed == 4);
    expect_throw<std::runtime_error>([&] {
        bellman_ford_to_target(graph, 3, [](const auto&) { throw std::runtime_error("cancel pilot"); });
    });
    expect_throw<std::runtime_error>([&] {
        floyd_warshall(graph, default_floyd_warshall_memory_budget,
                      [](const auto&) { throw std::runtime_error("cancel pilot"); });
    });
    CHECK(bellman_ford_to_target(graph, 3).distance[0] == 1);
}

// Referencia independente: Bellman-Ford por origem, com copia da rodada anterior.
std::vector<double> reference_from_source(const Graph& graph, std::size_t source) {
    const auto n = graph.vertex_count();
    std::vector<double> distance(n, infinity);
    distance[source] = 0;
    for (std::size_t pass = 0; pass < n - 1; ++pass) {
        auto next = distance;
        for (std::size_t u = 0; u < n; ++u) {
            if (distance[u] == infinity) continue;
            for (const auto& edge : graph.neighbor_view(u)) {
                next[edge.vertex] = std::min(next[edge.vertex], distance[u] + edge.weight);
            }
        }
        if (next == distance) break;
        distance.swap(next);
    }
    std::vector<unsigned char> affected(n, 0);
    std::vector<std::size_t> pending;
    for (std::size_t u = 0; u < n; ++u) {
        if (distance[u] == infinity) continue;
        for (const auto& edge : graph.neighbor_view(u)) {
            if (distance[u] + edge.weight < distance[edge.vertex] && !affected[edge.vertex]) {
                affected[edge.vertex] = 1;
                pending.push_back(edge.vertex);
            }
        }
    }
    for (std::size_t i = 0; i < pending.size(); ++i) {
        for (const auto& edge : graph.neighbor_view(pending[i])) {
            if (!affected[edge.vertex]) {
                affected[edge.vertex] = 1;
                pending.push_back(edge.vertex);
            }
        }
    }
    for (std::size_t v = 0; v < n; ++v) if (affected[v]) distance[v] = -infinity;
    return distance;
}

void compare_all_pairs(const Graph& graph) {
    const auto n = graph.vertex_count();
    const auto fw = floyd_warshall(graph);
    std::vector<std::vector<double>> reference;
    bool negative = false;
    for (std::size_t u = 0; u < n; ++u) {
        reference.push_back(reference_from_source(graph, u));
        for (const auto value : reference.back()) negative = negative || value == -infinity;
        if (!graph.has_negative_weight()) {
            const auto linear = dijkstra_vector(graph, u);
            const auto heap = dijkstra_heap(graph, u);
            CHECK(linear.distance == heap.distance);
            CHECK(linear.parent == heap.parent);
            for (std::size_t v = 0; v < n; ++v) check_distance(heap.distance[v], reference.back()[v]);
        }
    }
    CHECK(fw.has_negative_cycle == negative);
    for (std::size_t v = 0; v < n; ++v) {
        const auto bf = bellman_ford_to_target(graph, v);
        CHECK(bf.has_negative_cycle == negative);
        for (std::size_t u = 0; u < n; ++u) {
            ++verified_pairs;
            check_distance(bf.distance[u], reference[u][v]);
            check_distance(fw.distance_between(u, v), reference[u][v]);
            CHECK(bf.status(u) == fw.status(u, v));
            if (std::isfinite(reference[u][v])) {
                check_path(graph, bf.path_from(u), u, v, reference[u][v]);
                check_path(graph, fw.path(u, v), u, v, reference[u][v]);
            } else if (reference[u][v] == infinity) {
                CHECK(bf.path_from(u).empty() && fw.path(u, v).empty());
            }
        }
    }
}

void randomized_integration() {
    std::mt19937 generator(242);
    for (const bool signed_weights : {false, true}) {
        for (std::size_t n = 1; n <= 8; ++n) {
            for (int iteration = 0; iteration < 80; ++iteration) {
                AdjacencyListGraph graph(n, directed);
                for (std::size_t u = 0; u < n; ++u) {
                    for (std::size_t v = 0; v < n; ++v) {
                        if (generator() % 4 == 0) {
                            const double w = static_cast<int>(generator() % 13) - (signed_weights ? 5 : 0);
                            graph.add_edge(u, v, w);
                        }
                    }
                }
                compare_all_pairs(graph);
            }
        }
    }
}

double enumerate_dag(const Graph& graph, std::size_t u, std::size_t target) {
    if (u == target) return 0;
    double best = infinity;
    for (const auto& edge : graph.neighbor_view(u)) {
        const auto suffix = enumerate_dag(graph, edge.vertex, target);
        if (suffix != infinity) best = std::min(best, edge.weight + suffix);
    }
    return best;
}

void dag_enumeration_and_staged_floyd() {
    std::mt19937 generator(531);
    for (std::size_t n = 1; n <= 9; ++n) {
        for (int iteration = 0; iteration < 20; ++iteration) {
            std::vector<std::tuple<std::size_t, std::size_t, double>> edges;
            for (std::size_t u = 0; u < n; ++u) {
                for (std::size_t v = u + 1; v < n; ++v) {
                    if (generator() % 3 != 0) edges.emplace_back(u, v, (static_cast<int>(generator() % 61) - 20) / 4.0);
                }
            }
            std::shuffle(edges.begin(), edges.end(), generator);
            AdjacencyListGraph graph(n, directed);
            for (const auto& [u, v, w] : edges) graph.add_edge(u, v, w);
            auto staged = std::vector<double>(n * n, infinity);
            for (std::size_t u = 0; u < n; ++u) {
                staged[u * n + u] = 0;
                for (const auto& edge : graph.neighbor_view(u)) staged[u * n + edge.vertex] = edge.weight;
            }
            for (std::size_t k = 0; k < n; ++k) {
                auto next = staged;
                for (std::size_t u = 0; u < n; ++u) {
                    for (std::size_t v = 0; v < n; ++v) {
                        if (staged[u * n + k] != infinity && staged[k * n + v] != infinity) {
                            next[u * n + v] = std::min(next[u * n + v], staged[u * n + k] + staged[k * n + v]);
                        }
                    }
                }
                staged.swap(next);
            }
            const auto fw = floyd_warshall(graph);
            CHECK(fw.distance == staged);
            CHECK(!fw.has_negative_cycle);
            for (std::size_t v = 0; v < n; ++v) {
                const auto bf = bellman_ford_to_target(graph, v);
                CHECK(!bf.has_negative_cycle);
                for (std::size_t u = 0; u < n; ++u) {
                    ++verified_pairs;
                    const auto expected = enumerate_dag(graph, u, v);
                    check_distance(bf.distance[u], expected);
                    check_distance(fw.distance_between(u, v), expected);
                }
            }
        }
    }
}

} // namespace

int main() {
    try {
        representation_and_loading();
        directed_searches();
        known_paths_and_optimizations();
        negative_cycles();
        resource_and_numeric_limits();
        progress_observers();
        randomized_integration();
        dag_enumeration_and_staged_floyd();
        std::cout << "Testes direcionados passaram; " << verified_pairs << " pares verificados.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Teste direcionado falhou: " << error.what() << '\n';
        return 1;
    }
}
