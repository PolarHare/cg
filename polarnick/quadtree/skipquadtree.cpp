#include <vector>
#include <fstream>
#include <utility>
#include <algorithm>
#include <list>
#include <memory>

#include <QColor>
#include <QApplication>

#include "cg/visualization/viewer_adapter.h"
#include "cg/visualization/draw_util.h"

#include <cg/primitives/contour.h>
#include "cg/triangulation/triangulation.h"

#include "cg/io/point.h"
#include "skipquadtree.hpp"

using cg::point_2f;
using cg::vector_2f;
using cg::rectangle_2f;

#define P 0.7


bool isEagle() {
    int randValue = std::rand();
    return randValue < P * RAND_MAX;
}

// Range implementation BEGIN
float Range::getMiddleX() {
    return (fromX + toX) / 2;
}

float Range::getMiddleY() {
    return (fromY + toY) / 2;
}

point_2f Range::getMiddlePoint() {
    return point_2f(getMiddleX(), getMiddleY());
}

Range Range::localize(point_2f point) {
    float resFromX;
    float resToX;
    if (point.x < getMiddleX()) {
        resFromX = fromX;
        resToX = getMiddleX();
    } else {
        resFromX = getMiddleX();
        resToX = toX;
    }
    float resFromY;
    float resToY;
    if (point.y < getMiddleY()) {
        resFromY = fromY;
        resToY = getMiddleY();
    } else {
        resFromY = getMiddleY();
        resToY = toY;
    }
    return Range(lvl == -1 ? -1 : lvl + 1,
            resFromX, resToX, resFromY, resToY);
}

int Range::recognizePartId(point_2f point) {
    if (point.x < fromX || point.x >= toX
            || point.y < fromY || point.y >= toY) {
        return -1;
    }
    int res = 0;
    if (point.x >= getMiddleX()) {
        res++;
    }
    res *= 2;
    if (point.y >= getMiddleY()) {
        res++;
    }
    return res;
}

std::ostream &operator<<(std::ostream &os, Range const &range) {
    return os << "{x=[" << range.fromX << ", " << range.toX << ")"
            " y=[" << range.fromY << ", " << range.toY << ") lvl=" << range.lvl << "}";
}
// Range implementation END

// Node implementation BEGIN
std::ostream &operator<<(std::ostream &os, Node const &node) {
    os << "id=" << node.id << " ";
    const MiddleNode *middleNode = dynamic_cast<const MiddleNode*>(&node);
    if (middleNode != NULL) {
        os << middleNode->range << "children[";
        for (int i = 0; i < 4; i++) {
            if (middleNode->children[i] == NULL) {
                os << "-";
            } else {
                os << middleNode->children[i]->id;
            }
            if (i != 3) {
                os << ", ";
            }
        }
        os << "]";
    } else {
        const TermNode *termNode = dynamic_cast<const TermNode *>(&node);
        assert (termNode != NULL);
        os << termNode->point;
    }
    return os;
}
// Node implementation END

// TermNode implementation BEGIN
std::list<std::pair<int, point_2f>> TermNode::getContain(Range range, float eps) {
    if (point.x >= range.fromX && point.x < range.toX
            && point.y >= range.fromY && point.y < range.toY) {
        return {{id, point}};
    } else {
        return {};
    }
}

std::list<std::pair<int, point_2f>> TermNode::getAll() {
    return {{id, point}};
}
// TermNode implementation END



Range determineCommonRange(point_2f point, MiddleNode *node, Range min) {
    Range range = node->range;
    while (true) {
        Range newMin = min.localize(point);
        if (newMin.fromX > range.fromX || newMin.toX < range.toX
                || newMin.fromY > range.fromY || newMin.toY < range.toY) {
            break;
        }
        min = newMin;
    }
    return min;
}

Range determineCommonRange(point_2f p1, point_2f p2, Range min) {
    while (true) {
        Range newMin1 = min.localize(p1);
        Range newMin2 = min.localize(p2);
        if (newMin1.fromX != newMin2.fromX || newMin1.toX != newMin2.toX
                || newMin1.fromY != newMin2.fromY || newMin1.toY != newMin2.toY) {
            break;
        }
        min = newMin1;
    }
    return min;
}

