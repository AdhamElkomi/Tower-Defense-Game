#include "MapGenerator.hpp"
#include <queue>
#include <algorithm>
#include <cmath>

MapGenerator::MapGenerator(uint32_t seed) : rng_(seed) {}

static std::vector<Point> neighbors4(Point p){
    return {{p.x+1,p.y},{p.x-1,p.y},{p.x,p.y+1},{p.x,p.y-1}};
}

std::vector<Point> MapGenerator::randomEdgePoints(int w,int h,int count){
    std::uniform_int_distribution<int> side(0,3);
    std::uniform_int_distribution<int> rx(0,w-1);
    std::uniform_int_distribution<int> ry(0,h-1);
    std::vector<Point> out; out.reserve(count);
    for(int i=0;i<count;i++){
        int s = side(rng_);
        if (s==0) out.push_back({rx(rng_), 0});
        else if(s==1) out.push_back({rx(rng_), h-1});
        else if(s==2) out.push_back({0, ry(rng_)});
        else          out.push_back({w-1, ry(rng_)});
    }
    return out;
}

// BFS “reverse” pour avoir un chemin simple (suffisant pour un slice)
bool MapGenerator::carvePath(Map& m, Point a, Point b){
    std::vector<int> prev(m.w*m.h, -1);
    std::queue<Point> q;
    auto id = [&](int x,int y){ return y*m.w + x; };
    q.push(a);
    prev[id(a.x,a.y)] = id(a.x,a.y);

    while(!q.empty()){
        auto u = q.front(); q.pop();
        if (u.x==b.x && u.y==b.y) break;
        for(auto v: neighbors4(u)){
            if (!m.inBounds(v.x,v.y)) continue;
            if (!m.at(v.x,v.y).walkable) continue; // évite forbidden
            int vi = id(v.x,v.y);
            if (prev[vi] != -1) continue;
            prev[vi] = id(u.x,u.y);
            q.push(v);
        }
    }
    if (prev[id(b.x,b.y)] == -1) return false;

    // retrace & grave un chemin
    Point cur = b;
    while(!(cur.x==a.x && cur.y==a.y)){
        auto& c = m.at(cur.x,cur.y);
        c.ground = Tile::Path; c.buildable = false; c.walkable = true;
        int pi = prev[id(cur.x,cur.y)];
        cur = { pi % m.w, pi / m.w };
    }
    auto& c = m.at(a.x,a.y);
    c.ground = Tile::Path; c.buildable = false; c.walkable = true;
    return true;
}

void MapGenerator::placeForbiddenPatches(Map& m, int count, int radius){
    std::uniform_int_distribution<int> rx(1, m.w-2);
    std::uniform_int_distribution<int> ry(1, m.h-2);
    for(int i=0;i<count;i++){
        int cx = rx(rng_), cy = ry(rng_);
        for(int y=-radius; y<=radius; ++y){
            for(int x=-radius; x<=radius; ++x){
                int xx = cx+x, yy = cy+y;
                if (!m.inBounds(xx,yy)) continue;
                if (x*x + y*y <= radius*radius){
                    auto& c = m.at(xx,yy);
                    c.ground = Tile::Water;
                    c.walkable = false; c.buildable = false;
                }
            }
        }
    }
}

void MapGenerator::sprinkleBlockers(Map& m, int trees, int rocks, int bush){
    std::uniform_int_distribution<int> rx(1, m.w-2);
    std::uniform_int_distribution<int> ry(1, m.h-2);

    auto place = [&](Tile t, int n){
        int placed=0, guard=0;
        while(placed<n && guard< n*50){
            guard++;
            int x = rx(rng_), y = ry(rng_);
            auto& c = m.at(x,y);
            if (c.ground==Tile::Grass && c.buildable){ // éviter chemins & forbidden
                c.ground = t;
                if (t==Tile::Forest || t==Tile::Rock){ c.walkable=false; c.buildable=false; }
                else if (t==Tile::Bush){ c.walkable=true; c.buildable=false; }
                placed++;
            }
        }
    };
    place(Tile::Forest, trees);
    place(Tile::Rock,   rocks);
    place(Tile::Bush,   bush);
}

Map MapGenerator::generate(int w, int h, int eMin, int eMax, int xMin, int xMax){
    Map m(w,h);

    // Base: herbe, walkable+buildable
    for(auto& c : m.cells){ c = Cell{}; }

    // Patches “forbidden”
    placeForbiddenPatches(m, /*count*/ 3, /*radius*/ 3);

    // Entrées / sorties
    std::uniform_int_distribution<int> de(eMin, eMax), dx(xMin, xMax);
    auto entries = randomEdgePoints(w,h,de(rng_));
    auto exits   = randomEdgePoints(w,h,dx(rng_));

    // Point “ressource” = centre approx
    Point R{ w/2, h/2 };

    // Graver des chemins: E→R et R→X
    for(auto e : entries) carvePath(m, e, R);
    for(auto x : exits)   carvePath(m, R, x);

    // Objets naturels
    sprinkleBlockers(m, /*trees*/ 80, /*rocks*/ 40, /*bush*/ 60);

    return m;
}
