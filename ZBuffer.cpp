//
// Created by Xinyang Liu on 2021/11/13.
//

#include "ZBuffer.h"
using namespace std;

int ZBuffer::neighbor_vertex(int i, Face face, bool previous, Point3f& p){
    // return neighbor id and use p to get neighbor
    if(previous){ // get previous vertex
        if(i>0) {
            p = model.vertices[face.vertex_ids[i-1]];
            return i-1;
        }
        else {
            p = model.vertices[face.vertex_ids[face.vertex_ids.size()-1]];
            return face.vertex_ids.size()-1;
        }
    }
    else{ // get next vertex
        if(i+1<face.vertex_ids.size()){
            p = model.vertices[face.vertex_ids[i+1]];
            return i+1;
        }
        else {
            p = model.vertices[face.vertex_ids[0]];
            return 0;
        }
    }
}

bool ZBuffer::is_extreme_point(int i, Face face){
    // return true if the ith vertex in face is an extreme point ((yi-1 - yi)(yi+1 - yi)>=0)
    Point3f p0, p1, p;
    neighbor_vertex(i, face, false, p0);
    neighbor_vertex(i, face, true, p1);
    float y, y0, y1;
    p = model.vertices[face.vertex_ids[i]];
    y = p.y;
    y0 = p0.y, y1 = p1.y;
    return (y0 - y)*(y1 - y) > 0;
}

ZBuffer::ZBuffer(Model& m, int h, int w):model(m) {
    // Initialize ZBuffer
    // Build all the static data structures including PolygonTable, EdgeTable
    clock_t t = clock();
    // 1. allocate buffer
    height = h, width = w;
    z_buffer = new float[width];
    frame_buffer = new int*[height];
    for (int i = 0; i < height; ++i)
    {
        frame_buffer[i] = new int[width];
    }

    // 2. Build PolygonTable and EdgeTable
    edge_table.resize(height);
    int polygon_index = 0;
    for(Face face:model.faces){
        // add polygon to polygon table
        Point3f n = (model.vertices[face.vertex_ids[2]] - model.vertices[face.vertex_ids[1]]).cross(
                (model.vertices[face.vertex_ids[1]] - model.vertices[face.vertex_ids[0]]));
        if(fabs(n.z)<1e-7 || n.z==0) continue; // this face can't be seen through z-axis, it degenerates a line
        float d = -(n.dot(model.vertices[face.vertex_ids[0]]));
        if(1){
//            face.print(model);
            face.fix(model, n);
//            face.print(model);
            Polygon tmp_polygon(n.x, n.y, n.z, d, 0, face.color);
            polygon_table.push_back(tmp_polygon);
        }
//        Polygon tmp_polygon(n.x, n.y, n.z, d, polygon_index, face.color);
//        polygon_table.push_back(tmp_polygon);

        // add edge to edge table
        for(int i = 0; i<face.vertex_ids.size(); i++){
            int top_id = i; // use i to initialize top_id, change to p2_id if p1 is not top point
            Point3f p1 = model.vertices[face.vertex_ids[i]];
            Point3f p2;
            int p2_id = neighbor_vertex(i, face, false, p2); // next point
            if (p1.y < p2.y) {  // make sure p1 is the top point
                swap(p1, p2);
                top_id = p2_id;
            }

            int dy = p1.y - p2.y; // y range
            if(dy==0) continue; // pass horizontal lines
            float dx = -(p1.x - p2.x) / (p1.y - p2.y);

            //if top point is not extreme point, label it and add to p1.y-1 list
            Edge tmp_edge(p1.x, dx, dy, polygon_index, p1.z);
            assert(p1.y >=1 && p1.y < height); // must in the window size
//            if(!is_extreme_point(top_id, face)) { // not extreme point
//                tmp_edge.is_extreme = false;
//                edge_table[p1.y-1].push_back(tmp_edge);
//            }
//            else{
//                tmp_edge.is_extreme = true;
//                edge_table[p1.y].push_back(tmp_edge);
//            }
            if(1){
//                tmp_edge.id = 0;
                if(!is_extreme_point(top_id, face)) { // not extreme point
                    tmp_edge.is_extreme = false;
                    edge_table[p1.y-1].push_back(tmp_edge);
                }
                else{
                    tmp_edge.is_extreme = true;
                    edge_table[p1.y].push_back(tmp_edge);
                }
            }
        }
        polygon_index++;
    }
    cout << "Build Zbuffer takes " << float(clock() - t) << "ms." << endl;
}
void ZBuffer::build_edge_pair(ActiveEdge* pae) {
    int id = pae->id;
    map<int, ActiveEdge*>::iterator p = build_pair_dict.find(id);
    if(p != build_pair_dict.end()){
        // have a previous half, then build a pair
        active_edge_pair_table.push_back(AEPair(p->second, pae));
        build_pair_dict.erase(p);
    }
    else{
        build_pair_dict.insert(pair<int, ActiveEdge*>(id, pae));
    }
}

