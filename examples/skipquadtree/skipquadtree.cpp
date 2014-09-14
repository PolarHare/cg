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
        MiddleNode *lvlRoot = tree.lowDetailedRoot.get();
        for (int i = 1; i < tree.skipLevels - level; i++) {
            lvlRoot = lvlRoot->linkToMoreDetailed.get();
        }
        traceMiddleNode(lvlRoot, 0);
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

    void drawNodeNum(const MiddleNode *node, cg::visualization::printer_type &p) const {
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

    void drawNode(const MiddleNode *node, cg::visualization::drawer_type &drawer) const {
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
                    drawer.set_color(Qt::green);
                    drawer.draw_point(((TermNode *) node->children[i].get())->point, 2);
                }
            }
        }
    }

    MiddleNode *getRootFromSkipLevel(int viewLevel) const {
        MiddleNode* node = tree.lowDetailedRoot.get();
        for (int i = 1; i < tree.skipLevels - viewLevel; i++) {
            node = node->linkToMoreDetailed.get();
            assert (node != NULL);
        }
        return node;
    }

    void draw(cg::visualization::drawer_type &drawer) const {
        if (showLayout) {
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

        drawNode(getRootFromSkipLevel(viewLevel), drawer);
    }

    void print(cg::visualization::printer_type &p) const {
        p.corner_stream() << "double-click to reset." << cg::visualization::endl
                << "press mouse rbutton to add vertex (in blue layout)" << cg::visualization::endl
                << "press l to disable blue layout" << cg::visualization::endl
                << "press t to trace quad-tree structure in terminal" << cg::visualization::endl
                << "press a/s to change level of quad tree to be represented (level="
                << (viewLevel + 1) << "/" << tree.skipLevels << ")" << cg::visualization::endl
                << "press r to enter points for rectangle selection (after that - two rclicks)" << cg::visualization::endl
                << "press d and click rbutton on existing point to delete it" << cg::visualization::endl
                << "nodesCount: " << nodesCount << cg::visualization::endl
        ;

        drawNodeNum(getRootFromSkipLevel(viewLevel), p);
    }

    bool on_double_click(const point_2f &p) {
        isDeletingMode = false;
        showLayout = true;
        nodeNextId = 1;
        tree = SkipQuadTree(MIN_X, MAX_X, MIN_Y, MAX_Y);
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
            showLayout = !showLayout;
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
    bool showLayout = true;

    SkipQuadTree tree = SkipQuadTree(MIN_X, MAX_X, MIN_Y, MAX_Y);

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