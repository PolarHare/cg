#pragma once

#include <list>
#include <memory>

#include "cg/io/point.h"

using cg::point_2f;

bool isEagle();

struct Range {
    int lvl;
    float fromX;
    float toX;
    float fromY;
    float toY;

    Range(int lvl, float fromX, float toX, float fromY, float toY)
            : lvl(lvl), fromX(fromX), toX(toX), fromY(fromY), toY(toY) {
    }

    Range(float fromX, float toX, float fromY, float toY)
            : Range(-1, fromX, toX, fromY, toY) {
    }

    float getMiddleX() const;

    float getMiddleY() const;

    point_2f getMiddlePoint() const;

    Range localize(point_2f point);

    int recognizePartId(point_2f point);

};

static int nodesCount = 0;
static int nodeNextId = 1;

struct Node {

    int id;

    Node() {
        id = nodeNextId;
        nodeNextId++;
        nodesCount++;
    }

    virtual std::list<std::pair<int, point_2f>> getContain(Range range, float eps) = 0;

    virtual std::list<std::pair<int, point_2f>> getAll() = 0;

    virtual ~Node() {
        nodesCount--;
    }
};

struct MiddleNode : Node {
    Range range;
    std::shared_ptr<Node> children[4];
    std::shared_ptr<MiddleNode> linkToMoreDetailed;

    MiddleNode(Range range) : range(range) {
    }

    bool addPoint(point_2f point);

    virtual std::list<std::pair<int, point_2f>> getContain(Range range, float eps) override;

    virtual std::list<std::pair<int, point_2f>> getAll() override;
};

struct TermNode : Node {
    point_2f point;

    TermNode(point_2f point) : point(point) {
    }

    virtual std::list<std::pair<int, point_2f>> getContain(Range range, float eps) override;

    virtual std::list<std::pair<int, point_2f>> getAll() override;
};

struct SkipQuadTree {
    float fromX;
    float toX;
    float fromY;
    float toY;
    int skipLevels = 1;
    std::shared_ptr<MiddleNode> lowDetailedRoot;

    SkipQuadTree(float fromX, float toX, float fromY, float toY);

    std::list<std::pair<int, point_2f>> getContain(Range range, float eps);

    bool addPoint(point_2f point);
};