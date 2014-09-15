#pragma once

#include <list>
#include <memory>
#include <misc/random_utils.h>

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
            : Range(-239, fromX, toX, fromY, toY) {
    }

    float getMiddleX() const;

    float getMiddleY() const;

    point_2f getMiddlePoint() const;

    Range localize(point_2f point);

    int recognizePartId(point_2f point);

};

std::ostream &operator<<(std::ostream &os, Range const &range);

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

std::ostream &operator<<(std::ostream &os, Node const &node);

struct MiddleNode : Node {
    Range range;
    std::shared_ptr<Node> children[4];
    std::shared_ptr<MiddleNode> linkToMoreDetailed;

    MiddleNode(Range range) : range(range) {
    }

    virtual ~MiddleNode() {
    }

    bool addPoint(point_2f point);

    bool deletePoint(point_2f point, float eps);

    virtual std::list<std::pair<int, point_2f>> getContain(Range range, float eps) override;

    virtual std::list<std::pair<int, point_2f>> getAll() override;
};

struct TermNode : Node {
    point_2f point;

    TermNode(point_2f point) : point(point) {
    }

    virtual ~TermNode() {
    }

    virtual std::list<std::pair<int, point_2f>> getContain(Range range, float eps) override;

    virtual std::list<std::pair<int, point_2f>> getAll() override;
};

struct SkipQuadTree {
    int skipLevels = 1;
    std::shared_ptr<Node> lowDetailedRoot;

    std::list<std::pair<int, point_2f>> getContainWithId(point_2f p1, point_2f p2, float eps);

    std::list<std::pair<int, point_2f>> getContainWithId(Range range, float eps);

    std::list<point_2f> getContain(point_2f p1, point_2f p2, float eps);

    std::list<point_2f> getContain(Range range, float eps);

    bool addPoint(point_2f point);

    bool deletePoint(point_2f point, float eps);
};

//_______________________________________________________IMPLEMENTATION_________________________________________________

using cg::point_2f;
using cg::vector_2f;

static double eagleProbability = 0.5;

util::uniform_random_real<double, std::random_device> probabilityRand(0., 1.);

bool isEagle() {
    return probabilityRand() < eagleProbability;
}

// Range implementation BEGIN
float Range::getMiddleX() const {
    return (fromX + toX) / 2;
}

float Range::getMiddleY() const {
    return (fromY + toY) / 2;
}

point_2f Range::getMiddlePoint() const {
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
    return Range(lvl + 1, resFromX, resToX, resFromY, resToY);
}

