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

int calcXiByPoint(point_2f point, int lvl)
{
   return (point.x - MIN_X) * (1 << lvl) / WIDTH;
}

int calcYiByPoint(point_2f point, int lvl)
{
   return (point.y - MIN_Y) * (1 << lvl) / HEIGHT;
}

struct Node
{
   bool isTerm;

   Node(bool isTerm) : isTerm(isTerm)
   {}
};

struct MiddleNode : Node
{
   int xi;
   int yi;
   int lvl;

   Node* childs[4];

   MiddleNode(int xi, int yi, int lvl) : Node(false), xi(xi), yi(yi), lvl(lvl)
   {
      for (int i = 0; i < 4; i++) {
         childs[i] = NULL;
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

int determineCommonLvl(point_2f point, MiddleNode* node) {
   int curLvl = node->lvl - 1;
   int curXi = node->xi / 2;
   int curYi = node->yi / 2;
   while (calcXiByPoint(point, curLvl) != curXi
      || calcYiByPoint(point, curLvl) != curYi) {
      curLvl--;
      curXi /= 2;
      curYi /= 2;
   }
   return curLvl;
}

int determineCommonLvl(point_2f p1, point_2f p2) {
   int lvl = 0;
   while (calcXiByPoint(p1, lvl) == calcXiByPoint(p2, lvl)
      && calcYiByPoint(p1, lvl) == calcYiByPoint(p2, lvl)) {
      lvl++;
   }
   return lvl - 1;
}

void MiddleNode::addPoint(point_2f point)
{
   printf("Add point to: xi=%d yi=%d lvl=%d\n", xi, yi, lvl);
   int index;
   {
      int pxi = calcXiByPoint(point, lvl + 1);
      int pyi = calcYiByPoint(point, lvl + 1);
      index = (pxi % 2) * 2 + (pyi % 2);
   }
   printf(" to child %d\n", index);
   if (childs[index] != NULL) {
      if (!childs[index]->isTerm) {
         MiddleNode* child = (MiddleNode*) childs[index];
         if (child->xi == calcXiByPoint(point, child->lvl) && child->yi == calcYiByPoint(point, child->lvl)) {
           child->addPoint(point);
         } else {
            int commonLvl = determineCommonLvl(point, child);
            printf(" commonLvl=%d (with xi=%d yi=%d lvl=%d)\n", commonLvl, child->xi, child->yi, child->lvl);

            MiddleNode* commonNode = new MiddleNode(calcXiByPoint(point, commonLvl), calcYiByPoint(point, commonLvl), commonLvl);
            childs[index] = commonNode;
            int commonNodePointIndex;
            int commonNodeOldChildIndex;
            {
               int pxi = calcXiByPoint(point, commonLvl + 1);
               int pyi = calcYiByPoint(point, commonLvl + 1);
               commonNodePointIndex = (pxi % 2) * 2 + (pyi % 2);
               pxi = (child->xi) >> (child->lvl - commonLvl);
               pyi = (child->xi) >> (child->lvl - commonLvl);
               commonNodeOldChildIndex = (pxi % 2) * 2 + (pyi % 2);
            }
            printf("  nodes located at indexes: %d, %d\n" + commonNodePointIndex, commonNodeOldChildIndex);
            commonNode->childs[commonNodePointIndex] = new TermNode(point);
            commonNode->childs[commonNodeOldChildIndex] = child;
         }
      } else {
         printf(" child is terminal node\n");
         TermNode* term = (TermNode*) childs[index];
         printf(" to terminal child: x=%f y=%f\n", term->point.x, term->point.y);
         int commonLvl = determineCommonLvl(point, term->point);
         printf(" commonLvl=%d (new: x=%f y=%f)\n", commonLvl, point.x, point.y);
         MiddleNode* commonNode = new MiddleNode(calcXiByPoint(point, commonLvl), calcYiByPoint(point, commonLvl), commonLvl);
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
         printf("  points located at indexes: %d, %d\n" + commonNodePointIndex, commonNodeOldTermIndex);
         commonNode->childs[commonNodePointIndex] = new TermNode(point);
         commonNode->childs[commonNodeOldTermIndex] = term;
      }
   } else {
      printf(" new terminal node\n");
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
      float stepX = (MAX_X - MIN_X) / (2 << node->lvl);
      float stepY = (MAX_Y - MIN_Y) / (2 << node->lvl);
      point_2f nodePoint(MIN_X + stepX * (2 * node->xi + 1), MIN_Y + stepY * (2 * node->yi + 1));
      drawer.set_color(Qt::red);
      drawer.draw_point(nodePoint, MAX_LEVEL_RENDER + 2);
      for (int i = 0; i < 4; i++) {
         if (node->childs[i] != NULL) {
            if (!node->childs[i]->isTerm) {
               MiddleNode* childNode = (MiddleNode*) node->childs[i];
               stepX = (MAX_X - MIN_X) / (2 << childNode->lvl);
               stepY = (MAX_Y - MIN_Y) / (2 << childNode->lvl);
               point_2f childNodePoint(MIN_X + stepX * (2 * childNode->xi + 1), MIN_Y + stepY * (2 * childNode->yi + 1));
               drawer.set_color(Qt::gray);
               drawer.draw_line(nodePoint, childNodePoint, 2);
               drawNode(childNode, drawer);
            } else {
               drawer.set_color(Qt::gray);
               drawer.draw_line(nodePoint, ((TermNode*) node->childs[i])->point, 2);
               drawer.set_color(Qt::green);
               drawer.draw_point(((TermNode*) node->childs[i])->point, 3);
            }
         }
      }
   }

   void draw(cg::visualization::drawer_type & drawer) const
   {
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
      drawNode(&rootNode, drawer);
   }

   void print(cg::visualization::printer_type & p) const
   {
      p.corner_stream() << "double-click to reset." << cg::visualization::endl
                        << "press mouse rbutton to add vertex." << cg::visualization::endl
                        << "press d to switch debug level" << cg::visualization::endl
                        << "press t to trace quad-tree structure in terminal" << cg::visualization::endl;
   }

   bool on_double_click(const point_2f & p)
   {
      rootNode.addPoint(p);
      return true;
   }

   bool on_press(const point_2f & p)
   {
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

   bool on_key(int key)
   {
      if (key == Qt::Key_D) {
      } else if (key == Qt::Key_T) {
         // if (in_building_) return false;
         // std::ofstream out("../../tests/tests/" + std::to_string(current_test++));
         // out << poly.size() << std::endl;
         // for (auto cont : poly) {
         //    out << cont.size() << std::endl;
         //    for (size_t i = 0; i < cont.size(); i++) {
         //       out << cont[i].x << " " << cont[i].y << std::endl;
         //    }
         // }
      } else return false;
      return true;
   }


private:
   int debugLevel = 0;
   MiddleNode rootNode = MiddleNode(0, 0, 0);
};

int main(int argc, char ** argv)
{
   QApplication app(argc, argv);
   triangulation_viewer viewer;
   cg::visualization::run_viewer(&viewer, "quadtree viewer");
}
