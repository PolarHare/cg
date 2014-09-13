#include <gtest/gtest.h>

#include <cg/structures/skipquadtree.h>

#define WIDTH 640
#define HEIGHT 480
#define MIN_X (-WIDTH / 2)
#define MAX_X (WIDTH / 2)
#define MIN_Y (-HEIGHT / 2)
#define MAX_Y (HEIGHT / 2)

TEST(skipquadtree_initialization, test1) {
    SkipQuadTree tree(MIN_X, MAX_X, MIN_Y, MAX_Y);
    tree.addPoint(point_2f(-83 ,32));
    tree.addPoint(point_2f(210 ,50));
    tree.addPoint(point_2f(87,108));
    tree.addPoint(point_2f(0 ,124));
    tree.addPoint(point_2f(0 ,179));
    tree.addPoint(point_2f(222 ,59));
    tree.addPoint(point_2f(294 ,60));
    tree.addPoint(point_2f(176 ,174));
}