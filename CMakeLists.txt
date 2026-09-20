cmake_minimum_required(VERSION 3.16)
project(graph_study LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(graph_study graph.cpp algorithms.cpp)
target_include_directories(graph_study PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(graph_study PRIVATE -Wall -Wextra -Wpedantic)

add_executable(graph_tests tests.cpp)
target_link_libraries(graph_tests PRIVATE graph_study)
target_compile_options(graph_tests PRIVATE -Wall -Wextra -Wpedantic)

add_executable(graph_experiments experiments.cpp)
target_link_libraries(graph_experiments PRIVATE graph_study)
target_compile_options(graph_experiments PRIVATE -Wall -Wextra -Wpedantic)
if(WIN32)
    target_link_libraries(graph_experiments PRIVATE psapi)
endif()

enable_testing()
add_test(NAME graph_tests COMMAND graph_tests)
