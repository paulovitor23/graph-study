#include "graph.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {

std::size_t matrix_size(std::size_t vertex_count) {
    if (vertex_count != 0 &&
        vertex_count > std::numeric_limits<std::size_t>::max() / vertex_count) {
        throw std::length_error("grafo grande demais para a matriz de adjacencia");
    }
    return vertex_count * vertex_count;
}

void validate_weight(double weight) {
    if (!std::isfinite(weight)) {
        throw std::invalid_argument("peso da aresta deve ser finito");
    }
}

} // namespace

Graph::Graph(std::size_t vertex_count) : vertex_count_(vertex_count) {}

std::size_t Graph::vertex_count() const noexcept {
    return vertex_count_;
}

std::size_t Graph::edge_count() const noexcept {
    return edge_count_;
}

bool Graph::has_negative_weight() const noexcept {
    return has_negative_weight_;
}

NeighborRange Graph::neighbor_view(std::size_t vertex) const {
    return NeighborRange(neighbors(vertex));
}

void Graph::validate_vertex(std::size_t vertex) const {
    if (vertex >= vertex_count_) {
        throw std::out_of_range("vertice fora do intervalo do grafo");
    }
}

void Graph::register_weight(double weight) noexcept {
    has_negative_weight_ = has_negative_weight_ || weight < 0.0;
}

void Graph::register_edge(double weight) noexcept {
    ++edge_count_;
    register_weight(weight);
}

AdjacencyListGraph::AdjacencyListGraph(std::size_t vertex_count)
    : Graph(vertex_count), adjacency_(vertex_count) {}

void AdjacencyListGraph::add_edge(
    std::size_t u,
    std::size_t v,
    double weight
) {
    validate_vertex(u);
    validate_vertex(v);
    validate_weight(weight);

    const auto existing = std::find_if(
        adjacency_[u].begin(),
        adjacency_[u].end(),
        [v](const Edge& edge) { return edge.vertex == v; }
    );
    if (existing != adjacency_[u].end()) {
        if (weight < existing->weight) {
            existing->weight = weight;
            if (u != v) {
                const auto reverse = std::find_if(
                    adjacency_[v].begin(),
                    adjacency_[v].end(),
                    [u](const Edge& edge) { return edge.vertex == u; }
                );
                reverse->weight = weight;
            }
            register_weight(weight);
        }
        return;
    }

    adjacency_[u].push_back({v, weight});
    if (u != v) {
        adjacency_[v].push_back({u, weight});
    }
    register_edge(weight);
}

bool AdjacencyListGraph::has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u);
    validate_vertex(v);
    const auto& adjacent = adjacency_[u];
    return std::any_of(
        adjacent.begin(),
        adjacent.end(),
        [v](const Edge& edge) { return edge.vertex == v; }
    );
}

std::vector<Edge> AdjacencyListGraph::neighbors(std::size_t vertex) const {
    validate_vertex(vertex);
    return adjacency_[vertex];
}

NeighborRange AdjacencyListGraph::neighbor_view(std::size_t vertex) const {
    validate_vertex(vertex);
    return NeighborRange(&adjacency_[vertex]);
}

void AdjacencyListGraph::append_edge_for_loading(
    std::size_t u, std::size_t v, double weight
) {
    adjacency_[u].push_back({v, weight});
    if (u != v) {
        adjacency_[v].push_back({u, weight});
    }
}

