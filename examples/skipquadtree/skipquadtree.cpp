#include <cg/visualization/viewer_adapter.h>
#include <QtGui/qapplication.h>
#include <cg/structures/skipquadtree.h>

#define MAX_LEVEL_RENDER 5
#define WIDTH 640
#define HEIGHT 480
#define MIN_X (-WIDTH / 2)
#define MAX_X (WIDTH / 2)
#define MIN_Y (-HEIGHT / 2)
#define MAX_Y (HEIGHT / 2)

struct triangulation_viewer : cg::visualization::viewer_adapter {

    void printfDumpOfSkipLayer(int level) {
        printf("Skip-Quadtree dump (skip-level=%d)\n", level + 1);
        if (tree.lowDetailedRoot == NULL) {
            printf("Empty\n");
        } else {
            MiddleNode *lvlRoot = dynamic_cast<MiddleNode *>(tree.lowDetailedRoot.get());
            if (lvlRoot != NULL) {
                for (int i = 1; i < tree.skipLevels - level; i++) {
                    lvlRoot = lvlRoot->linkToMoreDetailed.get();
                }
                traceMiddleNode(lvlRoot, 0);
            } else {
                TermNode *termNode = dynamic_cast<TermNode *>(tree.lowDetailedRoot.get());
                std::cout << "stackLevel=" << 0 << "\t " << *termNode << std::endl;
            }
        }
        printf("____________________________________________\n");
    }

    triangulation_viewer() {
        tree.addPoint(point_2f(127, 24));
        tree.addPoint(point_2f(213, 36));
        tree.addPoint(point_2f(312, 114));
        tree.addPoint(point_2f(312, 100));
        tree.addPoint(point_2f(212, 202));
        tree.addPoint(point_2f(53, 199));
        tree.addPoint(point_2f(64, 200));
        tree.addPoint(point_2f(231, -188));
        tree.addPoint(point_2f(249, -175));
        tree.addPoint(point_2f(-334, -23));
        tree.addPoint(point_2f(-306, -6));
        tree.addPoint(point_2f(-289, -99));
        for (int i = 0; i < tree.skipLevels; i++) {
            printfDumpOfSkipLayer(i);
        }
    }

    void drawNodeNum(const Node *n, cg::visualization::printer_type &p) const {
        if (n == NULL) {
            return;
        }
        const MiddleNode *node = dynamic_cast<const MiddleNode *>(n);
        if (node == NULL) {
            p.global_stream(((TermNode *) n)->point) << n->id;
            return;
        }
        p.global_stream(node->range.getMiddlePoint()) << node->id;
        for (int i = 0; i < 4; i++) {
            if (node->children[i] != NULL) {
                MiddleNode *child = dynamic_cast<MiddleNode *>(node->children[i].get());
                if (child != NULL) {
                    drawNodeNum(child, p);
                } else {
                    p.global_stream(((TermNode *) node->children[i].get())->point) << node->children[i]->id;
                }
            }
        }
    }

    void drawNode(TermNode *node, cg::visualization::drawer_type &drawer) const {
        drawer.set_color(Qt::green);
        drawer.draw_point(node->point, 2);
    }

    void drawNode(const Node *n, cg::visualization::drawer_type &drawer) const {
        if (n == NULL) {
            return;
        }
        const MiddleNode *node = dynamic_cast<const MiddleNode *>(n);
        if (node == NULL) {
            drawNode(dynamic_cast<const TermNode *>(node), drawer);
            return;
        }
        drawer.set_color(Qt::red);
        drawer.draw_point(node->range.getMiddlePoint(), 3);//MAX_LEVEL_RENDER + 2);
        for (int i = 0; i < 4; i++) {
            if (node->children[i] != NULL) {
                MiddleNode *childNode = dynamic_cast<MiddleNode *>(node->children[i].get());
                if (childNode != NULL) {
                    drawer.set_color(Qt::gray);
                    drawer.draw_line(node->range.getMiddlePoint(), childNode->range.getMiddlePoint(), 2);
                    drawNode(childNode, drawer);
                } else {
                    drawer.set_color(Qt::gray);
                    drawer.draw_line(node->range.getMiddlePoint(), ((TermNode *) node->children[i].get())->point, 1);
                    drawNode((TermNode *) node->children[i].get(), drawer);
                }
            }
        }
    }

    Node *getRootFromSkipLevel(int viewLevel) const {
        MiddleNode *node = dynamic_cast<MiddleNode *>(tree.lowDetailedRoot.get());
        if (node != NULL) {
            for (int i = 1; i < tree.skipLevels - viewLevel; i++) {
                node = node->linkToMoreDetailed.get();
                assert (node != NULL);
            }
            return node;
        } else {
            return tree.lowDetailedRoot.get();
        }
    }

    void drawRectangles(MiddleNode *node, cg::visualization::drawer_type &drawer) const {
        Range r = node->range;
        point_2f a(r.fromX, r.fromY);
        point_2f b(r.fromX, r.toY);
        point_2f c(r.toX, r.toY);
        point_2f d(r.toX, r.fromY);
        drawer.set_color(Qt::blue);
        drawer.draw_line(a, b, 1);
        drawer.draw_line(b, c, 1);
        drawer.draw_line(c, d, 1);
        drawer.draw_line(d, a, 1);

        float middleX = (r.fromX + r.toX) / 2;
        float middleY = (r.fromY + r.toY) / 2;
        drawer.set_color(Qt::darkBlue);
        drawer.draw_line(point_2f(middleX, r.fromY), point_2f(middleX, r.toY));
        drawer.draw_line(point_2f(r.fromX, middleY), point_2f(r.toX, middleY));
        for (auto child : node->children) {
            MiddleNode *childMiddle = dynamic_cast<MiddleNode *>(child.get());
            if (childMiddle != NULL) {
                drawRectangles(childMiddle, drawer);
            }
        }
    }

