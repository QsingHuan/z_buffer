//
// Created by Xinyang Liu on 2021/11/13.
//

#include "Model.h"
#include <iostream>
#include <fstream>
#include <ctime>

Point3f rotate_vec_by_axis(Point3f v, Point3f axis, float degree){
    // rotate vector vec by axis with degree angle, return the new vec
    // using Rodrigues' rotation formula
    float d_arc = degree / 180 * M_PI;
    axis = axis / norm(axis); // make sure the axis is normalized
    Point3f v_rot = v * cos(d_arc) + sin(d_arc) * axis.cross(v) +
            axis * axis.dot(v) * (1 - cos(d_arc));
    return v_rot;
}


void Face::print(const Model& model) {
    cout << "-----print Face ------"<<endl;
    for(int i=0; i < vertex_ids.size(); i++)
        cout << "p" << i+1 << " " << model.vertices[vertex_ids[i]].x << " " << model.vertices[vertex_ids[i]].y << endl;
}

bool Face::fix(Model& model, Point3f& n) {
    // make model face valid for z-Buffer
    int vnum = vertex_ids.size();
    // 1. delete coincident point caused by rounding
    vector<int> vertex_to_delete;
    for(int i = 0; i < vnum-1; i++){
        for(int j = i+1; j < vnum; j++){
            Point3f v1 = model.vertices[vertex_ids[i]], v2 = model.vertices[vertex_ids[j]];
            if(v1.x==v2.x && v1.y==v2.y){
                if(v1.z > v2.z) vertex_to_delete.push_back(j);
                else vertex_to_delete.push_back(i);
            }
        }
    }
    if(vnum - vertex_to_delete.size() < 3) return false; // less than 3 point, can't form a valid polygon
    sort(vertex_to_delete.begin(), vertex_to_delete.end());
    for(int i = vertex_to_delete.size()-1; i>=0; i--)
        vertex_ids.erase(vertex_ids.begin() + vertex_to_delete[i]);

    // 2. check if this face can be seen through z-axis or degenerates a line
    n = (model.vertices[vertex_ids[2]] - model.vertices[vertex_ids[1]]).cross(
            (model.vertices[vertex_ids[1]] - model.vertices[vertex_ids[0]]));
    if(fabs(n.z)<1e-7 || n.z==0) return false;
    return true;

}

void Model::set_size(int w, int h){
    height = h, width = w;
}

bool Model::load_obj(string path){
    ifstream file(path);
    if (!file.is_open()) return false;
    string type;
    clock_t t = clock();

    while (file >> type){
        // load vertices
        if (type == "v"){
            Point3f vt;
            file >> vt.x >> vt.y >> vt.z;
            vertices.push_back(vt);
        }

        // load vertex normals
        else if (type == "vn"){
            Point3f vn;
            file >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }

        // load faces
        else if (type == "f"){
            // load faces
            Face face;
            int vIndex, tIndex, nIndex; //vertex, texture, normal index separately
            while (true){
                char ch = file.get();
                if (ch == ' ') continue;
                else if (ch == '\n' || ch == EOF) break;
                else file.putback(ch);

                file >> vIndex;//顶点的索引

                char splitter = file.get();
                nIndex = 0;

                if (splitter == '/')
                {
                    splitter = file.get();
                    if (splitter == '/')
                    {
                        file >> nIndex;//面法向量的index
                    }
                    else
                    {
                        file.putback(splitter);
                        file >> tIndex;//纹理(texture)索引
                        splitter = file.get();
                        if (splitter == '/')
                        {
                            file >> nIndex;
                        }
                        else file.putback(splitter);
                    }
                }
                else file.putback(splitter);

                face.vertex_ids.push_back(vIndex - 1);
                face.normal_ids.push_back(nIndex - 1);
            }
            assert(face.vertex_ids.size() > 2); // a face must have 3 endpoints
            // calculate face normal, used for plane params a, b, c, d
            Point3f &a = vertices[face.vertex_ids[0]],
                    &b = vertices[face.vertex_ids[1]],
                    &c = vertices[face.vertex_ids[2]];
            Point3f normal = ((b - a).cross(c - b));
            normal /= norm(normal); // normalize it
            face.normal = normal;
            faces.push_back(face);
        }
    }
    file.close();
    cout << "Loading model takes " << float(clock() - t) << "ms." << endl;
    return true;
}