void AdjacencyListGraph::finish_loading() {
    // Ordena indices, preservando a ordem original dos vizinhos (BFS/DFS).
    // O espaco temporario e proporcional ao maior grau, nao ao grafo todo.
    std::vector<std::size_t> order;
    constexpr auto removed = std::numeric_limits<std::size_t>::max();
    for (std::size_t u = 0; u < adjacency_.size(); ++u) {
        auto& edges = adjacency_[u];
        order.resize(edges.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::sort(order.begin(), order.end(), [&edges](auto a, auto b) {
            return edges[a].vertex < edges[b].vertex ||
                   (edges[a].vertex == edges[b].vertex && a < b);
        });
        for (std::size_t first = 0; first < order.size();) {
            const auto keep = order[first];
            const auto vertex = edges[keep].vertex;
            auto next = first + 1;
            while (next < order.size() && edges[order[next]].vertex == vertex) {
                edges[keep].weight = std::min(edges[keep].weight, edges[order[next]].weight);
                edges[order[next]].vertex = removed;
                ++next;
            }
            first = next;
        }
        edges.erase(std::remove_if(edges.begin(), edges.end(), [](const Edge& edge) {
            return edge.vertex == removed;
        }), edges.end());
        for (const auto& edge : edges) {
            if (u <= edge.vertex) {
                register_edge(edge.weight);
            }
        }
    }
}

AdjacencyMatrixGraph::AdjacencyMatrixGraph(std::size_t vertex_count)
    : Graph(vertex_count),
      adjacency_(
          matrix_size(vertex_count),
          std::numeric_limits<double>::quiet_NaN()
      ) {}

std::size_t AdjacencyMatrixGraph::index(
    std::size_t row,
    std::size_t column
) const noexcept {
    return row * vertex_count() + column;
}

void AdjacencyMatrixGraph::add_edge(
    std::size_t u,
    std::size_t v,
    double weight
) {
    validate_vertex(u);
    validate_vertex(v);
    validate_weight(weight);

    const double existing = adjacency_[index(u, v)];
    if (!std::isnan(existing)) {
        if (weight < existing) {
            adjacency_[index(u, v)] = weight;
            adjacency_[index(v, u)] = weight;
            register_weight(weight);
        }
        return;
    }

    adjacency_[index(u, v)] = weight;
    adjacency_[index(v, u)] = weight;
    register_edge(weight);
}

bool AdjacencyMatrixGraph::has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u);
    validate_vertex(v);
    return !std::isnan(adjacency_[index(u, v)]);
}

std::vector<Edge> AdjacencyMatrixGraph::neighbors(
    std::size_t vertex
) const {
    validate_vertex(vertex);

    std::vector<Edge> result;
    for (std::size_t candidate = 0; candidate < vertex_count(); ++candidate) {
        const double weight = adjacency_[index(vertex, candidate)];
        if (!std::isnan(weight)) {
            result.push_back({candidate, weight});
        }
    }
    return result;
}

std::unique_ptr<Graph> make_graph(
    std::size_t vertex_count,
    GraphRepresentation representation
) {
    if (representation == GraphRepresentation::AdjacencyList) {
        return std::make_unique<AdjacencyListGraph>(vertex_count);
    }
    return std::make_unique<AdjacencyMatrixGraph>(vertex_count);
}

std::unique_ptr<Graph> read_graph_from_file(
    const std::string& path,
    GraphRepresentation representation
) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("nao foi possivel abrir o arquivo: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("numero de vertices ausente ou invalido");
    }
    std::istringstream header(line);
    std::size_t vertex_count = 0;
    std::string extra;
    if (!(header >> vertex_count) || (header >> extra)) {
        throw std::runtime_error("numero de vertices ausente ou invalido");
    }

    auto graph = make_graph(vertex_count, representation);
    auto* list = dynamic_cast<AdjacencyListGraph*>(graph.get());
    while (std::getline(input, line)) {
        if (line.find_first_not_of(" \t\r") == std::string::npos) {
            continue;
        }

        std::istringstream edge_input(line);
        std::size_t file_u = 0;
        std::size_t file_v = 0;
        double weight = 1.0;
        if (!(edge_input >> file_u >> file_v)) {
            throw std::runtime_error("aresta incompleta ou invalida no arquivo");
        }
        edge_input >> std::ws;
        if (!edge_input.eof()) {
            if (!(edge_input >> weight)) {
                throw std::runtime_error("peso da aresta invalido no arquivo");
            }
            edge_input >> std::ws;
            if (!edge_input.eof()) {
                throw std::runtime_error("linha de aresta possui colunas extras");
            }
        }
        validate_weight(weight);

        // O enunciado numera os vertices a partir de 1; internamente usamos 0.
        if (file_u == 0 || file_v == 0 ||
            file_u > vertex_count || file_v > vertex_count) {
            throw std::runtime_error("aresta contem vertice fora do intervalo");
        }
        if (list) {
            list->append_edge_for_loading(file_u - 1, file_v - 1, weight);
        } else {
            graph->add_edge(file_u - 1, file_v - 1, weight);
        }
    }

    if (input.bad()) {
        throw std::runtime_error("erro de leitura no arquivo: " + path);
    }
    if (list) {
        list->finish_loading();
    }

    return graph;
}
