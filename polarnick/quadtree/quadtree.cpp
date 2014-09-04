#include <vector>
#include <fstream>
#include <string>

#include <QColor>
#include <QApplication>

#include "cg/visualization/viewer_adapter.h"
#include "cg/visualization/draw_util.h"

#include <cg/primitives/contour.h>
#include "cg/triangulation/triangulation.h"

#include "cg/io/point.h"

#define MAX_LEVEL 1024
#define MAX_LEVEL_RENDER 5
#define WIDTH 640
#define HEIGHT 480
#define MIN_X (-WIDTH / 2)
#define MAX_X (WIDTH / 2)
#define MIN_Y (-HEIGHT / 2)
#define MAX_Y (HEIGHT / 2)

using cg::point_2;
using cg::point_2f;
using cg::vector_2f;

static int middleNodesCount = 0;

int calcXiByPoint(point_2f point, int lvl)
{
   int res = (point.x - MIN_X) * (1 << lvl) / WIDTH;
   return res;
}

int calcYiByPoint(point_2f point, int lvl)
{
   int res = (point.y - MIN_Y) * (1 << lvl) / HEIGHT;
   return res;
}

struct Node
{
   bool isTerm;

   Node(bool isTerm) : isTerm(isTerm)
   {}

   virtual ~Node() {}
};

struct MiddleNode : Node
{
   int xi;
   int yi;
   int lvl;

   Node* childs[4];

   MiddleNode(int xi, int yi, int lvl) : Node(false), xi(xi), yi(yi), lvl(lvl)
   {
      middleNodesCount++;
      for (int i = 0; i < 4; i++) {
         childs[i] = NULL;
      }
   }

   ~MiddleNode() {
      middleNodesCount--;
      for (int i = 0; i < 4; i++) {
         if (childs[i] != NULL) {
            delete childs[i];
         }
      }
   }

   void addPoint(point_2f point);
};

struct TermNode : Node
{
   point_2f point;
   TermNode(point_2f point) : Node(true), point(point)
   {}
};

int determineCommonLvl(point_2f point, MiddleNode* node, int minLvl) {
   int l = minLvl;
   int r = node->lvl;
   while (l + 1 < r) {
      int m = (l + r) / 2;
      if (calcXiByPoint(point, m) == (node->xi >> (node->lvl - m))
         && calcYiByPoint(point, m) != (node->yi >> (node->lvl - m))) {
         l = m;
      } else {
         r = m;
      }
   }
   return l;
}

int determineCommonLvl(point_2f p1, point_2f p2, int minLvl) {
   int l = minLvl;
   int r = MAX_LEVEL;
   while (l + 1 < r) {
      int m = (l + r) / 2;
      if (calcXiByPoint(p1, m) == calcXiByPoint(p2, m)
         && calcYiByPoint(p1, m) == calcYiByPoint(p2, m)) {
         l = m;
      } else {
         r = m;
      }
   }
   return l;
}

void MiddleNode::addPoint(point_2f point)
{
   printf("Adding point at: xi=%d yi=%d lvl=%d\n", xi, yi, lvl);
   int index;
   {
      int pxi = calcXiByPoint(point, lvl + 1);
      int pyi = calcYiByPoint(point, lvl + 1);
      index = (pxi % 2) * 2 + (pyi % 2);
   }
   if (childs[index] != NULL) {
      if (!childs[index]->isTerm) {
         printf(" (adding at middle node)\n");
         MiddleNode* child = (MiddleNode*) childs[index];
         if (child->xi == calcXiByPoint(point, child->lvl) && child->yi == calcYiByPoint(point, child->lvl)) {
           child->addPoint(point);
         } else {
            int commonLvl = determineCommonLvl(point, child, lvl + 1);
            printf(" commonLvl=%d (with middle node xi=%d yi=%d lvl=%d)\n", commonLvl, child->xi, child->yi, child->lvl);

            MiddleNode* commonNode = new MiddleNode(calcXiByPoint(point, commonLvl), calcYiByPoint(point, commonLvl), commonLvl);
            printf("  new middle node: xi=%d yi=%d lvl=%d\n", commonNode->xi, commonNode->yi, commonNode->lvl);
            childs[index] = commonNode;
            int commonNodePointIndex;
            int commonNodeOldChildIndex;
            {
               int pxi = calcXiByPoint(point, commonLvl + 1);
               int pyi = calcYiByPoint(point, commonLvl + 1);
               commonNodePointIndex = (pxi % 2) * 2 + (pyi % 2);
               pxi = (child->xi) >> (child->lvl - commonLvl - 1);
               pyi = (child->yi) >> (child->lvl - commonLvl - 1);
               commonNodeOldChildIndex = (pxi % 2) * 2 + (pyi % 2);
            }
            printf("  nodes located at indexes: %d, %d\n", commonNodePointIndex, commonNodeOldChildIndex);
            commonNode->childs[commonNodePointIndex] = new TermNode(point);
            commonNode->childs[commonNodeOldChildIndex] = child;
         }
      } else {
         printf(" (adding at terminal node)\n");
         TermNode* term = (TermNode*) childs[index];
         printf("  to terminal child: x=%f y=%f\n", term->point.x, term->point.y);
         int commonLvl = determineCommonLvl(point, term->point, lvl + 1);
         printf("  commonLvl=%d (new: x=%f y=%f)\n", commonLvl, point.x, point.y);
         MiddleNode* commonNode = new MiddleNode(calcXiByPoint(point, commonLvl), calcYiByPoint(point, commonLvl), commonLvl);
         printf("  new middle node: xi=%d yi=%d lvl=%d stepX=%f stepY=%f\n", commonNode->xi, commonNode->yi, commonNode->lvl,
            (WIDTH * 1.0 / (1 << commonNode->lvl)), (HEIGHT * 1.0 / (1 << commonNode->lvl)));
         childs[index] = commonNode;
         int commonNodePointIndex;
         int commonNodeOldTermIndex;
         {
            int pxi = calcXiByPoint(point, commonLvl + 1);
            int pyi = calcYiByPoint(point, commonLvl + 1);
            commonNodePointIndex = (pxi % 2) * 2 + (pyi % 2);
            pxi = calcXiByPoint(term->point, commonLvl + 1);
            pyi = calcYiByPoint(term->point, commonLvl + 1);
            commonNodeOldTermIndex = (pxi % 2) * 2 + (pyi % 2);
         }
         printf("  points located at indexes: %d, %d\n", commonNodePointIndex, commonNodeOldTermIndex);
         commonNode->childs[commonNodePointIndex] = new TermNode(point);
         commonNode->childs[commonNodeOldTermIndex] = term;
      }
   } else {
      printf("New terminal node at child index=%d\n", index);
      childs[index] = new TermNode(point);
   }
}

