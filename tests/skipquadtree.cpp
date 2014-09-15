#include <gtest/gtest.h>

#include "random_utils.h"
#include <cg/structures/skipquadtree.h>

#define COORD_RANGE 200
#define WIDTH COORD_RANGE
#define HEIGHT COORD_RANGE
#define MIN_X (-WIDTH / 2)
#define MAX_X (WIDTH / 2)
#define MIN_Y (-HEIGHT / 2)
#define MAX_Y (HEIGHT / 2)

TEST(skipquadtree_initialization, test1) {
    size_t countOfPoints = 10000;

    SkipQuadTree tree;
    std::vector<point_2f> points = uniform_points<float>(countOfPoints, -COORD_RANGE / 2, COORD_RANGE / 2);
    for (point_2f point : points) {
        tree.addPoint(point);
    }
}

TEST(skipquadtree_initialization, test2) {
    size_t countOfPoints = 100000;

    SkipQuadTree tree;
    std::vector<point_2f> points = uniform_points<float>(countOfPoints, -COORD_RANGE / 2, COORD_RANGE / 2);
    for (point_2f point : points) {
        tree.addPoint(point);
    }
}

TEST(skipquadtree_initialization_getContain, test1) {
    size_t countOfPoints = 10000;
    size_t countOfGetContainsQueries = 10000;
    float eps = 0.1;

    SkipQuadTree tree;
    std::vector<point_2f> points = uniform_points<float>(countOfPoints, -COORD_RANGE / 2, COORD_RANGE / 2);
    for (point_2f point : points) {
        tree.addPoint(point);
    }
    auto rectPoints = uniform_points<float>(countOfGetContainsQueries);
    for (size_t i = 0; i < rectPoints.size() - 1; i++) {
        tree.getContainWithId(rectPoints[i], rectPoints[1], eps);
    }
}

std::vector<point_2f> makeUnique(std::vector<point_2f> points) {
    std::vector<point_2f> result;
    for (auto point : points) {
        bool was = false;
        for (auto resP : result) {
            if (point.x == resP.x && point.y == resP.y) {
                was = true;
            }
        }
        if (!was) {
            result.push_back(point);
        }
    }
    return result;
}

TEST(skipquadtree_initialization_getContain, test2) {
    size_t countOfPoints = 10000;
    size_t countOfGetContainsQueries = 100;
    float eps = 0.1;

    SkipQuadTree tree;
    std::vector<point_2f> points = uniform_points<float>(countOfPoints - 2, -COORD_RANGE / 2, COORD_RANGE / 2);
    points.push_back(point_2f(0.239, 0.2391));
    points.push_back(point_2f(0.239, 0.2391));
    for (point_2f point : points) {
        tree.addPoint(point);
    }
    points = makeUnique(points);

    auto rectPoints = uniform_points<float>(countOfGetContainsQueries);
    for (size_t i = 0; i < rectPoints.size() - 1; i++) {
        std::list<point_2f> res = tree.getContain(rectPoints[i], rectPoints[i + 1], eps);

        float fromX = std::min(rectPoints[i].x, rectPoints[i + 1].x);
        float toX = std::max(rectPoints[i].x, rectPoints[i + 1].x);
        float fromY = std::min(rectPoints[i].y, rectPoints[i + 1].y);
        float toY = std::max(rectPoints[i].y, rectPoints[i + 1].y);
        bool resCorrect[res.size()];
        for (size_t j = 0; j < res.size(); j++) {
            resCorrect[j] = false;
        }

        for (auto point : points) {
            if (point.x >= fromX && point.x < toX
                    && point.y >= fromY && point.y < toY) {
                bool wasFoundInRes = false;
                size_t j = 0;
                for (point_2f resPoint : res) {
                    if (resPoint.x == point.x && resPoint.y == point.y) {
                        wasFoundInRes = true;
                        resCorrect[j] = true;
                        break;
                    }
                    j++;
                }
                EXPECT_TRUE(wasFoundInRes);
            }
        }

        size_t j = 0;
        size_t countOfOnEdge = 0;
        for (point_2f resPoint : res) {
            if (!resCorrect[j]) {
                EXPECT_TRUE(resPoint.x >= fromX - eps && resPoint.x < toX + eps && resPoint.y >= fromY - eps && resPoint.y < toY + eps);
                countOfOnEdge++;
            }
            j++;
        }
        if (countOfOnEdge > 0) {
            printf("[          ] Count of points on edge: %zu/%zu\n", countOfOnEdge, res.size());
        }
    }
}