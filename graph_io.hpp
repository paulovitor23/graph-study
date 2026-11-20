#ifndef GRAPH_STUDY_GRAPH_IO_HPP
#define GRAPH_STUDY_GRAPH_IO_HPP

#include <charconv>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string_view>

// Parsing interno do formato textual, sem alocar streams por linha.
namespace graph_io {

inline std::string_view token(std::string_view& line) {
    const auto first = line.find_first_not_of(" \t\r\n\v\f");
    if (first == std::string_view::npos) {
        line = {};
        return {};
    }
    line.remove_prefix(first);
    const auto end = line.find_first_of(" \t\r\n\v\f");
    const auto value = line.substr(0, end);
    line.remove_prefix(value.size());
    return value;
}

inline std::size_t unsigned_value(std::string_view text) {
    if (!text.empty() && text.front() == '+') text.remove_prefix(1);
    if (text.empty()) throw std::runtime_error("inteiro nao negativo ausente ou invalido");
    std::size_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
        throw std::runtime_error("inteiro nao negativo ausente ou invalido");
    }
    return value;
}

inline std::size_t vertex_count(std::string_view line) {
    const auto count = unsigned_value(token(line));
    if (!token(line).empty()) throw std::runtime_error("colunas extras no cabecalho");
    return count;
}

struct FileEdge {
    std::size_t u;
    std::size_t v;
    double weight;
};

inline FileEdge edge(std::string_view line) {
    const auto u = unsigned_value(token(line));
    const auto v = unsigned_value(token(line));
    auto weight_text = token(line);
    double weight = 1.0;
    if (!weight_text.empty()) {
        if (weight_text.front() == '+') {
            weight_text.remove_prefix(1);
            if (!weight_text.empty() && weight_text.front() == '-') {
                throw std::runtime_error("peso da aresta invalido");
            }
        }
        if (weight_text.empty()) throw std::runtime_error("peso da aresta invalido");
        const auto result = std::from_chars(
            weight_text.data(), weight_text.data() + weight_text.size(), weight
        );
        if (result.ec != std::errc{} || result.ptr != weight_text.data() + weight_text.size() ||
            !std::isfinite(weight)) {
            throw std::runtime_error("peso da aresta invalido ou nao finito");
        }
    }
    if (!token(line).empty()) throw std::runtime_error("linha de aresta possui colunas extras");
    return {u, v, weight};
}

} // namespace graph_io

#endif
