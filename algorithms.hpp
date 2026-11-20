#ifndef GRAPH_STUDY_ALGORITHMS_HPP
#define GRAPH_STUDY_ALGORITHMS_HPP

#include "graph.hpp"

#include <cstddef>
#include <functional>
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

// Em grafos direcionados, degree e as estatisticas se referem ao grau de saida.
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

struct ShortestPathResult {
    static constexpr std::size_t no_parent =
        std::numeric_limits<std::size_t>::max();

    std::size_t source = no_parent;
    std::vector<double> distance;
    std::vector<std::size_t> parent;

    bool was_reached(std::size_t vertex) const;
    std::vector<std::size_t> path_to(std::size_t vertex) const;
};

ShortestPathResult dijkstra_vector(
    const Graph& graph,
    std::size_t source
);

ShortestPathResult dijkstra_heap(
    const Graph& graph,
    std::size_t source
);

enum class PathStatus {
    Finite,
    Unreachable,
    NegativeCycle
};

struct BellmanFordResult {
    static constexpr std::size_t no_vertex = std::numeric_limits<std::size_t>::max();

    std::size_t target = no_vertex;
    std::vector<double> distance;
    std::vector<std::size_t> next;
    bool has_negative_cycle = false; // Em qualquer componente do grafo.
    // Contadores da busca ate o destino, sem a deteccao global complementar.
    std::size_t relaxation_passes = 0;
    std::size_t examined_edges = 0;

    PathStatus status(std::size_t source) const;
    // Vazio se inalcançavel; domain_error se afetado por ciclo negativo.
    std::vector<std::size_t> path_from(std::size_t source) const;
};

struct AlgorithmProgress {
    const char* phase;
    std::size_t completed;
    std::size_t total;
    std::size_t examined_edges;
};

// Usado pelos pilotos; sem observador nas rodadas oficiais cronometradas.
using AlgorithmObserver = std::function<void(const AlgorithmProgress&)>;
BellmanFordResult bellman_ford_to_target(
    const Graph& graph, std::size_t target, const AlgorithmObserver& observer = {}
);

struct FloydWarshallResult {
    static constexpr std::size_t no_vertex = std::numeric_limits<std::size_t>::max();

    std::size_t vertex_count = 0;
    std::vector<double> distance; // Indice source * vertex_count + target.
    std::vector<std::size_t> predecessor;
    bool has_negative_cycle = false;

    double distance_between(std::size_t source, std::size_t target) const;
    PathStatus status(std::size_t source, std::size_t target) const;
    std::vector<std::size_t> path(std::size_t source, std::size_t target) const;
    std::vector<std::size_t> predecessors_from(std::size_t source) const;
};

// Distancias, predecessores e indices de ciclos; nao inclui o grafo de entrada.
std::size_t floyd_warshall_memory_bytes(std::size_t vertex_count);
constexpr std::size_t default_floyd_warshall_memory_budget = std::size_t{256} * 1024 * 1024;
FloydWarshallResult floyd_warshall(
    const Graph& graph,
    std::size_t memory_budget_bytes = default_floyd_warshall_memory_budget,
    const AlgorithmObserver& observer = {}
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

struct ConnectedComponent {
    std::vector<std::size_t> vertices;

    std::size_t size() const noexcept;
};

std::vector<ConnectedComponent> connected_components(const Graph& graph);

struct DiameterResult {
    static constexpr std::size_t no_vertex =
        std::numeric_limits<std::size_t>::max();

    std::size_t value = 0;
    std::size_t first_vertex = no_vertex;
    std::size_t second_vertex = no_vertex;
    bool is_exact = false;
};

// Em grafos desconectados, considera a maior distancia finita encontrada
// dentro de qualquer componente conexa.
DiameterResult exact_diameter(const Graph& graph);
DiameterResult approximate_diameter(const Graph& graph);

#endif
