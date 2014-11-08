#include "cg/structures/kirkpatrickLinesByPoint.h"

void test1() {
    Line line0(1, -2, 1);
    Line line1(1, 1, 0);
    Line line2(1, 2, -3);
    Line line3(1.5, -1, -2.5f);
    DCEL dcel(line0);
    dcel.print();
    dcel.addLine(line1);
    dcel.print();
    dcel.addLine(line2);
    dcel.print();
    dcel.addLine(line3);
    dcel.print();
}

void test2() {
    Line line0(1, -2, 1);
    Line line1(1, 1, 0);
    Line line2(1, 2, -3);
    Line line3(1.5, -1, -2.5f);
    DCEL dcel(line0);
    dcel.print();
    dcel.addLine(line1);
    dcel.print();
    dcel.addLine(line2);
    dcel.print();
    dcel.addLine(line3);
    dcel.print();

    DCEL tri = dcel;
    tri.triangulate();
    printf("      TRIANGULATED:   \n\n");
    tri.print();
}

int main() {
    test2();
}