void Model::resize(){
    // change model's size and position into the center of the screen
    // after resize, the center of the model is at (width / 2, height / 2, 0)
    Point3f min_xyz(INT_INF, INT_INF, INT_INF),
            max_xyz(-INT_INF, -INT_INF, -INT_INF);
    Point3f center_xyz(0.0, 0.0, 0.0);
    int vertex_num = vertices.size();
    clock_t t = clock();
    for (int i = 0; i < vertex_num; ++i)
    {
        const Point3f& vertex = vertices[i];
        min_xyz.x = min(min_xyz.x, vertex.x);
        min_xyz.y = min(min_xyz.y, vertex.y);
        min_xyz.z = min(min_xyz.z, vertex.z);
        max_xyz.x = max(max_xyz.x, vertex.x);
        max_xyz.y = max(max_xyz.y, vertex.y);
        max_xyz.z = max(max_xyz.z, vertex.z);
    }
    center_xyz = (min_xyz + max_xyz) / 2;

    float model_width = max_xyz.x - min_xyz.x;
    float model_height = max_xyz.y - min_xyz.y;
    float max_model_len = max(model_width, model_height);
    float scale = min(width, height) / max_model_len;
    scale = 0.8*scale;

    for (int i = 0; i < vertex_num; ++i)
    {
        Point3f& vertex_point = vertices[i];
        vertex_point.x = round((vertex_point.x - center_xyz.x)*scale + width / 2);
        vertex_point.y = round((vertex_point.y - center_xyz.y)*scale + height / 2);
        vertex_point.z = (vertex_point.z - center_xyz.z)*scale;

    }
    cout << "Resize model takes " << float(clock() - t) << "ms." << endl;

}

void Model::render(Point3f light_pos, Point3f light_color){
    float kd = 0.8;
    Point3f ambient_color(0.3, 0.3, 0.3);
    for(Face& face:faces){
        for(int i = 0; i < face.vertex_ids.size(); i++){
            // calculate specular reflection term for each vertex
            int vid = face.vertex_ids[i], nid = face.normal_ids[i];
            Point3f ray = light_pos - vertices[vid];
            ray /= norm(ray);
            float NL_cosine = normals[nid].dot(ray);
            if (NL_cosine > 0.0) face.color += kd * NL_cosine * light_color;
            face.color += ambient_color;
        }
        face.color /= float(face.vertex_ids.size());
        if (face.color.x > 1.0f)face.color.x = 1.0f;
        if (face.color.x < 0.0f)face.color.x = 0.0f;
        if (face.color.y > 1.0f)face.color.y = 1.0f;
        if (face.color.y < 0.0f)face.color.y = 0.0f;
        if (face.color.z > 1.0f)face.color.z = 1.0f;
        if (face.color.z < 0.0f)face.color.z = 0.0f;
    }
}

void Model::rotate(Point3f axis, float degree){
    // rotate the model through axis by angle degree
    // components need to be rotated: vertices, normals, faces' normal
    // also the model need to be re-rendered
    Point3f center(width / 2, height / 2, 0);
    for(Point3f& vertex:vertices)
        vertex = rotate_vec_by_axis(vertex-center, axis, degree)+center;
//        vertex.x = round(vertex.x), vertex.y = round(vertex.y);
    for(Point3f& normal:normals)
        normal = rotate_vec_by_axis(normal, axis, degree);
    for(Face& face:faces)
        face.normal = rotate_vec_by_axis(face.normal, axis, degree);
    (*this).resize();
    (*this).render(Point3f(400, 400, 800), Point3f(0.3, 0.3, 0.3));
}


















