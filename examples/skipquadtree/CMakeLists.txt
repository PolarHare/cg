cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(skipquadtree)

add_executable(skipquadtree skipquadtree.cpp)
target_link_libraries(skipquadtree visualization ${GMP_LIBRARIES})