// MiddleNode implementation BEGIN
std::list<std::pair<int, point_2f>> MiddleNode::getContain(Range rect, float eps) {
    if (range.fromX >= rect.toX || range.toX <= rect.fromX || range.fromY >= rect.toY || range.toY <= rect.fromY) {
        return {};
    }

    if (range.fromX >= rect.fromX - eps && range.toX < rect.toX + eps
            && range.fromY >= rect.fromY - eps && range.toY < rect.toY + eps) {
        return getAll();
    }

    bool childWasGetted[4] = {false, false, false, false};
    int gettedCount = 0;
    MiddleNode *cur = this;
    std::list<std::pair<int, point_2f>> result;
    while (gettedCount != 4) {
        for (int i = 0; i < 4; i++) {
            if (childWasGetted[i] || cur->children[i] == NULL) {
                continue;
            }
            if (cur->linkToMoreDetailed != NULL) {
                MiddleNode* child = dynamic_cast<MiddleNode *>(cur->children[i].get());
                if (child != NULL && child->range.lvl == cur->range.lvl + 1) {
                    childWasGetted[i] = true;
                    gettedCount++;
                    result.splice(result.end(), child->getContain(rect, eps));
                }
            } else {
                childWasGetted[i] = true;
                gettedCount++;
                result.splice(result.end(), cur->children[i]->getContain(rect, eps));
            }
        }
        if (cur->linkToMoreDetailed != NULL) {
            cur = &(*cur->linkToMoreDetailed);
        } else {
            break;
        }
    }
    return result;
}

std::list<std::pair<int, point_2f>> MiddleNode::getAll() {
    bool childWasGetted[4] = {false, false, false, false};
    int gettedCount = 0;
    MiddleNode *cur = this;
    std::list<std::pair<int, point_2f>> result;
    while (gettedCount != 4) {
        for (int i = 0; i < 4; i++) {
            if (childWasGetted[i] || cur->children[i] == NULL) {
                continue;
            }
            if (cur->linkToMoreDetailed != NULL) {
                MiddleNode *child = dynamic_cast<MiddleNode *>(cur->children[i].get());
                if (child != NULL && child->range.lvl == cur->range.lvl + 1) {
                    childWasGetted[i] = true;
                    gettedCount++;
                    result.splice(result.end(), child->getAll());
                }
            } else {
                childWasGetted[i] = true;
                gettedCount++;
                result.splice(result.end(), cur->children[i]->getAll());
            }
        }
        if (cur->linkToMoreDetailed != NULL) {
            cur = &(*cur->linkToMoreDetailed);
        } else {
            break;
        }
    }
    return result;
}

bool MiddleNode::addPoint(point_2f point) {
    int index = range.recognizePartId(point);
    if (children[index] != NULL) {
        MiddleNode* child = dynamic_cast<MiddleNode*>(children[index].get());
        if (child != NULL) {
            if (child->range.recognizePartId(point) != -1) {
                return child->addPoint(point);
            } else {
                bool shouldAdd = false;
                if (linkToMoreDetailed == NULL) {
                    shouldAdd = true;
                } else if (linkToMoreDetailed->addPoint(point) && isEagle()) {
                    shouldAdd = true;
                }
                if (shouldAdd) {

                    Range commonRange = determineCommonRange(point, child, range);
                    std::shared_ptr<MiddleNode> commonNode(new MiddleNode(commonRange));
                    int commonNodePointIndex = commonRange.recognizePartId(point);
                    int commonNodeOldChildIndex = commonRange.recognizePartId(child->range.getMiddlePoint());
                    printf("  nodes located at indexes: %d, %d\n", commonNodePointIndex, commonNodeOldChildIndex);
                    MiddleNode* commonChild = dynamic_cast<MiddleNode*>(children[index].get());
                    commonChild->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(point));
                    commonChild->children[commonNodeOldChildIndex] = std::shared_ptr<Node>(child);
                    if (linkToMoreDetailed != NULL) {
                        commonChild->linkToMoreDetailed = std::dynamic_pointer_cast<MiddleNode, Node>(linkToMoreDetailed->children[index]);
                    }
                    children[index] = commonNode;
                    return true;
                } else {
                    return false;
                }
            }
        } else {
            bool shouldAdd = false;
            if (linkToMoreDetailed == NULL) {
                shouldAdd = true;
            } else if (linkToMoreDetailed->addPoint(point) && isEagle()) {
                shouldAdd = true;
            }
            if (shouldAdd) {
                TermNode *term = (TermNode *) children[index].get();
                Range commonRange = determineCommonRange(point, term->point, range);
                std::shared_ptr<MiddleNode> commonNode(new MiddleNode(commonRange));
                int commonNodePointIndex = commonRange.recognizePartId(point);
                int commonNodeOldTermIndex = commonRange.recognizePartId(term->point);
                commonNode->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(point));
                commonNode->children[commonNodeOldTermIndex] = children[index];
                if (linkToMoreDetailed != NULL) {
                    commonNode->linkToMoreDetailed = std::dynamic_pointer_cast<MiddleNode, Node>(linkToMoreDetailed->children[index]);
                }
                children[index] = commonNode;
                return true;
            } else {
                return false;
            }
        }
    } else {
        bool shouldAdd = false;
        if (linkToMoreDetailed == NULL) {
            shouldAdd = true;
        } else if (linkToMoreDetailed->addPoint(point) && isEagle()) {
            shouldAdd = true;
        }
        if (shouldAdd) {
            children[index] = std::shared_ptr<TermNode>(new TermNode(point));
            return true;
        } else {
            return false;
        }
    }
}
// MiddleNode implementation END