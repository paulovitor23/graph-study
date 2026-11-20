#include "algorithms.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>

namespace {

constexpr double infinity = std::numeric_limits<double>::infinity();
using IncomingEdges = std::vector<std::vector<Edge>>;

double add_cost(double left, double right) {
    const double sum = left + right;
    if (!std::isfinite(sum)) {
        throw std::overflow_error("custo do caminho excede o intervalo finito de double");
    }
    return sum;
}

PathStatus classify(double distance) {
    if (distance == infinity) return PathStatus::Unreachable;
    if (distance == -infinity) return PathStatus::NegativeCycle;
    if (!std::isfinite(distance)) throw std::logic_error("distancia invalida no resultado");
    return PathStatus::Finite;
}

IncomingEdges incoming_edges(const Graph& graph) {
    IncomingEdges incoming(graph.vertex_count());
    for (std::size_t u = 0; u < graph.vertex_count(); ++u) {
        for (const auto& edge : graph.neighbor_view(u)) {
            incoming[edge.vertex].push_back({u, edge.weight});
        }
    }
    return incoming;
}

std::size_t relax_frontier(
    const IncomingEdges& incoming,
    const std::vector<std::size_t>& active,
    std::vector<double>& distance,
    std::vector<std::size_t>* successor,
    std::vector<std::size_t>& changed,
    std::vector<unsigned char>& scheduled
) {
    changed.clear();
    std::size_t examined = 0;
    for (const auto w : active) {
        for (const auto& edge : incoming[w]) {
            ++examined;
            const auto candidate = add_cost(distance[w], edge.weight);
            const auto v = edge.vertex;
            if (candidate < distance[v]) {
                distance[v] = candidate;
                if (successor) (*successor)[v] = w;
                if (!scheduled[v]) {
                    changed.push_back(v);
                    scheduled[v] = 1;
                }
            }
        }
    }
    for (const auto v : changed) scheduled[v] = 0;
    return examined;
}

bool global_negative_cycle(const IncomingEdges& incoming, const AlgorithmObserver& observer) {
    // Todos os vertices partem de zero: equivale a uma superorigem virtual.
    std::vector<double> distance(incoming.size(), 0.0);
    std::vector<std::size_t> active(incoming.size()), changed;
    std::vector<unsigned char> scheduled(incoming.size(), 0);
    std::iota(active.begin(), active.end(), std::size_t{0});
    if (observer) observer({"global", 0, incoming.size(), 0});
    std::size_t examined = 0;
    for (std::size_t pass = 0; pass < incoming.size(); ++pass) {
        examined += relax_frontier(incoming, active, distance, nullptr, changed, scheduled);
        if (observer) observer({"global", pass + 1, incoming.size(), examined});
        if (changed.empty()) return false;
        active.swap(changed);
    }
    return !active.empty();
}

std::size_t matrix_index(std::size_t n, std::size_t source, std::size_t target) {
    if (source >= n || target >= n) {
        throw std::out_of_range("vertice fora do resultado de Floyd-Warshall");
    }
    return source * n + target;
}

} // namespace

PathStatus BellmanFordResult::status(std::size_t source) const {
    return classify(distance.at(source));
}

std::vector<std::size_t> BellmanFordResult::path_from(std::size_t source) const {
    const auto state = status(source);
    if (state == PathStatus::Unreachable) return {};
    if (state == PathStatus::NegativeCycle) {
        throw std::domain_error("caminho afetado por ciclo negativo");
    }
    std::vector<std::size_t> path;
    auto current = source;
    while (current != target) {
        if (path.size() >= distance.size() || current >= next.size() ||
            next[current] >= distance.size()) {
            throw std::logic_error("arvore de Bellman-Ford inconsistente");
        }
        path.push_back(current);
        current = next[current];
    }
    path.push_back(target);
    return path;
}

BellmanFordResult bellman_ford_to_target(
    const Graph& graph, std::size_t target, const AlgorithmObserver& observer
) {
    const auto n = graph.vertex_count();
    if (target >= n) throw std::out_of_range("destino fora do intervalo do grafo");
    const auto incoming = incoming_edges(graph);
    BellmanFordResult result;
    result.target = target;
    result.distance.assign(n, infinity);
    result.next.assign(n, BellmanFordResult::no_vertex);
    result.distance[target] = 0.0;
    result.next[target] = target;
    std::vector<std::size_t> active{target}, changed;
    std::vector<unsigned char> scheduled(n, 0);
    if (observer) observer({"target", 0, n - 1, 0});
    for (std::size_t pass = 0; pass < n - 1 && !active.empty(); ++pass) {
        ++result.relaxation_passes;
        result.examined_edges += relax_frontier(
            incoming, active, result.distance, &result.next, changed, scheduled
        );
        if (observer) observer({"target", pass + 1, n - 1, result.examined_edges});
        active.swap(changed);
    }

    // A verificacao adicional usa distancias fixas e nao altera a arvore finita.
    std::vector<std::size_t> affected;
    for (const auto w : active) {
        for (const auto& edge : incoming[w]) {
            if (add_cost(result.distance[w], edge.weight) < result.distance[edge.vertex] &&
                !scheduled[edge.vertex]) {
                affected.push_back(edge.vertex);
                scheduled[edge.vertex] = 1;
            }
        }
    }
    for (std::size_t i = 0; i < affected.size(); ++i) {
        for (const auto& edge : incoming[affected[i]]) {
            if (!scheduled[edge.vertex]) {
                affected.push_back(edge.vertex);
                scheduled[edge.vertex] = 1;
            }
        }
    }
    for (const auto v : affected) {
        result.distance[v] = -infinity;
        result.next[v] = BellmanFordResult::no_vertex;
    }
    result.has_negative_cycle = !affected.empty() ||
        (graph.has_negative_weight() && global_negative_cycle(incoming, observer));
    return result;
}

