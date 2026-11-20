#include "algorithms.hpp"
#include "common.hpp"
#include "sampling.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <optional>

namespace {

using experiment::Clock;
constexpr std::array<std::size_t, 5> pairs{10, 20, 30, 40, 50};
struct PredictedTimeLimit {};

double seconds(Clock::time_point begin) {
    return std::chrono::duration<double>(Clock::now() - begin).count();
}

double positive_seconds(const char* text) {
    std::size_t consumed = 0;
    const auto value = std::stod(text, &consumed);
    if (consumed != std::strlen(text) || !std::isfinite(value) || value <= 0) {
        throw std::invalid_argument("limite de tempo invalido");
    }
    return value;
}

std::uint64_t fingerprint(const std::vector<double>& distances) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const double value : distances) {
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        hash = (hash ^ bits) * 1099511628211ULL;
    }
    return hash;
}

const char* status_name(PathStatus status) {
    switch (status) {
    case PathStatus::Finite: return "finite";
    case PathStatus::Unreachable: return "unreachable";
    case PathStatus::NegativeCycle: return "negative_cycle";
    }
    throw std::logic_error("estado de caminho invalido");
}

void query(std::ofstream& output, const Graph& graph, const char* phase, std::size_t round,
           std::size_t source, std::size_t target, PathStatus status, double distance,
           const std::vector<std::size_t>& path, bool negative_cycle) {
    output << phase << ',' << round << ',' << source << ',' << target << ',' << status_name(status) << ',';
    if (status == PathStatus::Finite) {
        double cost = 0;
        if (path.empty() || path.front() + 1 != source || path.back() + 1 != target) {
            throw std::logic_error("extremos do caminho invalidos");
        }
        for (std::size_t i = 1; i < path.size(); ++i) {
            bool found = false;
            for (const auto& edge : graph.neighbor_view(path[i - 1])) {
                if (edge.vertex == path[i]) { cost += edge.weight; found = true; break; }
            }
            if (!found) throw std::logic_error("arco inexistente no caminho");
        }
        if (std::abs(cost - distance) > 1e-9 * std::max(1.0, std::abs(distance))) {
            throw std::logic_error("custo do caminho inconsistente");
        }
        output << distance;
    } else output << (status == PathStatus::Unreachable ? "inf" : "-inf");
    output << ',';
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (i) output << '-';
        output << path[i] + 1;
    }
    output << ',' << negative_cycle << '\n';
    output.flush();
    if (!output) throw std::runtime_error("falha ao escrever consultas");
}

void bf_queries(std::ofstream& output, const Graph& graph, const BellmanFordResult& result,
                const char* phase, std::size_t round, bool all_pairs) {
    const auto target = result.target + 1;
    if (all_pairs && std::find(pairs.begin(), pairs.end(), target) == pairs.end()) return;
    for (const auto source : pairs) {
        if (!all_pairs && source > 30) break;
        if (source > graph.vertex_count()) continue;
        const auto state = result.status(source - 1);
        query(output, graph, phase, round, source, target, state, result.distance[source - 1],
              state == PathStatus::Finite ? result.path_from(source - 1) : std::vector<std::size_t>{},
              result.has_negative_cycle);
    }
}

