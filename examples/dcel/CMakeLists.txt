cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(dcel)

add_executable(dcel CELLtest.cpp)
target_link_libraries(dcel visualization ${GMP_LIBRARIES})