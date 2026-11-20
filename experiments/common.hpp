#ifndef GRAPH_STUDY_EXPERIMENT_COMMON_HPP
#define GRAPH_STUDY_EXPERIMENT_COMMON_HPP

#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace experiment {

using Clock = std::chrono::steady_clock;

inline std::uint64_t parse_unsigned(std::string_view text, const char* field) {
    if (text.empty()) throw std::invalid_argument(std::string(field) + " invalido");
    std::uint64_t value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    if (result.ec != std::errc{} ||
        result.ptr != text.data() + text.size()) {
        throw std::invalid_argument(std::string(field) + " invalido");
    }
    return value;
}

inline std::size_t parse_positive_size(std::string_view text, const char* field) {
    const auto value = parse_unsigned(text, field);
    if (value == 0 || value > std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument(std::string(field) + " invalido");
    }
    return static_cast<std::size_t>(value);
}

inline void prepare_output(const std::string& path) {
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
}

inline std::ofstream open_output(const std::string& path) {
    prepare_output(path);
    std::ofstream output(path);
    if (!output) throw std::runtime_error("nao foi possivel criar " + path);
    return output;
}

inline void finish_output(std::ofstream& output, const std::string& path) {
    output.flush();
    if (!output) throw std::runtime_error("falha ao escrever " + path);
    output.close();
    if (!output) throw std::runtime_error("falha ao fechar " + path);
}

inline double elapsed_ms(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}

inline void write_io_timing(const std::string& prefix, double load_ms, Clock::time_point export_begin) {
    const auto export_ms = elapsed_ms(export_begin);
    const auto path = prefix + "_io.csv";
    auto output = open_output(path);
    output << "load_ms,export_ms\n" << load_ms << ',' << export_ms << '\n';
    finish_output(output, path);
}

} // namespace experiment

#endif