double FloydWarshallResult::distance_between(std::size_t source, std::size_t target) const {
    return distance.at(matrix_index(vertex_count, source, target));
}

PathStatus FloydWarshallResult::status(std::size_t source, std::size_t target) const {
    return classify(distance_between(source, target));
}

std::vector<std::size_t> FloydWarshallResult::path(std::size_t source, std::size_t target) const {
    const auto state = status(source, target);
    if (state == PathStatus::Unreachable) return {};
    if (state == PathStatus::NegativeCycle) {
        throw std::domain_error("caminho afetado por ciclo negativo");
    }
    std::vector<std::size_t> path;
    auto current = target;
    while (current != source) {
        if (path.size() >= vertex_count || current >= vertex_count) {
            throw std::logic_error("arvore de Floyd-Warshall inconsistente");
        }
        path.push_back(current);
        current = predecessor.at(source * vertex_count + current);
    }
    path.push_back(source);
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::size_t> FloydWarshallResult::predecessors_from(std::size_t source) const {
    const auto offset = matrix_index(vertex_count, source, 0);
    if (predecessor.size() != distance.size() || offset > predecessor.size() ||
        vertex_count > predecessor.size() - offset) {
        throw std::logic_error("arvore de Floyd-Warshall inconsistente");
    }
    return {predecessor.begin() + offset, predecessor.begin() + offset + vertex_count};
}

std::size_t floyd_warshall_memory_bytes(std::size_t n) {
    constexpr auto maximum = std::numeric_limits<std::size_t>::max();
    constexpr auto cell_bytes = sizeof(double) + sizeof(std::size_t);
    if (n != 0 && n > maximum / n) {
        throw std::length_error("Floyd-Warshall inviavel: V ao quadrado excede size_t");
    }
    const auto cells = n * n;
    if (n > maximum / sizeof(std::size_t) ||
        cells > (maximum - n * sizeof(std::size_t)) / cell_bytes ||
        cells > std::vector<double>().max_size() ||
        cells > std::vector<std::size_t>().max_size()) {
        throw std::length_error("Floyd-Warshall inviavel: memoria excede o intervalo de enderecamento");
    }
    return cells * cell_bytes + n * sizeof(std::size_t);
}

FloydWarshallResult floyd_warshall(
    const Graph& graph, std::size_t memory_budget_bytes, const AlgorithmObserver& observer
) {
    const auto n = graph.vertex_count();
    const auto required = floyd_warshall_memory_bytes(n);
    if (required > memory_budget_bytes) {
        throw std::length_error("Floyd-Warshall inviavel: " + std::to_string(required) +
            " bytes excedem o orcamento de " + std::to_string(memory_budget_bytes) + " bytes");
    }
    FloydWarshallResult result;
    result.vertex_count = n;
    result.distance.assign(n * n, infinity);
    result.predecessor.assign(n * n, FloydWarshallResult::no_vertex);
    std::vector<std::size_t> negative_vertices;
    negative_vertices.reserve(n);
    auto& d = result.distance;
    auto& pred = result.predecessor;
    for (std::size_t u = 0; u < n; ++u) {
        d[u * n + u] = 0.0;
        pred[u * n + u] = u;
        for (const auto& edge : graph.neighbor_view(u)) {
            if (edge.weight < d[u * n + edge.vertex]) {
                d[u * n + edge.vertex] = edge.weight;
                pred[u * n + edge.vertex] = u;
            }
        }
    }
    if (observer) observer({"floyd", 0, n, 0});
    for (std::size_t k = 0; k < n; ++k) {
        const bool negative_through_k = d[k * n + k] < 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (d[i * n + k] == infinity) continue;
            for (std::size_t j = 0; j < n; ++j) {
                if (d[k * n + j] == infinity) continue;
                // Um ciclo ja conhecido torna esses pares ilimitados; nao somar -infinito.
                const auto candidate = negative_through_k || d[i * n + k] == -infinity ||
                    d[k * n + j] == -infinity ? -infinity : add_cost(d[i * n + k], d[k * n + j]);
                if (candidate < d[i * n + j]) {
                    d[i * n + j] = candidate;
                    pred[i * n + j] = candidate == -infinity
                        ? FloydWarshallResult::no_vertex : pred[k * n + j];
                }
            }
        }
        if (observer) observer({"floyd", k + 1, n, 0});
    }
    for (std::size_t v = 0; v < n; ++v) {
        if (d[v * n + v] < 0) negative_vertices.push_back(v);
    }
    result.has_negative_cycle = !negative_vertices.empty();
    for (const auto k : negative_vertices) {
        for (std::size_t i = 0; i < n; ++i) {
            if (d[i * n + k] == infinity) continue;
            for (std::size_t j = 0; j < n; ++j) {
                if (d[k * n + j] != infinity) {
                    d[i * n + j] = -infinity;
                    pred[i * n + j] = FloydWarshallResult::no_vertex;
                }
            }
        }
    }
    return result;
}
