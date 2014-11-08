#pragma once

//#include <memory>
#include <vector>
#include <math.h>
#include <stdio.h>

#include "cg/primitives/point.h"
#include "cg/operations/orientation.h"

using std::vector;
using std::nan;
using std::cout;
using std::endl;

using cg::orientation_t;
using cg::orientation_r;
using cg::point_2f;

//typedef std::shared_ptr<Vertex> VertexPtr;
//typedef std::shared_ptr<Edge> EdgePtr;
//typedef std::shared_ptr<Line> LinePtr;

struct Vertex;
struct Edge;
struct Line;

struct Vertex {
    int id;
    int outEid;
    point_2f point;
    int lineId1;
    int lineId2;

    Vertex(int id, int outEid, cg::point_2t<float> const &point, int lineId1, int lineId2)
            : id(id), outEid(outEid), point(point), lineId1(lineId1), lineId2(lineId2) {
    }
};

struct Edge {
    int id;
    int fromVid;
    int nextEid;
    int prevEid;
    int twinEid;
    int lineId;

    Edge(int id, int fromVid, int nextEid, int prevEid, int twinEid, int lineId)
            : id(id), fromVid(fromVid), nextEid(nextEid), prevEid(prevEid), twinEid(twinEid), lineId(lineId) {
    }
};

// a * x + b * y + c = 0
struct Line {
    int id;
    int twinLid;
    float a;
    float b;
    float c;

    Line(float a, float b, float c) : id(-1), twinLid(-1), a(a), b(b), c(c) {
    }
};

struct DCEL {
    const int infinityVid = 0;
    vector<Vertex> vertexes;
    vector<Edge> edges;
    vector<Line> lines;
    int nextEdgeId = 0;
    int nextLineId = 0;
    int nextVertexId = 1;

    DCEL(Line line) {
        Line twinLine(-line.a, -line.b, -line.c);
        line.id = getNextLineId();
        twinLine.id = getNextLineId();
        line.twinLid = twinLine.id;
        twinLine.twinLid = line.id;

        int e1Id = getNextEdgeId();
        int e2Id = getNextEdgeId();
        Edge e1(e1Id, infinityVid, e1Id, e1Id, e2Id, line.id);
        Edge e2(e2Id, infinityVid, e2Id, e2Id, e1Id, twinLine.id);

        Vertex fictiveV(infinityVid, 0, point_2f(nan("FictiveV"), nan("FictiveV")), -1, -1);

        vertexes.push_back(fictiveV);

        edges.push_back(e1);
        edges.push_back(e2);

        lines.push_back(line);
        lines.push_back(twinLine);
    }

    bool isLineBetween(Line line, Line toRight, Line toLeft) {
        point_2f zero(0, 0);
        point_2f lineN(line.a, line.b);
        point_2f a(toRight.a, toRight.b);
        point_2f b(toLeft.a, toLeft.b);
        return orientation_r()(zero, a, lineN) == orientation_t::CG_LEFT
                && orientation_r()(zero, b, lineN) == orientation_t::CG_RIGHT;
    }

    bool isIntersect(Edge edge, Line line) {//TODO
        if (edge.fromVid == infinityVid) {
            if (edges[edge.nextEid].fromVid == infinityVid) {
                return true;
            }
            point_2f b = vertexes[edges[edge.nextEid].fromVid].point;
            float signB = line.a * b.x + line.b * b.y + line.c;
            return signB < 0;
        }
        if (edges[edge.nextEid].fromVid == infinityVid) {
            point_2f a = vertexes[edge.fromVid].point;
            float signA = line.a * a.x + line.b * a.y + line.c;
            return signA > 0;
        }

        point_2f a = vertexes[edge.fromVid].point;
        point_2f b = vertexes[edges[edge.nextEid].fromVid].point;
        float signA = line.a * a.x + line.b * a.y + line.c;
        float signB = line.a * b.x + line.b * b.y + line.c;
        return signA * signB < 0;
    }

