#ifndef GRAPH_STUDY_GRAPH_HPP
#define GRAPH_STUDY_GRAPH_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

enum class GraphRepresentation {
    AdjacencyList,
    AdjacencyMatrix
};

struct Edge {
    std::size_t vertex;
    double weight;
};

class Graph {
public:
    explicit Graph(std::size_t vertex_count);
    virtual ~Graph() = default;

    std::size_t vertex_count() const noexcept;
    std::size_t edge_count() const noexcept;
    bool has_negative_weight() const noexcept;

    virtual void add_edge(
        std::size_t u,
        std::size_t v,
        double weight = 1.0
    ) = 0;
    virtual bool has_edge(std::size_t u, std::size_t v) const = 0;
    virtual std::vector<Edge> neighbors(std::size_t vertex) const = 0;

protected:
    void validate_vertex(std::size_t vertex) const;
    void register_weight(double weight) noexcept;
    void register_edge(double weight) noexcept;

private:
    std::size_t vertex_count_;
    std::size_t edge_count_ = 0;
    bool has_negative_weight_ = false;
};

class AdjacencyListGraph final : public Graph {
public:
    explicit AdjacencyListGraph(std::size_t vertex_count);

    void add_edge(
        std::size_t u,
        std::size_t v,
        double weight = 1.0
    ) override;
    bool has_edge(std::size_t u, std::size_t v) const override;
    std::vector<Edge> neighbors(std::size_t vertex) const override;

private:
    std::vector<std::vector<Edge>> adjacency_;
};

class AdjacencyMatrixGraph final : public Graph {
public:
    explicit AdjacencyMatrixGraph(std::size_t vertex_count);

    void add_edge(
        std::size_t u,
        std::size_t v,
        double weight = 1.0
    ) override;
    bool has_edge(std::size_t u, std::size_t v) const override;
    std::vector<Edge> neighbors(std::size_t vertex) const override;

private:
    std::size_t index(std::size_t row, std::size_t column) const noexcept;
    std::vector<double> adjacency_;
};

std::unique_ptr<Graph> make_graph(
    std::size_t vertex_count,
    GraphRepresentation representation
);

std::unique_ptr<Graph> read_graph_from_file(
    const std::string& path,
    GraphRepresentation representation
);

#endif