int run(int argc, char** argv) {
    if (argc != 8) throw std::invalid_argument(
        "uso: directed_graph_experiments <grafo> <saida> <bf_target|bf_all_pairs|floyd> "
        "<rodadas> <orcamento_memoria_bytes> <limite_segundos> <semente_piloto>"
    );
    const std::string mode = argv[3];
    if (mode != "bf_target" && mode != "bf_all_pairs" && mode != "floyd") {
        throw std::invalid_argument("algoritmo invalido");
    }
    const auto rounds = experiment::parse_positive_size(argv[4], "rodadas");
    const auto budget = experiment::parse_positive_size(argv[5], "orcamento de memoria");
    const auto wall_limit = positive_seconds(argv[6]);
    const auto seed = experiment::parse_unsigned(argv[7], "semente piloto");
    const auto wall_begin = Clock::now();
    const auto load_begin = Clock::now();
    const auto graph = read_graph_from_file(argv[1], GraphRepresentation::AdjacencyList, GraphDirection::Directed);
    const auto n = graph->vertex_count();
    if (n < 100) throw std::invalid_argument("estudo de caso requer ao menos 100 vertices");
    std::cout << std::setprecision(17);
    std::cout << "{\"event\":\"loaded\",\"seconds\":" << seconds(load_begin)
              << ",\"vertices\":" << n << ",\"edges\":" << graph->edge_count()
              << ",\"negative_weights\":" << graph->has_negative_weight() << "}" << std::endl;
    const auto out = std::filesystem::path(argv[2]);
    std::filesystem::create_directories(out);
    auto timings = experiment::open_output((out / "timings.csv").string());
    timings << "round,seconds,targets,checksum,has_negative_cycle,relaxation_passes,examined_edges\n" << std::setprecision(17);
    auto pilots = experiment::open_output((out / "pilots.csv").string());
    pilots << "target,seconds,has_negative_cycle,relaxation_passes,examined_edges\n" << std::setprecision(17);
    auto queries = experiment::open_output((out / "queries.csv").string());
    queries << "phase,round,source,target,status,distance,path,has_negative_cycle\n" << std::setprecision(17);

    std::optional<std::uint64_t> expected;
    const auto observer_for = [&](Clock::time_point begin, std::size_t calls_per_round) -> AlgorithmObserver {
        return [&, begin, calls_per_round, last = Clock::now(), phase_begin = begin,
                prefix_s = 0.0](const AlgorithmProgress& progress) mutable {
            if (progress.completed == 0) {
                phase_begin = Clock::now();
                prefix_s = seconds(begin);
                return;
            }
            const auto elapsed = seconds(begin);
            if (seconds(last) < .5) return;
            last = Clock::now();
            double estimate = 0;
            // Para BF, projetar V passos apenas apos persistencia em grafo com negativos.
            if (mode == "floyd" || (graph->has_negative_weight() && progress.completed >= 128)) {
                estimate = (prefix_s + seconds(phase_begin) * static_cast<double>(progress.total) / progress.completed)
                         * calls_per_round * rounds;
            }
            std::cout << "{\"event\":\"progress\",\"phase\":\"" << progress.phase
                      << "\",\"completed\":" << progress.completed << ",\"total\":" << progress.total
                      << ",\"elapsed_s\":" << elapsed << ",\"estimated_total_s\":" << estimate
                      << ",\"examined_edges\":" << progress.examined_edges << "}" << std::endl;
            if (estimate > 0 && estimate + seconds(wall_begin) >= wall_limit) throw PredictedTimeLimit{};
        };
    };
    const auto accept_signature = [&](std::uint64_t signature) {
        if (expected && *expected != signature) throw std::logic_error("resultados divergem entre rodadas");
        expected = signature;
    };
    const auto finish_round = [&](std::size_t round, double elapsed, std::size_t targets,
                                  std::uint64_t signature, bool negative, std::size_t passes, std::size_t edges) {
        accept_signature(signature);
        timings << round << ',' << elapsed << ',' << targets << ',' << signature << ','
                << negative << ',' << passes << ',' << edges << '\n';
        timings.flush();
        if (!timings) throw std::runtime_error("falha ao escrever tempos");
        std::cout << "{\"event\":\"round\",\"round\":" << round << ",\"seconds\":" << elapsed
                  << ",\"targets\":" << targets << "}" << std::endl;
        const auto projected = elapsed * (rounds - round) + seconds(wall_begin);
        if (round < rounds && projected >= wall_limit) throw PredictedTimeLimit{};
    };

    try {
        if (mode == "floyd") {
            auto begin = Clock::now();
            double pilot_s = 0;
            {
                const auto pilot = floyd_warshall(*graph, budget, observer_for(begin, 1));
                pilot_s = seconds(begin);
                pilots << "0," << pilot_s << ',' << pilot.has_negative_cycle << ",0,0\n";
                expected = fingerprint(pilot.distance);
                for (const auto u : pairs) for (const auto v : pairs) {
                    const auto state = pilot.status(u - 1, v - 1);
                    query(queries, *graph, "pilot", 0, u, v, state, pilot.distance_between(u - 1, v - 1),
                          state == PathStatus::Finite ? pilot.path(u - 1, v - 1) : std::vector<std::size_t>{},
                          pilot.has_negative_cycle);
                }
            }
            pilots.flush();
            std::cout << "{\"event\":\"forecast\",\"estimated_total_s\":" << pilot_s * rounds
                      << ",\"method\":\"completed_pilot\"}" << std::endl;
            if (pilot_s * rounds + seconds(wall_begin) >= wall_limit) throw PredictedTimeLimit{};
            for (std::size_t round = 1; round <= rounds; ++round) {
                begin = Clock::now();
                const auto result = floyd_warshall(*graph, budget);
                const auto elapsed = seconds(begin);
                const auto signature = fingerprint(result.distance);
                for (const auto u : pairs) for (const auto v : pairs) {
                    const auto state = result.status(u - 1, v - 1);
                    query(queries, *graph, "round", round, u, v, state, result.distance_between(u - 1, v - 1),
                          state == PathStatus::Finite ? result.path(u - 1, v - 1) : std::vector<std::size_t>{},
                          result.has_negative_cycle);
                }
                finish_round(round, elapsed, n, signature, result.has_negative_cycle, 0, 0);
            }
        } else {
            std::vector<std::size_t> destinations{99};
            if (mode == "bf_all_pairs") {
                // Amostra uniforme sem reposicao, independente do destino fixo da pergunta 1.
                auto sample = select_sources(n, 3, seed);
                for (const auto t : sample) if (std::find(destinations.begin(), destinations.end(), t) == destinations.end()) {
                    destinations.push_back(t);
                }
            }
            double minimum_pilot = std::numeric_limits<double>::infinity();
            for (const auto target : destinations) {
                const auto begin = Clock::now();
                const auto result = bellman_ford_to_target(*graph, target, observer_for(begin, mode == "bf_all_pairs" ? n : 1));
                const auto elapsed = seconds(begin);
                minimum_pilot = std::min(minimum_pilot, elapsed);
                pilots << target + 1 << ',' << elapsed << ',' << result.has_negative_cycle << ','
                       << result.relaxation_passes << ',' << result.examined_edges << '\n';
                pilots.flush();
                if (mode == "bf_target") {
                    expected = fingerprint(result.distance);
                    bf_queries(queries, *graph, result, "pilot", 0, false);
                }
                std::cout << "{\"event\":\"pilot\",\"target\":" << target + 1 << ",\"seconds\":" << elapsed << "}" << std::endl;
            }
            const auto forecast = minimum_pilot * (mode == "bf_all_pairs" ? n : 1) * rounds;
            std::cout << "{\"event\":\"forecast\",\"estimated_total_s\":" << forecast
                      << ",\"method\":\"fastest_completed_pilot\"}" << std::endl;
            if (forecast + seconds(wall_begin) >= wall_limit) throw PredictedTimeLimit{};
            for (std::size_t round = 1; round <= rounds; ++round) {
                double elapsed = 0;
                std::uint64_t signature = 0;
                bool negative = false;
                std::size_t passes = 0, edges = 0;
                const auto targets = mode == "bf_all_pairs" ? n : 1;
                const auto round_begin = Clock::now();
                double excluded = 0;
                for (std::size_t index = 0; index < targets; ++index) {
                    const auto target = mode == "bf_all_pairs" ? index : std::size_t{99};
                    Clock::time_point outside;
                    {
                    const auto result = bellman_ford_to_target(*graph, target);
                    outside = Clock::now();
                    signature = signature * 1099511628211ULL ^ fingerprint(result.distance);
                    negative = negative || result.has_negative_cycle;
                    passes += result.relaxation_passes;
                    edges += result.examined_edges;
                    bf_queries(queries, *graph, result, "round", round, mode == "bf_all_pairs");
                    if (targets > 1 && (index + 1 == targets || index % 16 == 0)) {
                        std::cout << "{\"event\":\"target_progress\",\"round\":" << round
                                  << ",\"completed\":" << index + 1 << ",\"total\":" << targets << "}" << std::endl;
                    }
                    }
                    excluded += seconds(outside);
                }
                elapsed = seconds(round_begin) - excluded;
                finish_round(round, elapsed, targets, signature, negative, passes, edges);
            }
        }
        experiment::finish_output(timings, (out / "timings.csv").string());
        experiment::finish_output(pilots, (out / "pilots.csv").string());
        experiment::finish_output(queries, (out / "queries.csv").string());
        std::cout << "{\"event\":\"done\",\"status\":\"complete\"}" << std::endl;
        return 0;
    } catch (const PredictedTimeLimit&) {
        std::cout << "{\"event\":\"done\",\"status\":\"aborted\",\"reason\":\"predicted_time_limit\"}" << std::endl;
        return 2;
    }
}

} // namespace

int main(int argc, char** argv) {
    try { return run(argc, argv); }
    catch (const std::bad_alloc&) { std::cerr << "memory_limit\n"; return 3; }
    catch (const std::length_error& error) { std::cerr << "memory_limit: " << error.what() << '\n'; return 3; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