    point_2f intersect(Edge edge, Line line) {//TODO
        Line l1 = lines[edge.lineId];
        Line l2 = line;
        return point_2f((l1.b * l2.c - l1.c * l2.b) / (l1.a * l2.b - l1.b * l2.a),
                (l1.a * l2.c - l1.c * l2.a) / (l1.b * l2.a - l1.a * l2.b));
    }

    bool fromRight(Line line, Line line2) {
        float a1 = line.a;
        float b1 = line.b;
        float a2 = line2.a;
        float b2 = line2.b;
        return a1 * b2 < b1 * a2;
    }

    int addLine(Line line) {
        Line twinLine(-line.a, -line.b, -line.c);
        line.id = getNextLineId();
        twinLine.id = getNextLineId();
        line.twinLid = twinLine.id;
        twinLine.twinLid = line.id;

        lines.push_back(line);
        lines.push_back(twinLine);

        Edge edgeLine1 = edges[vertexes[infinityVid].outEid];
        while (true) {
            Edge edgeLine2 = edges[edges[edgeLine1.twinEid].nextEid];
            if (isLineBetween(line, lines[edgeLine1.lineId], lines[edgeLine2.lineId])) {
                break;
            }
            edgeLine1 = edgeLine2;
        }
        int e1 = getNextEdgeId();
        int e2 = getNextEdgeId();

        Edge curEdge = edges[edges[edgeLine1.twinEid].nextEid];

        Edge curLineEdge(e1, infinityVid, -1, edgeLine1.twinEid, e2, line.id);
        Edge curLineEdgeTwin(e2, -1, edges[edgeLine1.twinEid].nextEid, -1, e1, twinLine.id);
        edges.push_back(curLineEdge);
        edges.push_back(curLineEdgeTwin);
        edges[edges[edgeLine1.twinEid].nextEid].prevEid = curLineEdgeTwin.id;
        edges[edgeLine1.twinEid].nextEid = curLineEdge.id;

        while (true) {
            if (isIntersect(curEdge, line)) {
                point_2f p = intersect(curEdge, line);

                curEdge = edges[curEdge.id];

                Vertex vertex(getNextVertexId(), curLineEdgeTwin.id, p, curEdge.lineId, line.id);
                vertexes.push_back(vertex);
                edges[curLineEdgeTwin.id].fromVid = vertex.id;

                Edge curTwin = edges[curEdge.twinEid];

                int e1 = getNextEdgeId();
                int e2 = getNextEdgeId();
                Edge curEdge2 = Edge(e1, vertex.id, curEdge.nextEid, curLineEdge.id, curTwin.id, curEdge.lineId);
                Edge curTwin2 = Edge(e2, vertex.id, curTwin.nextEid, -1, curEdge.id, curTwin.lineId);
                edges[curEdge.nextEid].prevEid = curEdge2.id;
                edges[curTwin.nextEid].prevEid = curTwin2.id;
                edges[curEdge.id].twinEid = curTwin2.id;
                edges[curTwin.id].twinEid = curEdge2.id;
                edges[curLineEdge.id].nextEid = curEdge2.id;
                edges[curLineEdgeTwin.id].prevEid = curEdge.id;
                edges[curEdge.id].nextEid = curLineEdgeTwin.id;
                edges[curLineEdgeTwin.id].fromVid = vertex.id;

                e1 = getNextEdgeId();
                e2 = getNextEdgeId();
                curLineEdge = Edge(e1, vertex.id, -1, curTwin.id, e2, line.id);
                curLineEdgeTwin = Edge(e2, -1, curTwin2.id, -1, e1, twinLine.id);
                edges[curTwin.id].nextEid = curLineEdge.id;
                curTwin2.prevEid = curLineEdgeTwin.id;

                edges.push_back(curEdge2);
                edges.push_back(curTwin2);
                edges.push_back(curLineEdge);
                edges.push_back(curLineEdgeTwin);

                curEdge = curTwin2;
            }
            curEdge = edges[curEdge.nextEid];
            if (curEdge.fromVid == infinityVid
                    && fromRight(line, lines[curEdge.lineId])) {
                edges[curEdge.id].prevEid = curLineEdge.id;
                edges[curEdge.prevEid].nextEid = curLineEdgeTwin.id;
                edges[curLineEdge.id].nextEid = curEdge.id;
                edges[curLineEdgeTwin.id].fromVid = infinityVid;
                edges[curLineEdgeTwin.id].prevEid = curEdge.prevEid;
                break;
            }
        }
        return line.id;
    }