//13
//02
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
    os << "id=" << node.id;
    const MiddleNode *middleNode = dynamic_cast<const MiddleNode *>(&node);
    if (middleNode != NULL) {
        os << " " << middleNode->range << " children[";
        for (int i = 0; i < 4; i++) {
            if (middleNode->children[i] == NULL) {
                os << "_";
            } else {
                os << middleNode->children[i]->id;
            }
            if (i != 3) {
                os << ", ";
            }
        }
        os << "]";
        os << " link=";
        if (middleNode->linkToMoreDetailed == NULL) {
            os << "_";
        } else {
            os << middleNode->linkToMoreDetailed->id;
        }
    } else {
        const TermNode *termNode = dynamic_cast<const TermNode *>(&node);
        assert (termNode != NULL);
        os << " " << termNode->point;
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
Range determineCommonRange(point_2f p1, point_2f p2) {
    float half = std::max(std::abs(p2.x - p1.x), std::abs(p2.y - p1.y));
    float middleX = (p2.x + p1.x) / 2;
    float middleY = (p2.y + p1.y) / 2;
    float resFromX = middleX - half;
    float resToX = middleX + half;
    float resFromY = middleY - half;
    float resToY = middleY + half;
    return Range(0, resFromX, resToX, resFromY, resToY);
}

Range determineCommonRange(point_2f point, Range range) {
    float resFromX = range.fromX;
    float resToX = range.toX;
    float resFromY = range.fromY;
    float resToY = range.toY;
    int resLvl = range.lvl;
    while (point.x < resFromX || point.x >= resToX
            || point.y < resFromY || point.y >= resToY) {
        if (point.x >= resToX) {
            resToX = 2 * resToX - resFromX;
        } else {
            resFromX = 2 * resFromX - resToX;
        }
        if (point.y >= resToY) {
            resToY = 2 * resToY - resFromY;
        } else {
            resFromY = 2 * resFromY - resToY;
        }
        resLvl--;
    }
    return Range(resLvl, resFromX, resToX, resFromY, resToY);
}

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
                MiddleNode *child = dynamic_cast<MiddleNode *>(cur->children[i].get());
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
        MiddleNode *child = dynamic_cast<MiddleNode *>(children[index].get());
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
                    commonNode->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(point));
                    commonNode->children[commonNodeOldChildIndex] = children[index];
                    children[index] = commonNode;
                    if (linkToMoreDetailed != NULL) {
                        std::shared_ptr<MiddleNode> moreDetailed = linkToMoreDetailed;
                        point_2f commonRangeMiddlePoint = commonRange.getMiddlePoint();
                        while (moreDetailed->range.lvl != commonRange.lvl) {
                            int commonRangeIdInMoreDetailed = moreDetailed->range.recognizePartId(commonRangeMiddlePoint);
                            moreDetailed = std::dynamic_pointer_cast<MiddleNode, Node>(moreDetailed->children[commonRangeIdInMoreDetailed]);
                            assert (moreDetailed != NULL);
                        }
                        commonNode->linkToMoreDetailed = moreDetailed;
                    }
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
            TermNode *term = (TermNode *) children[index].get();
            if (term->point.x == point.x && term->point.y == point.y) {
                shouldAdd = false;
            }
            if (shouldAdd) {
                Range commonRange = determineCommonRange(point, term->point, range);
                std::shared_ptr<MiddleNode> commonNode(new MiddleNode(commonRange));
                int commonNodePointIndex = commonRange.recognizePartId(point);
                int commonNodeOldTermIndex = commonRange.recognizePartId(term->point);
                commonNode->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(point));
                commonNode->children[commonNodeOldTermIndex] = children[index];
                children[index] = commonNode;
                if (linkToMoreDetailed != NULL) {
                    std::shared_ptr<MiddleNode> moreDetailed = linkToMoreDetailed;
                    point_2f commonRangeMiddlePoint = commonRange.getMiddlePoint();
                    while (moreDetailed->range.lvl != commonRange.lvl) {
                        int commonRangeIdInMoreDetailed = moreDetailed->range.recognizePartId(commonRangeMiddlePoint);
                        moreDetailed = std::dynamic_pointer_cast<MiddleNode, Node>(moreDetailed->children[commonRangeIdInMoreDetailed]);
                        assert (moreDetailed != NULL);
                    }
                    commonNode->linkToMoreDetailed = moreDetailed;
                }
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


bool MiddleNode::deletePoint(point_2f point, float eps) {
    bool wasDeleted = false;
    if (linkToMoreDetailed != NULL) {
        wasDeleted = linkToMoreDetailed->deletePoint(point, eps);
    }
    int index = range.recognizePartId(point);
    if (children[index] != NULL) {
        MiddleNode *child = dynamic_cast<MiddleNode *>(children[index].get());
        if (child != NULL) {
            if (child->range.recognizePartId(point) != -1) {
                if (child->deletePoint(point, eps)) {
                    int countOfChildChildren = 0;
                    std::shared_ptr<Node> childChild;
                    for (int i = 0; i < 4 && countOfChildChildren < 2; i++) {
                        if (child->children[i] != NULL) {
                            countOfChildChildren++;
                            childChild = child->children[i];
                        }
                    }
                    if (countOfChildChildren == 1) {
                        children[index] = childChild;
                    }
                    wasDeleted = true;
                }
            }
        } else {
            TermNode *term = (TermNode *) children[index].get();
            if (term->point.x >= point.x - eps && term->point.x <= point.x + eps
                    && term->point.y >= point.y - eps && term->point.y <= point.y + eps) {
                children[index] = NULL;
                wasDeleted = true;
            }
        }
    }
    return wasDeleted;
}
// MiddleNode implementation END

// SkipQuadTree implementation BEGIN

std::list<std::pair<int, point_2f>> SkipQuadTree::getContainWithId(point_2f p1, point_2f p2, float eps) {
    Range rect(-239,
            std::min(p1.x, p2.x), std::max(p1.x, p2.x),
            std::min(p1.y, p2.y), std::max(p1.y, p2.y));
    return getContainWithId(rect, eps);
}

std::list<std::pair<int, point_2f>> SkipQuadTree::getContainWithId(Range range, float eps) {
    if (lowDetailedRoot != NULL) {
        return lowDetailedRoot->getContain(range, eps);
    } else {
        return {};
    }
}

std::list<point_2f> SkipQuadTree::getContain(point_2f p1, point_2f p2, float eps) {
    Range rect(-239,
            std::min(p1.x, p2.x), std::max(p1.x, p2.x),
            std::min(p1.y, p2.y), std::max(p1.y, p2.y));
    return getContain(rect, eps);
}

