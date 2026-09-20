#include "graph.hpp"

#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {

std::size_t matrix_size(std::size_t vertex_count) {
    if (vertex_count != 0 &&
        vertex_count > std::numeric_limits<std::size_t>::max() / vertex_count) {
        throw std::length_error("grafo grande demais para a matriz de adjacencia");
    }
    return vertex_count * vertex_count;
}

} // namespace

Graph::Graph(std::size_t vertex_count) : vertex_count_(vertex_count) {}

std::size_t Graph::vertex_count() const noexcept {
    return vertex_count_;
}

std::size_t Graph::edge_count() const noexcept {
    return edge_count_;
}

void Graph::validate_vertex(std::size_t vertex) const {
    if (vertex >= vertex_count_) {
        throw std::out_of_range("vertice fora do intervalo do grafo");
    }
}

void Graph::register_edge() noexcept {
    ++edge_count_;
}

AdjacencyListGraph::AdjacencyListGraph(std::size_t vertex_count)
    : Graph(vertex_count), adjacency_(vertex_count) {}

void AdjacencyListGraph::add_edge(std::size_t u, std::size_t v) {
    validate_vertex(u);
    validate_vertex(v);

    if (has_edge(u, v)) {
        return;
    }

    adjacency_[u].push_back(v);
    if (u != v) {
        adjacency_[v].push_back(u);
    }
    register_edge();
}

bool AdjacencyListGraph::has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u);
    validate_vertex(v);
    const auto& adjacent = adjacency_[u];
    return std::find(adjacent.begin(), adjacent.end(), v) != adjacent.end();
}

std::vector<std::size_t> AdjacencyListGraph::neighbors(std::size_t vertex) const {
    validate_vertex(vertex);
    return adjacency_[vertex];
}

AdjacencyMatrixGraph::AdjacencyMatrixGraph(std::size_t vertex_count)
    : Graph(vertex_count),
      adjacency_(matrix_size(vertex_count), static_cast<unsigned char>(0)) {}

std::size_t AdjacencyMatrixGraph::index(
    std::size_t row,
    std::size_t column
) const noexcept {
    return row * vertex_count() + column;
}

void AdjacencyMatrixGraph::add_edge(std::size_t u, std::size_t v) {
    validate_vertex(u);
    validate_vertex(v);

    if (adjacency_[index(u, v)] != 0) {
        return;
    }

    adjacency_[index(u, v)] = 1;
    adjacency_[index(v, u)] = 1;
    register_edge();
}

bool AdjacencyMatrixGraph::has_edge(std::size_t u, std::size_t v) const {
    validate_vertex(u);
    validate_vertex(v);
    return adjacency_[index(u, v)] != 0;
}

std::vector<std::size_t> AdjacencyMatrixGraph::neighbors(
    std::size_t vertex
) const {
    validate_vertex(vertex);

    std::vector<std::size_t> result;
    for (std::size_t candidate = 0; candidate < vertex_count(); ++candidate) {
        if (adjacency_[index(vertex, candidate)] != 0) {
            result.push_back(candidate);
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

    std::size_t vertex_count = 0;
    if (!(input >> vertex_count)) {
        throw std::runtime_error("numero de vertices ausente ou invalido");
    }

    auto graph = make_graph(vertex_count, representation);
    std::size_t file_u = 0;
    std::size_t file_v = 0;

    while (true) {
        if (!(input >> file_u)) {
            if (input.eof()) {
                break;
            }
            throw std::runtime_error("linha de aresta invalida no arquivo");
        }
        if (!(input >> file_v)) {
            throw std::runtime_error("aresta incompleta ou invalida no arquivo");
        }

        // O enunciado numera os vertices a partir de 1; internamente usamos 0.
        if (file_u == 0 || file_v == 0 ||
            file_u > vertex_count || file_v > vertex_count) {
            throw std::runtime_error("aresta contem vertice fora do intervalo");
        }
        graph->add_edge(file_u - 1, file_v - 1);
    }

    return graph;
}
