#include "algorithms.hpp"

#include <algorithm>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <utility>

namespace {

struct DistanceSearchResult {
    std::vector<std::size_t> distances;
    std::size_t farthest_vertex;
};

DistanceSearchResult breadth_first_distances(
    const Graph& graph,
    std::size_t source
) {
    if (source >= graph.vertex_count()) {
        throw std::out_of_range("vertice inicial fora do intervalo do grafo");
    }

    DistanceSearchResult result;
    result.distances.assign(
        graph.vertex_count(),
        BreadthFirstSearchResult::not_visited
    );
    result.farthest_vertex = source;

    std::queue<std::size_t> pending;
    result.distances[source] = 0;
    pending.push(source);

    while (!pending.empty()) {
        const std::size_t current = pending.front();
        pending.pop();

        for (const Edge& edge : graph.neighbors(current)) {
            const std::size_t adjacent = edge.vertex;
            if (result.distances[adjacent] !=
                BreadthFirstSearchResult::not_visited) {
                continue;
            }

            result.distances[adjacent] = result.distances[current] + 1;
            pending.push(adjacent);
        }
    }

    for (std::size_t vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        if (result.distances[vertex] ==
            BreadthFirstSearchResult::not_visited) {
            continue;
        }

        if (result.distances[vertex] >
            result.distances[result.farthest_vertex]) {
            result.farthest_vertex = vertex;
        }
    }

    return result;
}

void reject_negative_weights(const Graph& graph) {
    if (graph.has_negative_weight()) {
        throw std::domain_error(
            "Dijkstra nao suporta arestas com peso negativo"
        );
    }
}

class LinearMinQueue {
public:
    explicit LinearMinQueue(const std::vector<double>& distance)
        : distance_(distance),
          pending_(distance.size(), 1),
          size_(distance.size()) {}

    bool empty() const noexcept {
        return size_ == 0;
    }

    bool contains(std::size_t vertex) const noexcept {
        return pending_[vertex] != 0;
    }

    std::size_t extract_minimum() {
        std::size_t minimum = ShortestPathResult::no_parent;
        for (std::size_t vertex = 0; vertex < distance_.size(); ++vertex) {
            if (!contains(vertex)) {
                continue;
            }
            if (minimum == ShortestPathResult::no_parent ||
                distance_[vertex] < distance_[minimum]) {
                minimum = vertex;
            }
        }
        if (minimum != ShortestPathResult::no_parent) {
            pending_[minimum] = 0;
            --size_;
        }
        return minimum;
    }

    void decrease_key(std::size_t) noexcept {}

private:
    const std::vector<double>& distance_;
    std::vector<unsigned char> pending_;
    std::size_t size_;
};

class IndexedMinHeap {
public:
    explicit IndexedMinHeap(const std::vector<double>& distance)
        : distance_(distance),
          heap_(distance.size()),
          position_(distance.size()) {
        std::iota(heap_.begin(), heap_.end(), std::size_t{0});
        std::iota(position_.begin(), position_.end(), std::size_t{0});
        for (std::size_t index = heap_.size() / 2; index > 0; --index) {
            sift_down(index - 1);
        }
    }

    bool empty() const noexcept {
        return heap_.empty();
    }

    bool contains(std::size_t vertex) const noexcept {
        return position_[vertex] != no_position;
    }

    std::size_t extract_minimum() {
        if (heap_.empty()) {
            return ShortestPathResult::no_parent;
        }

        const std::size_t minimum = heap_.front();
        swap_positions(0, heap_.size() - 1);
        heap_.pop_back();
        position_[minimum] = no_position;
        if (!heap_.empty()) {
            sift_down(0);
        }
        return minimum;
    }

    void decrease_key(std::size_t vertex) {
        sift_up(position_[vertex]);
    }

private:
    static constexpr std::size_t no_position =
        std::numeric_limits<std::size_t>::max();

    bool precedes(std::size_t left, std::size_t right) const noexcept {
        return distance_[left] < distance_[right] ||
               (distance_[left] == distance_[right] && left < right);
    }

