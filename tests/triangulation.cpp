#include <vector>
#include <iostream>
#include <gtest/gtest.h>

#include "cg/triangulation/triangulation.h"
#include "cg/operations/contains/triangle_point.h"
#include "cg/operations/contains/segment_point.h"
#include "cg/operations/contains/contour_point.h"

using namespace std;
using namespace cg;

typedef vector<contour_2> polygon;

bool inner_intersection(const segment_2 & a, const segment_2 &b) {
    if (a[0] == a[1]) 
        return false;

    orientation_t ab[2];
    for (size_t l = 0; l != 2; ++l) 
        ab[l] = orientation(a[0], a[1], b[l]);

    if (ab[0] == CG_COLLINEAR || ab[1] == CG_COLLINEAR) 
        return false;
    if (ab[0] == ab[1]) return false;
    for (size_t l = 0; l != 2; l++) 
        ab[l] = orientation(b[0], b[1], a[l]);
    if (ab[0] == CG_COLLINEAR || ab[1] == CG_COLLINEAR) 
        return false;
    return ab[0] != ab[1];
}

bool inner_intersection(const segment_2 & a, const triangle_2 &t) {
    for (size_t l = 0; l != 3; l++) 
        if (inner_intersection(a, t.side(l))) return true;
    for (int i = 0; i < 2; i++) {
        auto p = a[i];
        bool in = true;
        if (!contains(t, p)) in = false;
        for (int j = 0; j < 3; j++) {
            if (contains(t.side(j), p)) in = false;
        }
        if (in) return true;
    }
    return false;
}

bool contains(const point_2 &p, polygon &poly) {
    if (!contains(poly[0], p)) return false;
    for (int i = 1; i < poly.size(); i++) {
        if (contains(poly[i], p)) return false;
    }
    return true;
}

bool check_triangulation(polygon poly, vector<triangle_2> t) {
    //normal triangles
    size_t count_v = 0;
    for (auto cont : poly) count_v += cont.vertices_num();
    EXPECT_TRUE(t.size() == count_v + 2 * (poly.size() - 2)); 
    for (auto tr : t) {
        EXPECT_FALSE(orientation(tr[0], tr[1], tr[2]) == CG_COLLINEAR);
    }
    //which not intersect
    //(intersections with touches allowed
    for (int i = 0; i < t.size(); i++) {
        for (int j = i + 1; j < t.size(); j++) {
            auto t1 = t[i], t2 = t[j];
            for (int k = 0; k < 3; k++) {
                auto s = t1.side(k);
                EXPECT_FALSE(inner_intersection(s, t2));
            }
        }
    }
    //and not intersect border
    for (auto cont : poly) {
        auto c = cont.circulator();
        auto st = c;
        do {
            for (auto tr : t) 
                EXPECT_FALSE(inner_intersection(segment_2(*c, *(c + 1)), tr));
        } while (++c != st); 
    }
    //and fully contains in polygon
    for (auto tr : t) {
        point_2 p((tr[0].x + tr[1].x + tr[2].x) / 3, (tr[0].y + tr[1].y + tr[2].y) / 3);
        EXPECT_TRUE(contains(p, poly));
    }
    //and feel it completely
    return true;
}

TEST(triangulation, simple_test) {
    contour_2 outer({ point_2(-1, -1), point_2(1, -1), point_2(0, 1), point_2(-1, 1) });
    auto poly = {outer};
    vector<triangle_2> v = triangulate(poly);
    check_triangulation(poly, v);
}


TEST(triangulation, init_test) {
    contour_2 outer({ point_2(-2, -2), point_2(2, -2), point_2(2, 2), point_2(-2, 2) });
    contour_2 hole({ point_2(1, 1), point_2(1, -1), point_2(-1, -1), point_2(-1, 1) });
    auto poly = {outer, hole};
    vector<triangle_2> v = triangulate(poly);
    check_triangulation(poly, v);
}

int main(int argc, char **argv) {
    cout << "Start testing [monotone triangulation]" << endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