struct triangulation_viewer : cg::visualization::viewer_adapter
{
   triangulation_viewer()
   {
   }

   void drawNode(const MiddleNode* node, cg::visualization::drawer_type & drawer) const
   {
      float stepX = (MAX_X - MIN_X * 1.0) / (2 << node->lvl);
      float stepY = (MAX_Y - MIN_Y * 1.0) / (2 << node->lvl);
      point_2f nodePoint(MIN_X + stepX * (2 * node->xi + 1), MIN_Y + stepY * (2 * node->yi + 1));
      drawer.set_color(Qt::red);
      drawer.draw_point(nodePoint, 3);//MAX_LEVEL_RENDER + 2);
      for (int i = 0; i < 4; i++) {
         if (node->childs[i] != NULL) {
            if (!node->childs[i]->isTerm) {
               MiddleNode* childNode = (MiddleNode*) node->childs[i];
               stepX = (MAX_X - MIN_X * 1.0) / (2 << childNode->lvl);
               stepY = (MAX_Y - MIN_Y * 1.0) / (2 << childNode->lvl);
               point_2f childNodePoint(MIN_X + stepX * (2 * childNode->xi + 1), MIN_Y + stepY * (2 * childNode->yi + 1));
               drawer.set_color(Qt::gray);
               drawer.draw_line(nodePoint, childNodePoint, 2);
               drawNode(childNode, drawer);
            } else {
               drawer.set_color(Qt::gray);
               drawer.draw_line(nodePoint, ((TermNode*) node->childs[i])->point, 1);
               drawer.set_color(Qt::green);
               drawer.draw_point(((TermNode*) node->childs[i])->point, 2);
            }
         }
      }
   }

   void draw(cg::visualization::drawer_type & drawer) const
   {
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
      drawNode(rootNode, drawer);
   }

   void print(cg::visualization::printer_type & p) const
   {
      p.corner_stream() << "double-click to reset." << cg::visualization::endl
                        << "press mouse rbutton to add vertex (in blue layout)" << cg::visualization::endl
                        << "press l to disable blue layout" << cg::visualization::endl
                        << "press t to trace quad-tree structure in terminal" << cg::visualization::endl;
                        // << "middleNodesCount: " << middleNodesCount << cg::visualization::endl;
   }

   bool on_double_click(const point_2f & p)
   {
      delete rootNode;
      rootNode = new MiddleNode(0, 0, 0);
      return true;
   }

   bool on_press(const point_2f & p)
   {
      printf("Adding point: x=%f y=%f\n", p.x, p.y);
      rootNode->addPoint(p);
      return true;
   }

   bool on_release(const point_2f & p)
   {
      return true;
   }

   bool on_move(const point_2f & p)
   {
      return true;
   }

   void printMiddleNode(MiddleNode* node, int lvl, int index)
   {
      printf("%d\t %d\t xi=%d yi=%d lvl=%d\n", lvl, index, node->xi, node->yi, node->lvl);
      for (int i = 0; i < 4; i++) {
         if (node->childs[i] != NULL) {
            if (!node->childs[i]->isTerm) {
               MiddleNode* childNode = (MiddleNode*) node->childs[i];
               printMiddleNode(childNode, lvl + 1, i);
            } else {
               point_2f point = ((TermNode*) node->childs[i])->point;
               printf("%d\t %d\t x=%f y=%f\n", lvl + 1, i, point.x, point.y);
            }
         }
      }
   }

   bool on_key(int key)
   {
      if (key == Qt::Key_D) {
      } else if (key == Qt::Key_T) {
         printf("_____Quad-tree dump_____\n");
         printMiddleNode(rootNode, 1, -1);
         printf("________________________\n");
      } else if (key == Qt::Key_L) {
         layout = !layout;
      } else return false;
      return true;
   }


private:
   bool layout = true;
   MiddleNode* rootNode = new MiddleNode(0, 0, 0);
};

int main(int argc, char ** argv)
{
   QApplication app(argc, argv);
   triangulation_viewer viewer;
   cg::visualization::run_viewer(&viewer, "quadtree viewer");
}