    void swap_positions(std::size_t left, std::size_t right) {
        std::swap(heap_[left], heap_[right]);
        position_[heap_[left]] = left;
        position_[heap_[right]] = right;
    }

    void sift_up(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!precedes(heap_[index], heap_[parent])) {
                break;
            }
            swap_positions(index, parent);
            index = parent;
        }
    }

    void sift_down(std::size_t index) {
        while (true) {
            const std::size_t left = index * 2 + 1;
            const std::size_t right = left + 1;
            std::size_t smallest = index;

            if (left < heap_.size() &&
                precedes(heap_[left], heap_[smallest])) {
                smallest = left;
            }
            if (right < heap_.size() &&
                precedes(heap_[right], heap_[smallest])) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }
            swap_positions(index, smallest);
            index = smallest;
        }
    }

    const std::vector<double>& distance_;
    std::vector<std::size_t> heap_;
    std::vector<std::size_t> position_;
};

template <typename MinQueue>
ShortestPathResult run_dijkstra(
    const Graph& graph,
    std::size_t source
) {
    if (source >= graph.vertex_count()) {
        throw std::out_of_range("vertice inicial fora do intervalo do grafo");
    }
    reject_negative_weights(graph);

    ShortestPathResult result;
    result.source = source;
    result.distance.assign(
        graph.vertex_count(),
        std::numeric_limits<double>::infinity()
    );
    result.parent.assign(graph.vertex_count(), ShortestPathResult::no_parent);
    result.distance[source] = 0.0;
    result.parent[source] = source;

    MinQueue pending(result.distance);
    while (!pending.empty()) {
        const std::size_t current = pending.extract_minimum();
        if (result.distance[current] ==
            std::numeric_limits<double>::infinity()) {
            break;
        }

        for (const Edge& edge : graph.neighbors(current)) {
            if (!pending.contains(edge.vertex)) {
                continue;
            }
            const double candidate = result.distance[current] + edge.weight;
            if (candidate < result.distance[edge.vertex]) {
                result.distance[edge.vertex] = candidate;
                result.parent[edge.vertex] = current;
                pending.decrease_key(edge.vertex);
            }
        }
    }

    return result;
}

} // namespace

std::size_t degree(const Graph& graph, std::size_t vertex) {
    return graph.neighbors(vertex).size();
}

std::vector<std::size_t> degrees(const Graph& graph) {
    std::vector<std::size_t> result;
    result.reserve(graph.vertex_count());

    for (std::size_t vertex = 0; vertex < graph.vertex_count(); ++vertex) {
        result.push_back(degree(graph, vertex));
    }

    return result;
}

GraphStatistics calculate_statistics(const Graph& graph) {
    GraphStatistics statistics;
    statistics.vertex_count = graph.vertex_count();
    statistics.edge_count = graph.edge_count();

    auto graph_degrees = degrees(graph);
    if (graph_degrees.empty()) {
        return statistics;
    }

    const auto [minimum, maximum] = std::minmax_element(
        graph_degrees.begin(),
        graph_degrees.end()
    );
    statistics.minimum_degree = *minimum;
    statistics.maximum_degree = *maximum;

    const std::size_t degree_sum = std::accumulate(
        graph_degrees.begin(),
        graph_degrees.end(),
        std::size_t{0}
    );
    statistics.average_degree = static_cast<double>(degree_sum) /
                                static_cast<double>(graph_degrees.size());

    std::sort(graph_degrees.begin(), graph_degrees.end());
    const std::size_t middle = graph_degrees.size() / 2;
    if (graph_degrees.size() % 2 == 0) {
        statistics.median_degree =
            (static_cast<double>(graph_degrees[middle - 1]) +
             static_cast<double>(graph_degrees[middle])) /
            2.0;
    } else {
        statistics.median_degree = static_cast<double>(graph_degrees[middle]);
    }

    return statistics;
}

