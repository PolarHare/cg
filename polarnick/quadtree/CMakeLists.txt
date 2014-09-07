cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

project(quadtree)

add_executable(quadtree quadtree.cpp)
add_executable(skipquadtree skipquadtree.cpp)
add_executable(skipquadtree2 skipquadtree2.cpp)
target_link_libraries(quadtree visualization ${GMP_LIBRARIES})
target_link_libraries(skipquadtree visualization ${GMP_LIBRARIES})
target_link_libraries(skipquadtree2 visualization ${GMP_LIBRARIES})