std::list<point_2f> SkipQuadTree::getContain(Range range, float eps) {
    std::list<point_2f> res;
    if (lowDetailedRoot != NULL) {
        std::list<std::pair<int, point_2f>> resWithIds = lowDetailedRoot->getContain(range, eps);
        for (auto a : resWithIds) {
            res.push_back(a.second);
        }
    }
    return res;
}

bool SkipQuadTree::addPoint(point_2f p) {
    if (lowDetailedRoot == NULL) {
        lowDetailedRoot = std::shared_ptr<TermNode>(new TermNode(p));
        return true;
    }
    MiddleNode *root = dynamic_cast<MiddleNode *>(lowDetailedRoot.get());
    if (root == NULL) {
        TermNode *oldRoot = dynamic_cast<TermNode *>(lowDetailedRoot.get());
        if (oldRoot->point.x == p.x && oldRoot->point.y == p.y) {
            return true;
        }
        Range commonRange = determineCommonRange(p, oldRoot->point);
        std::shared_ptr<MiddleNode> commonNode(new MiddleNode(commonRange));
        int commonNodePointIndex = commonRange.recognizePartId(p);
        int commonNodeOldPointIndex = commonRange.recognizePartId(oldRoot->point);
        commonNode->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(p));
        commonNode->children[commonNodeOldPointIndex] = lowDetailedRoot;
        lowDetailedRoot = commonNode;
        return true;
    } else {
        if (root->range.recognizePartId(p) == -1) {
            Range commonRange = determineCommonRange(p, root->range);
            MiddleNode *prevRoot = NULL;
            MiddleNode *curRoot = root;
            std::shared_ptr<Node> curRootNode = lowDetailedRoot;
            std::shared_ptr<MiddleNode> prevNewRootNode;
            int commonNodePointIndex = commonRange.recognizePartId(p);
            int commonNodeOldChildIndex = commonRange.recognizePartId(curRoot->range.getMiddlePoint());
            bool lowestSkipLvl = true;
            while (true) {
                std::shared_ptr<MiddleNode> commonNode(new MiddleNode(commonRange));
                commonNode->children[commonNodePointIndex] = std::shared_ptr<Node>(new TermNode(p));
                commonNode->children[commonNodeOldChildIndex] = curRootNode;
                if (lowestSkipLvl) {
                    lowDetailedRoot = commonNode;
                    lowestSkipLvl = false;
                } else {
                    prevRoot->linkToMoreDetailed = std::dynamic_pointer_cast<MiddleNode>(curRootNode);
                    prevNewRootNode->linkToMoreDetailed = commonNode;
                }
                if (curRoot->linkToMoreDetailed != NULL) {
                    prevRoot = curRoot;
                    curRootNode = curRoot->linkToMoreDetailed;
                    curRoot = curRoot->linkToMoreDetailed.get();
                    prevNewRootNode = commonNode;
                } else {
                    break;
                }
            }
            return true;
        } else if (root->addPoint(p) && isEagle()) {
            MiddleNode *newLevel = new MiddleNode(root->range);
            newLevel->addPoint(p);
            newLevel->linkToMoreDetailed = std::dynamic_pointer_cast<MiddleNode>(lowDetailedRoot);
            lowDetailedRoot = std::shared_ptr<MiddleNode>(newLevel);
            skipLevels++;
            return true;
        }
    }
    return true;
}

bool SkipQuadTree::deletePoint(point_2f point, float eps) {
    if (lowDetailedRoot == NULL) {
        return false;
    }
    MiddleNode *root = dynamic_cast<MiddleNode *>(lowDetailedRoot.get());
    if (root == NULL) {
        TermNode *rootP = dynamic_cast<TermNode *>(lowDetailedRoot.get());
        if (rootP->point.x >= point.x - eps && rootP->point.x <= point.x + eps
                && rootP->point.y >= point.y - eps && rootP->point.y <= point.y + eps) {
            lowDetailedRoot = NULL;
            return true;
        } else {
            return false;
        }
    }
    if (root->deletePoint(point, eps)) {
        int rootChilds = 0;
        std::shared_ptr<Node> child;
        for (int i = 0; i < 4; i++) {
            if (root->children[i] != NULL) {
                rootChilds++;
                child = root->children[i];
            }
        }
        if (root->linkToMoreDetailed == NULL) {
            if (rootChilds == 1) {
                lowDetailedRoot = child;
            }
            return true;
        }
        if (rootChilds >= 1) {
            return true;
        } else {
            lowDetailedRoot = root->linkToMoreDetailed;
            skipLevels--;
        }
        return true;
    } else {
        return false;
    }
}
// SkipQuadTree implementation END
