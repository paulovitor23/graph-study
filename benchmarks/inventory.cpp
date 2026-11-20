#include "graph_io.hpp"
#include "experiments/common.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("uso: graph_inventory <grafo>");
        std::ifstream input(argv[1]);
        std::string line;
        if (!input || !std::getline(input, line)) throw std::runtime_error("entrada ausente");
        const auto vertices = graph_io::vertex_count(line);
        if (vertices > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("inventario de pares requer identificadores de 32 bits");
        }
        std::vector<std::uint64_t> arcs;
        arcs.reserve(std::filesystem::file_size(argv[1]) / 12);
        std::size_t lines = 0, loops = 0, negative = 0, invalid = 0, malformed = 0;
        double minimum = std::numeric_limits<double>::infinity();
        double maximum = -minimum;
        while (std::getline(input, line)) {
            if (line.find_first_not_of(" \t\r\v\f") == std::string::npos) continue;
            ++lines;
            graph_io::FileEdge edge;
            try { edge = graph_io::edge(line); }
            catch (const std::runtime_error&) { ++malformed; continue; }
            if (edge.u == 0 || edge.v == 0 || edge.u > vertices || edge.v > vertices) {
                ++invalid;
                continue;
            }
            loops += edge.u == edge.v;
            negative += edge.weight < 0;
            minimum = std::min(minimum, edge.weight);
            maximum = std::max(maximum, edge.weight);
            arcs.push_back((std::uint64_t(edge.u) << 32) | edge.v);
        }
        if (input.bad()) throw std::runtime_error("falha ao ler entrada");
        std::sort(arcs.begin(), arcs.end());
        const auto unique = std::unique(arcs.begin(), arcs.end()) - arcs.begin();
        std::cout << "vertices,edge_lines,unique_ordered_arcs,ordered_duplicates,self_loops,negative_weight_lines,min_weight,max_weight,invalid_ids,malformed_lines\n"
                  << vertices << ',' << lines << ',' << unique << ',' << arcs.size() - unique
                  << ',' << loops << ',' << negative << ',';
        if (!arcs.empty()) std::cout << minimum;
        std::cout << ',';
        if (!arcs.empty()) std::cout << maximum;
        std::cout << ',' << invalid << ',' << malformed << '\n';
        return invalid == 0 && malformed == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
