#include <iostream>
#include "Model.h"
#include "ZBuffer.h"
#include "opencv2/highgui.hpp"
#include "opencv2/core.hpp"

const int width = 800, height = 800;
Model build_demo_model(){
    // build a model with two triangles, visualized as demo.png
    // 1 (112, 212)(114, 745)(786, 749)
    // 2 (627, 223)(227, 608)(968, 607)
    Model model;
    Point3f p11(112, 212, 200), p12(114, 745, 200), p13(786, 749, 200);
    Point3f p21(627, 223, -1000), p22(285, 608, 500), p23(968, 608, -300);
    model.vertices.push_back(p11);
    model.vertices.push_back(p12);
    model.vertices.push_back(p13);
    model.vertices.push_back(p21);
    model.vertices.push_back(p22);
    model.vertices.push_back(p23);

    Face f1, f2;
    f1.vertex_ids.push_back(0);
    f1.vertex_ids.push_back(1);
    f1.vertex_ids.push_back(2);
    f2.vertex_ids.push_back(3);
    f2.vertex_ids.push_back(4);
    f2.vertex_ids.push_back(5);

    model.faces.push_back(f1);
    model.faces.push_back(f2);

    return model;
}

void simple_demo(){
    // a simple demo to test ZBuffer
    Model model = build_demo_model();
    ZBuffer zBuffer(model, 1000, 1000);
    zBuffer.scan();

    Mat m(1000, 1000, CV_8UC3);
    for(int i = 0; i < 1000; i++){
        for(int j = 0; j < 1000; j++){
            switch (zBuffer.frame_buffer[i][j]){
                case 0:
                    m.at<Vec3b>(i,j)[0] = 255;
                    m.at<Vec3b>(i,j)[1] = 0;
                    m.at<Vec3b>(i,j)[2] = 0;
                    break;
                case 1:
                    m.at<Vec3b>(i,j)[0] = 0;
                    m.at<Vec3b>(i,j)[1] = 255;
                    m.at<Vec3b>(i,j)[2] = 0;
                    break;
                case -1:
                    m.at<Vec3b>(i,j)[0] = 0;
                    m.at<Vec3b>(i,j)[1] = 0;
                    m.at<Vec3b>(i,j)[2] = 0;
                    break;
            }

        }
    }
    imshow("demo", m);
    waitKey(0);
}

void show(string path){
    Model model;
    model.load_obj(path);
    model.set_size(width, height);
    model.resize();
    model.render(Point3f(400, 400, 800), Point3f(0.3, 0.3, 0.3));
    char c;
    while(1){
        c = waitKey(0);
        if(c =='d')
            model.rotate(Point3f(0, 1, 0), 10);
        if(c == 'a')
            model.rotate(Point3f(0, 1, 0), -10);
        if(c == 'w')
            model.rotate(Point3f(1, 0, 0), 10);
        if(c == 's')
            model.rotate(Point3f(1, 0, 0), -10);
        if(c == 27) break;
        ZBuffer zBuffer(model, height, width);
        zBuffer.scan();
        Mat m(height, width, CV_8UC3);
        clock_t t = clock();
        for(int i = 0; i < height; i++){
            for(int j = 0; j < width; j++){
                int polygon_id = zBuffer.frame_buffer[i][j];
                Point3f color;
                if(polygon_id>=0) color = zBuffer.polygon_table[polygon_id].color;
                else color = Point3f(0,0,0);
                m.at<Vec3b>(i,j)[0] = color.x*255;
                m.at<Vec3b>(i,j)[1] = color.y*255;
                m.at<Vec3b>(i,j)[2] = color.z*255;
            }
        }
        imshow("demo", m);
        cout << "Paint model takes " << float(clock() - t) << "ms." << endl;
    }
}

int main() {
    show("../models/f-16.obj");
//    simple_demo();
    return 0;
}