//
// Created by Xinyang Liu on 2021/11/13.
//

#ifndef ZBUFFER_MODEL_H
#define ZBUFFER_MODEL_H

#include "opencv2/core.hpp"
#include <iostream>
#define INT_INF 0xfffffff

using namespace std;
using namespace cv;

// use Point3f in opencv as basic vector struct
class Model;
struct Face{
    vector<int> normal_ids, vertex_ids;
    Point3f normal;
    Point3f color;
public:
    void print(const Model& model);
    bool fix(Model& model, Point3f& n);
};

class Model {
private:
    int height, width;
public:
    vector<Point3f> vertices, normals;
    vector<Face> faces;
    void set_size(int w, int h);
    bool load_obj(string path);
    void resize();
    void render(Point3f light_pos, Point3f light_color);
    void rotate(Point3f axis, float degree);
};

Point3f rotate_vec_by_axis(Point3f v, Point3f axis, float degree);

#endif //ZBUFFER_MODEL_H
