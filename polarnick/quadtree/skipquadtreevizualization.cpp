#include <cg/visualization/viewer_adapter.h>
#include <QtGui/qapplication.h>
#include "skipquadtree.hpp"

#define MAX_LEVEL_RENDER 5
#define WIDTH 640
#define HEIGHT 480
#define MIN_X (-WIDTH / 2)
#define MAX_X (WIDTH / 2)
#define MIN_Y (-HEIGHT / 2)
#define MAX_Y (HEIGHT / 2)

struct triangulation_viewer : cg::visualization::viewer_adapter {
    triangulation_viewer() {
    }

    void drawNodeNum(const MiddleNode *node, cg::visualization::printer_type &p) const {
        point_2f nodePoint((node->range.fromX + node->range.toX) / 2, (node->range.fromY + node->range.toY) / 2);
        p.global_stream(nodePoint) << node->id;
        for (int i = 0; i < 4; i++) {
            if (node->children[i] != NULL) {
                MiddleNode* child = dynamic_cast<MiddleNode*>(node->children[i].get());
                if (child != NULL) {
                    drawNodeNum(child, p);
                } else {
                    p.global_stream(((TermNode *) node->children[i].get())->point) << node->children[i]->id;
                }
            }
        }
    }

    void drawNode(const MiddleNode *node, cg::visualization::drawer_type &drawer) const {
        point_2f nodePoint((node->range.fromX + node->range.toX) / 2, (node->range.fromY + node->range.toY) / 2);
        drawer.set_color(Qt::red);
        drawer.draw_point(nodePoint, 3);//MAX_LEVEL_RENDER + 2);
        for (int i = 0; i < 4; i++) {
            if (node->children[i] != NULL) {
                MiddleNode* childNode = dynamic_cast<MiddleNode*>(node->children[i].get());
                if (childNode != NULL) {
                    point_2f childNodePoint((childNode->range.fromX + childNode->range.toX) / 2, (childNode->range.fromY + childNode->range.toY) / 2);
                    drawer.set_color(Qt::gray);
                    drawer.draw_line(nodePoint, childNodePoint, 2);
                    drawNode(childNode, drawer);
                } else {
                    drawer.set_color(Qt::gray);
                    drawer.draw_line(nodePoint, ((TermNode *) node->children[i].get())->point, 1);
                    drawer.set_color(Qt::green);
                    drawer.draw_point(((TermNode *) node->children[i].get())->point, 2);
                }
            }
        }
    }

    void draw(cg::visualization::drawer_type &drawer) const {
        if (layout) {
            drawer.set_color(Qt::darkBlue);
            for (int lvl = MAX_LEVEL_RENDER - 1; lvl >= 0; lvl--) {
                float stepX = (MAX_X - MIN_X) / (2 << lvl);
                float stepY = (MAX_Y - MIN_Y) / (2 << lvl);
                int thickness = MAX_LEVEL_RENDER - lvl + 1;
                for (int xi = 1; xi < 2 << lvl; xi++) {
                    drawer.draw_line(point_2f(MIN_X + xi * stepX, MIN_Y), point_2f(MIN_X + xi * stepX, MAX_Y), thickness);
                }
                for (int yi = 1; yi < 2 << lvl; yi++) {
                    drawer.draw_line(point_2f(MIN_X, MIN_Y + yi * stepY), point_2f(MAX_X, MIN_Y + yi * stepY), thickness);
                }
            }
        }
        MiddleNode *lvlRoot = lowDetailRootNode;
        for (int i = 1; i < totalLevels - viewLevel; i++) {
            lvlRoot = &(*lvlRoot->linkToMoreDetailed);
        }
        drawNode(lvlRoot, drawer);
    }