bool is_dead_edge(ActiveEdge ae) {return ae.dead;}

void ZBuffer::scan() {
    bool update_flag= true; // whether the active edge pairs should be updated
    clock_t t = clock();
    for(int i = height-1; i >= 0; i--){
        // scan from top to buttom
        // 1. Initialize z_buffer and  frame_buffer
        for(int j = 0;j < width; j++){
            z_buffer[j] = MAX_DEPTH;
            frame_buffer[height-i-1][j] = BACKGROUND;
        }

        if(active_edge_pair_table.empty()&&edge_table[i].empty()) continue; // no graphics at this scan line
//        cout << "Scaning " << i <<endl;
        // 2. Update Active Edge Table
        if(!edge_table[i].empty()){
            // new edges need to be added
            update_flag = true;
            list<Edge> edge_list = edge_table[i];
            for(Edge e:edge_list){
                ActiveEdge ae(e, polygon_table);
                bool dead_flag = false;
                if (!e.is_extreme) dead_flag = ae.update(); // cut 1 pixel is equal to update once
                if (!dead_flag)
                    active_edge_table.push_back(ae);
            }
        }

        // 3. Update Active Edge Pair
        if(update_flag){
            // Reset the pair_dict and pair_table
            build_pair_dict.clear(); // clear dict
            active_edge_pair_table.clear(); // clear pair_table
            update_flag = false; // reset

            sort(active_edge_table.begin(), active_edge_table.end());
            if(active_edge_table.size()%2!=0) {
                cout << endl;
                check_polygon();
            }
            for(ActiveEdge& ae:active_edge_table){
                build_edge_pair(&ae);
            }
        }

        // 4. Go through each pair, update depth and pair itself
        for(AEPair ae_pair:active_edge_pair_table){
            ActiveEdge& e1 = *(ae_pair.first);
            ActiveEdge& e2 = *(ae_pair.second);
            float z = e1.z;
            for(int x = e1.x; x <= e2.x;x++){
                if(z>z_buffer[x]){
                    z_buffer[x] = z;
                    frame_buffer[height-i-1][x] = e1.id;
                }
                z += e1.dzx;
            }
            update_flag |= e1.update();
            update_flag |= e2.update();
        }
        active_edge_table.erase(remove_if(active_edge_table.begin(), active_edge_table.end(), is_dead_edge), active_edge_table.end());
    }
    cout << "Scan model takes " << float(clock() - t) << "ms." << endl;
}


bool ActiveEdge::operator<(ActiveEdge e){
    if ( x<e.x ) return true;
    else if ( x==e.x && dx>e.dx) return true; // dx should be sorted decreasingly
    return false;
}
bool ActiveEdge::update(){
    // update active edge
    // return true if the edge need to be removed
    dy--;
    x+=dx;
    z+=dzx*dx+dzy;
    dead = (dy<0);
    return dead;
}

ActiveEdge::ActiveEdge(Edge e, vector<Polygon> polygon_table):x(e.x),dx(e.dx), dy(e.dy), id(e.id), z(e.z){
    dead = false;
    Polygon polygon = polygon_table[id];
    assert(polygon.c!=0);
    dzx = -polygon.a/polygon.c;
    dzy = polygon.b/polygon.c;
}

void ZBuffer::check_polygon(){
    // a polygon should have even active edges
    map<int, int> records;
    for(auto ae:active_edge_table){
        records[ae.id]++;
    }
    for(auto record:records){
        if(record.second%2!=0)
            cout << "Wrong Polygon" << record.first << endl;
    }
}