#ifndef GRAPH_STUDY_TEST_CHECK_HPP
#define GRAPH_STUDY_TEST_CHECK_HPP

#include <stdexcept>
#include <string>

namespace test_support {

inline void require(bool condition, const char* expression, const char* file, int line) {
    if (!condition) {
        throw std::runtime_error(
            std::string(file) + ':' + std::to_string(line) + ": " + expression
        );
    }
}

} // namespace test_support

#define CHECK(condition) test_support::require((condition), #condition, __FILE__, __LINE__)

#endif