bool BreadthFirstSearchResult::was_visited(std::size_t vertex) const {
    if (vertex >= level.size()) {
        throw std::out_of_range("vertice fora do resultado da busca");
    }
    return level[vertex] != not_visited;
}

BreadthFirstSearchResult breadth_first_search(
    const Graph& graph,
    std::size_t source
) {
    if (source >= graph.vertex_count()) {
        throw std::out_of_range("vertice inicial fora do intervalo do grafo");
    }

    BreadthFirstSearchResult result;
    result.source = source;
    result.parent.assign(
        graph.vertex_count(),
        BreadthFirstSearchResult::not_visited
    );
    result.level.assign(
        graph.vertex_count(),
        BreadthFirstSearchResult::not_visited
    );
    result.visit_order.reserve(graph.vertex_count());

    std::queue<std::size_t> pending;
    result.parent[source] = source;
    result.level[source] = 0;
    pending.push(source);

    while (!pending.empty()) {
        const std::size_t current = pending.front();
        pending.pop();
        result.visit_order.push_back(current);

        for (const Edge& edge : graph.neighbors(current)) {
            const std::size_t adjacent = edge.vertex;
            if (result.level[adjacent] != BreadthFirstSearchResult::not_visited) {
                continue;
            }

            result.parent[adjacent] = current;
            result.level[adjacent] = result.level[current] + 1;
            pending.push(adjacent);
        }
    }

    return result;
}

std::optional<std::size_t> distance(
    const Graph& graph,
    std::size_t source,
    std::size_t destination
) {
    if (destination >= graph.vertex_count()) {
        throw std::out_of_range("vertice de destino fora do intervalo do grafo");
    }

    const auto search = breadth_first_search(graph, source);
    if (!search.was_visited(destination)) {
        return std::nullopt;
    }
    return search.level[destination];
}

bool ShortestPathResult::was_reached(std::size_t vertex) const {
    if (vertex >= distance.size()) {
        throw std::out_of_range("vertice fora do resultado de Dijkstra");
    }
    return distance[vertex] != std::numeric_limits<double>::infinity();
}

std::vector<std::size_t> ShortestPathResult::path_to(
    std::size_t vertex
) const {
    if (!was_reached(vertex)) {
        return {};
    }

    std::vector<std::size_t> path;
    std::size_t current = vertex;
    while (current != source) {
        path.push_back(current);
        if (current >= parent.size() || parent[current] == no_parent) {
            throw std::logic_error("arvore de caminhos minimos inconsistente");
        }
        current = parent[current];
        if (path.size() > parent.size()) {
            throw std::logic_error("ciclo na arvore de caminhos minimos");
        }
    }
    path.push_back(source);
    std::reverse(path.begin(), path.end());
    return path;
}

ShortestPathResult dijkstra_vector(
    const Graph& graph,
    std::size_t source
) {
    return run_dijkstra<LinearMinQueue>(graph, source);
}

ShortestPathResult dijkstra_heap(
    const Graph& graph,
    std::size_t source
) {
    return run_dijkstra<IndexedMinHeap>(graph, source);
}

bool DepthFirstSearchResult::was_visited(std::size_t vertex) const {
    if (vertex >= level.size()) {
        throw std::out_of_range("vertice fora do resultado da busca");
    }
    return level[vertex] != not_visited;
}

