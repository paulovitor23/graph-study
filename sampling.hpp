#ifndef GRAPH_STUDY_SAMPLING_HPP
#define GRAPH_STUDY_SAMPLING_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_set>
#include <vector>

// Amostragem uniforme sem reposicao (Floyd), com espaco O(k).
inline std::vector<std::size_t> select_sources(
    std::size_t vertex_count, std::size_t requested, std::uint64_t seed
) {
    const auto count = std::min(vertex_count, requested);
    std::mt19937_64 generator(seed);
    std::unordered_set<std::size_t> chosen;
    std::vector<std::size_t> sources;
    chosen.reserve(count);
    sources.reserve(count);
    for (std::size_t offset = 0; offset < count; ++offset) {
        const auto upper = vertex_count - count + offset;
        std::uniform_int_distribution<std::size_t> distribution(0, upper);
        auto vertex = distribution(generator);
        if (!chosen.insert(vertex).second) {
            vertex = upper;
            chosen.insert(vertex);
        }
        sources.push_back(vertex);
    }
    std::shuffle(sources.begin(), sources.end(), generator);
    return sources;
}

#endif
