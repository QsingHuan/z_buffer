//
// Created by Xinyang Liu on 2021/11/13.
//

#ifndef ZBUFFER_ZBUFFER_H
#define ZBUFFER_ZBUFFER_H

#include "opencv2/core.hpp"
#include <iostream>
#include <algorithm>
#include <list>
#include <map>
#include "Model.h"

#define MAX_DEPTH -1e10
#define BACKGROUND -1

using namespace std;
using namespace cv;

struct Polygon{
    float a, b, c, d; // the polygon plane parameters
    Point3f color;
    int id;
    Polygon(float a_, float b_, float c_, float d_, int id_, Point3f color_):
     a(a_), b(b_),c(c_),d(d_), id(id_), color(color_){};
};

struct Edge{
    float x; // x coordinate of the top endpoint
    float dx; // -1/k
    float z; // z coordinate of top endpoint
    int dy; // y range
    int id; // corresponding polygon
    bool is_extreme; // if top point is extreme point, cut it by 1 pixel when add to active edge
    Edge(float x_, float dx_, int dy_, int id_, float z_):x(x_), dx(dx_), dy(dy_), id(id_), z(z_){};
};

class ActiveEdge{
    // Active
public:
    float x; // current x coordinate
    float dx; // -1/k
    int dy; // remain y range
    int id; // corresponding polygon
    float z, dzx, dzy; // used for depth calculation
    bool dead; // true if the edge is dead and need deprecation

    ActiveEdge(Edge e, vector<Polygon> polygon_table);
    bool operator<(ActiveEdge e);
    bool update();

};

typedef pair<ActiveEdge*, ActiveEdge*> AEPair; // active edge pair

class ZBuffer {
public:
    int height, width;
    int** frame_buffer;
    float* z_buffer;
    Model& model;
    vector<Polygon> polygon_table;
    ZBuffer(Model& m, int h, int w);
    void scan();
    void check_polygon();

private:
    map<int, ActiveEdge*> build_pair_dict;
    vector<list<Edge>> edge_table;
    vector<ActiveEdge> active_edge_table;
    vector<AEPair> active_edge_pair_table;

    void build_edge_pair(ActiveEdge* pae);
    int neighbor_vertex(int i, Face face, bool previous, Point3f& p);
    bool is_extreme_point(int i, Face face);
};

#endif //ZBUFFER_ZBUFFER_H
