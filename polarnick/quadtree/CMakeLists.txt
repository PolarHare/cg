cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(skipquadtree)

add_executable(skipquadtreevizualization skipquadtreevizualization.cpp skipquadtree.cpp)
target_link_libraries(skipquadtreevizualization visualization ${GMP_LIBRARIES})