    void draw(cg::visualization::drawer_type &drawer) const {
        if (layoutMode == 0) {
            MiddleNode *node = dynamic_cast<MiddleNode *>(getRootFromSkipLevel(viewLevel));
            if (node != NULL) {
                drawRectangles(node, drawer);
            }
        }

        drawNode(getRootFromSkipLevel(viewLevel), drawer);
    }

    void print(cg::visualization::printer_type &p) const {
        p.corner_stream() << "press DOUBLE-CLICK to clear" << cg::visualization::endl
                << "press mouse RBUTTON to add vertex" << cg::visualization::endl
                << "press L to change blue layout mode (per node/disabled)" << cg::visualization::endl
                << "press T to trace quad-tree structure in terminal" << cg::visualization::endl
                << "press A/S to change level of quad tree to be represented (level="
                << (viewLevel + 1) << "/" << tree.skipLevels << ")" << cg::visualization::endl
                << "press R to enter points for rectangle selection (after that - two RBUTTONS)" << cg::visualization::endl
                << "press D and click RBUTTON on existing point to delete it" << cg::visualization::endl
                << "nodesCount: " << nodesCount << cg::visualization::endl;

        drawNodeNum(getRootFromSkipLevel(viewLevel), p);
    }

    bool on_double_click(const point_2f &p) {
        isDeletingMode = false;
        layoutMode = 0;
        nodeNextId = 1;
        tree = SkipQuadTree();
        viewLevel = 0;
        pointsCountToEnter = 0;
        return true;
    }

    bool on_press(const point_2f &p) {
        if (pointsCountToEnter > 0) {
            pointsCountToEnter--;
            pointsOfRect[pointsCountToEnter] = p;
            if (pointsCountToEnter == 1) {
                printf("One point of rectangle:    x=%f y=%f\n", p.x, p.y);
            }
            if (pointsCountToEnter == 0) {
                printf("Secont point of rectangle: x=%f y=%f\n", p.x, p.y);
                auto points = tree.getContainWithId(pointsOfRect[0], pointsOfRect[1], 0.1);
                std::cout << "In rect [" << pointsOfRect[0] << "; " << pointsOfRect[1] << "] points found: " << points.size() << std::endl;
                for (std::pair<int, point_2f> pointWithId : points) {
                    std::cout << " id=" << pointWithId.first << "\t" << pointWithId.second << std::endl;
                }
            }
            return true;
        } else if (isDeletingMode) {
            float eps = 5;
            printf("Deleting one point with range=%f from click={x=%f y=%f}...\n", eps, p.x, p.y);
            if (tree.deletePoint(p, eps)) {
                printf(" Point deleted.\n");
                if (viewLevel == tree.skipLevels) {
                    viewLevel = tree.skipLevels - 1;
                }
            } else {
                printf(" No point was found.\n");
            }
            isDeletingMode = false;
            return true;
        } else {
            printf("Adding point: x=%f y=%f.\n", p.x, p.y);
            if (!tree.addPoint(p)) {
                printf(" Point was not added, because point is out of range!\n");
            }
            return true;
        }
    }

    void traceMiddleNode(MiddleNode *node, int stackLevel) {
        std::cout << "stackLevel=" << stackLevel << "\t " << *node << std::endl;
        for (auto child : node->children) {
            if (child == NULL) {
                continue;
            }
            MiddleNode *middleChild = dynamic_cast<MiddleNode *>(child.get());
            if (middleChild != NULL) {
                traceMiddleNode(middleChild, stackLevel + 1);
            } else {
                std::cout << "stackLevel=" << stackLevel + 1 << "\t " << *((TermNode *) child.get()) << std::endl;
            }
        }
    }

    bool on_key(int key) {
        if (key == Qt::Key_D) {
            printf("Now choose existing point for deletion. (you can choose point by rbutton)\n");
            isDeletingMode = true;
        } else if (key == Qt::Key_T) {
            printfDumpOfSkipLayer(viewLevel);
        } else if (key == Qt::Key_L) {
            layoutMode = (layoutMode + 1) % 2;
        } else if (key == Qt::Key_R) {
            printf("Now choose two points of rectangle. (you can choose point by rbutton)\n");
            pointsCountToEnter = 2;
        } else if (key == Qt::Key_S) {
            viewLevel = (viewLevel + 1) % tree.skipLevels;
        } else if (key == Qt::Key_A) {
            viewLevel = (viewLevel - 1 + tree.skipLevels) % tree.skipLevels;
        } else return false;
        return true;
    }


private:
    int layoutMode = 0;

    SkipQuadTree tree;

    int viewLevel = 0;

    int pointsCountToEnter = 0;
    point_2f pointsOfRect[2];

    bool isDeletingMode = false;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    triangulation_viewer viewer;
    cg::visualization::run_viewer(&viewer, "skipquadtree viewer");
    return 0;
}