    void print(cg::visualization::printer_type &p) const {
        p.corner_stream() << "double-click to reset." << cg::visualization::endl
                << "press mouse rbutton to add vertex (in blue layout)" << cg::visualization::endl
                << "press l to disable blue layout" << cg::visualization::endl
                << "press t to trace quad-tree structure in terminal" << cg::visualization::endl
                << "press a/s to change level of quad tree to be represented (level="
                << (viewLevel + 1) << "/" << totalLevels << ")" << cg::visualization::endl
                << "press r to enter points for rectangle selection" << cg::visualization::endl;
        // << "middleNodesCount: " << middleNodesCount << cg::visualization::endl;

        MiddleNode *lvlRoot = lowDetailRootNode;
        for (int i = 1; i < totalLevels - viewLevel; i++) {
            lvlRoot = lvlRoot->linkToMoreDetailed.get();
        }
        drawNodeNum(lvlRoot, p);
    }

    bool on_double_click(const point_2f &p) {
        MiddleNode *curLevel = lowDetailRootNode;
        while (curLevel->linkToMoreDetailed != NULL) {
            MiddleNode *next = curLevel->linkToMoreDetailed.get();
            delete curLevel;
            curLevel = next;
        }
        delete curLevel;

        viewLevel = 0;
        totalLevels = 1;
        countToEnter = 0;
        lowDetailRootNode = new MiddleNode(Range(0, MIN_X, MAX_X, MIN_Y, MAX_Y));
        return true;
    }

    bool on_press(const point_2f &p) {
        if (countToEnter > 0) {
            countToEnter--;
            xs[countToEnter] = p.x;
            ys[countToEnter] = p.y;
            if (countToEnter == 0) {
                lowDetailRootNode->getContain(Range(-1, std::min(xs[0], xs[1]), std::max(xs[0], xs[1]), std::min(ys[0], ys[1]), std::max(ys[0], ys[1])), 0.001);
            }
            return true;
        }

        printf("Adding point: x=%f y=%f\n", p.x, p.y);
        if (lowDetailRootNode->addPoint(p) && isEagle()) {
            printf("New skip-level added\n");
            totalLevels++;
            MiddleNode *newLevel = new MiddleNode(Range(0, MIN_X, MAX_X, MIN_Y, MAX_Y));
            newLevel->addPoint(p);
            newLevel->linkToMoreDetailed = std::unique_ptr<MiddleNode>(lowDetailRootNode);
            lowDetailRootNode = newLevel;
        }
        return true;
    }

    bool on_release(const point_2f &p) {
        return true;
    }

    bool on_move(const point_2f &p) {
        return true;
    }

    void printMiddleNode(MiddleNode *node, int lvl, int index) {
        for (int i = 0; i < 4; i++) {
            if (node->children[i] != NULL) {
                MiddleNode *childNode = dynamic_cast<MiddleNode*>(node->children[i].get());
                if (childNode != NULL) {
                    printMiddleNode(childNode, lvl + 1, i);
                } else {
                    point_2f point = ((TermNode *) node->children[i].get())->point;
                    printf("id=%d\t %d\t %d\t x=%f y=%f\n", node->id, lvl + 1, i, point.x, point.y);
                }
            }
        }
    }

    bool on_key(int key) {
        if (key == Qt::Key_D) {
        } else if (key == Qt::Key_T) {
            printf("_____Quad-tree dump (skip-level=%d)_____\n", viewLevel + 1);
            MiddleNode *lvlRoot = lowDetailRootNode;
            for (int i = 1; i < totalLevels - viewLevel; i++) {
                lvlRoot = lvlRoot->linkToMoreDetailed.get();
            }
            printMiddleNode(lvlRoot, 1, -1);
            printf("________________________________________\n");
        } else if (key == Qt::Key_L) {
            layout = !layout;
        } else if (key == Qt::Key_R) {
            countToEnter = 2;
        } else if (key == Qt::Key_S) {
            viewLevel = (viewLevel + 1) % totalLevels;
        } else if (key == Qt::Key_A) {
            viewLevel = (viewLevel - 1 + totalLevels) % totalLevels;
        } else return false;
        return true;
    }


private:
    int viewLevel = 0;
    int totalLevels = 1;
    bool layout = true;
    int countToEnter = 0;
    float xs[2];
    float ys[2];
    MiddleNode *lowDetailRootNode = new MiddleNode(Range(0, MIN_X, MAX_X, MIN_Y, MAX_Y));
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    triangulation_viewer viewer;
    cg::visualization::run_viewer(&viewer, "skipquadtree viewer");
}