    void triangulate() {
        int edgesWas = edges.size();
        const int EDGE_NONE = 1;
        const int EDGE_ADDED = 2;
        const int EDGE_PROCESSED = 3;
        int state[edgesWas];
        for (int i = 0; i < edgesWas; i++) {
            state[i] = EDGE_NONE;
        }

        int q[edgesWas];
        q[0] = 22;
        state[22] = EDGE_ADDED;
        int next = 0;
        int last = 0;
        while (next <= last) {
            {
                int startID = q[next];
                Edge e = edges[edges[startID].nextEid];
                while(e.id != startID){
                    state[e.id] = EDGE_PROCESSED;
                    if(state[e.twinEid] == EDGE_NONE) {
                        last++;
                        q[last] = e.twinEid;
                        state[e.twinEid] = EDGE_ADDED;
                    }
                    e = edges[e.nextEid];
                }
            }

            Edge e = edges[q[next]];
            next++;
            if (state[e.id] == EDGE_PROCESSED) {
                continue;
            }
            state[e.id] = EDGE_PROCESSED;

            int prevEId = e.prevEid;
            Edge prev = edges[e.nextEid];
            Edge cur = edges[prev.nextEid];
            while(cur.id != e.id && cur.id != prevEId) {
                int e1 = getNextEdgeId();
                int e2 = getNextEdgeId();
                Edge a(e1, e.fromVid, cur.id, e.prevEid, e2, -1);
                Edge twin(e2, cur.fromVid, e.id, prev.id, e1, -1);
                edges[prev.id].nextEid = twin.id;
                edges[cur.id].prevEid = a.id;
                edges[e.id].prevEid = twin.id;
                edges[prevEId].nextEid = a.id;
                prev = cur;
                cur = edges[cur.nextEid];
                e = a;
                edges.push_back(a);
                edges.push_back(twin);
            }
        }
    }

    void print() {
        cout << "____Vertexes (count=" << vertexes.size() << ")" << endl;
        for (unsigned int i = 0; i < vertexes.size(); i++) {
            Vertex v = vertexes[i];
            cout << " index=" << i << " id=" << v.id << " outputEdge=" << v.outEid << " point=(x=" << v.point.x << ", y=" << v.point.y << ")" << endl;
        }
        cout << "____Edges (count=" << edges.size() << ")" << endl;
        for (unsigned int i = 0; i < edges.size(); i++) {
            Edge e = edges[i];
            cout << " index=" << i << " id=" << e.id << " twinEdge=" << e.twinEid << " nextEdge=" << e.nextEid
                    << " prevEdge=" << e.prevEid << " fromVertex=" << e.fromVid << " line=" << e.lineId << endl;
        }
        cout << "____Lines (count=" << lines.size() << ")" << endl;
        for (unsigned int i = 0; i < lines.size(); i++) {
            Line l = lines[i];
            cout << " index=" << i << " id=" << l.id << " a=" << l.a << " b=" << l.b << " c=" << l.c << " twinLine=" << l.twinLid << endl;
        }
        cout << "____________________" << endl;
    }

    int getNextEdgeId() {
        return nextEdgeId++;
    }

    int getNextVertexId() {
        return nextVertexId++;
    }

    int getNextLineId() {
        return nextLineId++;
    }
};