DepthFirstSearchResult depth_first_search(
    const Graph& graph,
    std::size_t source
) {
    if (source >= graph.vertex_count()) {
        throw std::out_of_range("vertice inicial fora do intervalo do grafo");
    }

    DepthFirstSearchResult result;
    result.source = source;
    result.parent.assign(
        graph.vertex_count(),
        DepthFirstSearchResult::not_visited
    );
    result.level.assign(
        graph.vertex_count(),
        DepthFirstSearchResult::not_visited
    );
    result.visit_order.reserve(graph.vertex_count());

    struct StackFrame {
        std::size_t vertex;
        std::vector<Edge> adjacent_edges;
        std::size_t next_neighbor = 0;
    };

    std::vector<StackFrame> stack;
    stack.reserve(graph.vertex_count());

    result.parent[source] = source;
    result.level[source] = 0;
    result.visit_order.push_back(source);
    stack.push_back({source, graph.neighbors(source), 0});

    while (!stack.empty()) {
        StackFrame& frame = stack.back();

        if (frame.next_neighbor >= frame.adjacent_edges.size()) {
            stack.pop_back();
            continue;
        }

        const std::size_t adjacent =
            frame.adjacent_edges[frame.next_neighbor++].vertex;
        if (result.level[adjacent] != DepthFirstSearchResult::not_visited) {
            continue;
        }

        result.parent[adjacent] = frame.vertex;
        result.level[adjacent] = result.level[frame.vertex] + 1;
        result.visit_order.push_back(adjacent);
        stack.push_back({adjacent, graph.neighbors(adjacent), 0});
    }

    return result;
}

std::size_t ConnectedComponent::size() const noexcept {
    return vertices.size();
}

std::vector<ConnectedComponent> connected_components(const Graph& graph) {
    std::vector<ConnectedComponent> components;
    std::vector<unsigned char> visited(
        graph.vertex_count(),
        static_cast<unsigned char>(0)
    );
    std::vector<std::size_t> pending;

    for (std::size_t start = 0; start < graph.vertex_count(); ++start) {
        if (visited[start] != 0) {
            continue;
        }

        ConnectedComponent component;
        pending.clear();
        pending.push_back(start);
        visited[start] = 1;

        while (!pending.empty()) {
            const std::size_t current = pending.back();
            pending.pop_back();
            component.vertices.push_back(current);

            for (const Edge& edge : graph.neighbors(current)) {
                const std::size_t adjacent = edge.vertex;
                if (visited[adjacent] != 0) {
                    continue;
                }
                visited[adjacent] = 1;
                pending.push_back(adjacent);
            }
        }

        // Facilita a exibicao e torna o resultado independente da representacao.
        std::sort(component.vertices.begin(), component.vertices.end());
        components.push_back(std::move(component));
    }

    std::sort(
        components.begin(),
        components.end(),
        [](const ConnectedComponent& left, const ConnectedComponent& right) {
            if (left.size() != right.size()) {
                return left.size() > right.size();
            }
            return left.vertices < right.vertices;
        }
    );

    return components;
}

DiameterResult exact_diameter(const Graph& graph) {
    DiameterResult result;
    result.is_exact = true;

    if (graph.vertex_count() == 0) {
        return result;
    }

    result.first_vertex = 0;
    result.second_vertex = 0;

    for (std::size_t source = 0; source < graph.vertex_count(); ++source) {
        const auto search = breadth_first_distances(graph, source);

        for (std::size_t destination = source + 1;
             destination < graph.vertex_count();
             ++destination) {
            const std::size_t current_distance = search.distances[destination];
            if (current_distance == BreadthFirstSearchResult::not_visited ||
                current_distance <= result.value) {
                continue;
            }

            result.value = current_distance;
            result.first_vertex = source;
            result.second_vertex = destination;
        }
    }

    return result;
}

DiameterResult approximate_diameter(const Graph& graph) {
    DiameterResult result;
    result.is_exact = false;

    const auto components = connected_components(graph);
    if (components.empty()) {
        return result;
    }

    result.first_vertex = components.front().vertices.front();
    result.second_vertex = result.first_vertex;

    for (const auto& component : components) {
        const std::size_t start = component.vertices.front();
        const auto first_search = breadth_first_distances(graph, start);
        const std::size_t first_endpoint = first_search.farthest_vertex;

        const auto second_search = breadth_first_distances(
            graph,
            first_endpoint
        );
        const std::size_t second_endpoint = second_search.farthest_vertex;
        const std::size_t candidate = second_search.distances[second_endpoint];

        if (candidate > result.value) {
            result.value = candidate;
            result.first_vertex = first_endpoint;
            result.second_vertex = second_endpoint;
        }
    }

    return result;
}
