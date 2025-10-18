#include "MapGenerator.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <vector>

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

// BFS “reverse” pour avoir un chemin simple
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

// ------------------------ WATER PATCHES (lacs + rivières) ------------------------

void MapGenerator::placeForbiddenPatches(Map& m, int count, int radius){
    // patches d'eau plus grands → meilleurs amas visuels (TileMap rejettera les petites flaques)
    std::uniform_int_distribution<int> rx(2, m.w-3);
    std::uniform_int_distribution<int> ry(2, m.h-3);
    std::uniform_int_distribution<int> rr(std::max(3,radius-1), radius+2);

    for(int i=0;i<count;i++){
        int cx = rx(rng_), cy = ry(rng_);
        int R  = rr(rng_);
        for(int y=-R; y<=R; ++y){
            for(int x=-R; x<=R; ++x){
                int xx = cx+x, yy = cy+y;
                if (!m.inBounds(xx,yy)) continue;
                // disque + petite irrégularité pour casser le cercle parfait
                if (x*x + y*y <= R*R + ((x^y)&1)){
                    auto& c = m.at(xx,yy);
                    c.ground = Tile::Water;
                    c.walkable = false; c.buildable = false;
                }
            }
        }
    }
}

// ------------------------ DECOR (sans rocks) -------------------------------------

void MapGenerator::sprinkleBlockers(Map& m, int trees, int /*rocks*/, int bush){
    std::uniform_int_distribution<int> rx(1, m.w-2);
    std::uniform_int_distribution<int> ry(1, m.h-2);

    auto place = [&](Tile t, int n){
        int placed=0, guard=0;
        while(placed<n && guard< n*50){
            guard++;
            int x = rx(rng_), y = ry(rng_);
            auto& c = m.at(x,y);
            if (c.ground==Tile::Grass && c.buildable){ // éviter chemins/eau
                c.ground = t;
                if (t==Tile::Forest){ c.walkable=false; c.buildable=false; }
                else if (t==Tile::Bush){ c.walkable=true; c.buildable=false; }
                placed++;
            }
        }
    };
    place(Tile::Forest, trees);
    // ⚠️ pas de Rock ici (Rock est réservé aux pads de construction)
    place(Tile::Bush,   bush);
}

// ------------------------ BUILD PADS NEAR PATHS ---------------------------------

static bool rectInBounds(const Map& m, int x, int y, int w, int h){
    return x >= 0 && y >= 0 && (x+w) <= m.w && (y+h) <= m.h;
}

static bool canPlacePad(const Map& m, int x, int y, int w, int h){
    if (!rectInBounds(m, x, y, w, h)) return false;
    for (int yy = y; yy < y+h; ++yy){
        for (int xx = x; xx < x+w; ++xx){
            const auto& c = m.at(xx,yy);
            if (c.ground == Tile::Water) return false;
            if (c.ground == Tile::Path)  return false;
            if (!c.buildable)            return false;
        }
    }
    // bordure de sécurité contre l'eau
    for (int yy = y-1; yy <= y+h; ++yy){
        for (int xx = x-1; xx <= x+w; ++xx){
            if (!m.inBounds(xx,yy)) continue;
            if (m.at(xx,yy).ground == Tile::Water) return false;
        }
    }
    return true;
}

static void placePad(Map& m, int x, int y, int w, int h){
    for (int yy = y; yy < y+h; ++yy){
        for (int xx = x; xx < x+w; ++xx){
            auto& c = m.at(xx,yy);
            c.ground    = Tile::Rock;   // ← FREE tile (atlas col 11)
            c.walkable  = false;
            c.buildable = true;
        }
    }
}

static std::pair<int,int> pathNormal(const Map& m, int x, int y){
    auto grass = [&](int xx,int yy){
        return m.inBounds(xx,yy) && m.at(xx,yy).ground == Tile::Grass;
    };
    bool n = grass(x, y-1), s = grass(x, y+1), w = grass(x-1, y), e = grass(x+1, y);

    if (n && !s) return {0,-1};
    if (s && !n) return {0, 1};
    if (w && !e) return {-1,0};
    if (e && !w) return { 1,0};

    int hx = ((x + y) & 1) ? 1 : -1;
    int hy = ((x * 31 + y * 17) & 1) ? 1 : -1;
    if (n) return {hx,-1};
    if (s) return {hx, 1};
    if (w) return {-1,hy};
    if (e) return { 1,hy};
    return {0,-1};
}



// autorise la progression (y compris dans l'eau si on veut poser un pont)
static inline bool canStepForPath(const Map& m, int x, int y){
    if (!m.inBounds(x,y)) return false;
    const auto& c = m.at(x,y);
    // On peut marcher sur Grass/Path. On “accepte” Water pour créer un pont.
    // On refuse tout ce qui est non-walkable (pads/base, etc.)
    if (!c.walkable && c.ground != Tile::Water) return false;
    return true;
}



static inline bool canCarveHere(const Map& m, int x, int y){
    if (!m.inBounds(x,y)) return false;
    const auto& c = m.at(x,y);
    if (c.ground == Tile::Water) return false;
    if (!c.walkable) return false; // pads/base etc.
    return true;
}

static void carveCellToPath(Map& m, int x, int y){
    auto& c = m.at(x,y);
    c.ground = Tile::Path;
    c.buildable = false;
    c.walkable  = true;
}

static float pathCoverage(const Map& m){
    int water=0, path=0, total=m.w*m.h;
    for (const auto& c : m.cells){
        if (c.ground==Tile::Water) ++water;
        if (c.ground==Tile::Path)  ++path;
    }
    int usable = total - water;
    if (usable<=0) return 0.f;
    return (float)path / (float)usable;
}

// Renvoie toutes les cases Path (frontières comprises)
static std::vector<Point> collectPathFrontier(const Map& m){
    std::vector<Point> pts;
    pts.reserve(m.w*m.h/4);
    for (int y=0;y<m.h;++y){
        for(int x=0;x<m.w;++x){
            if (m.at(x,y).ground!=Tile::Path) continue;
            // on considère la case comme "frontière" si au moins un voisin est carve-able
            for(auto nb: neighbors4({x,y})){
                if (canCarveHere(m, nb.x, nb.y) && m.at(nb.x,nb.y).ground!=Tile::Path){
                    pts.push_back({x,y});
                    break;
                }
            }
        }
    }
    return pts;
}


struct NoiseField {
    int w=0,h=0;
    std::vector<float> n; // [0..1]
    float at(int x,int y) const { return n[y*w+x]; }
};

static NoiseField makeNoise(std::mt19937& rng, int w, int h){
    std::uniform_real_distribution<float> uf(0.f,1.f);
    NoiseField f; f.w=w; f.h=h; f.n.resize(w*h);
    for (int i=0;i<w*h;++i) f.n[i] = uf(rng);
    // petit lissage box 3x3
    std::vector<float> out = f.n;
    auto idx = [&](int x,int y){ return y*w+x; };
    for (int y=1;y<h-1;++y){
        for(int x=1;x<w-1;++x){
            float s=0; int c=0;
            for(int dy=-1; dy<=1; ++dy)
                for(int dx=-1; dx<=1; ++dx){ s += f.n[idx(x+dx,y+dy)]; ++c; }
            out[idx(x,y)] = s / (float)c;
        }
    }
    f.n.swap(out);
    return f;
}

static inline float norm01(float v, float lo, float hi){
    return (v-lo) / std::max(0.0001f, hi-lo);
}


// Retourne true si un chemin a été gravé. Ajoute de la "complexité" via des coûts.
bool carvePathComplex(Map& m, Point a, Point b, const NoiseField& nf, float bendWeight=0.7f){
    const int W=m.w, H=m.h;
    auto inB = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };
    auto id  = [&](int x,int y){ return y*W+x; };

    const float CX = (W-1)*0.5f, CY = (H-1)*0.5f;

    // Dijkstra
    std::vector<float> dist(W*H, 1e30f);
    std::vector<int>   prev(W*H, -1);
    std::vector<char>  prevDir(W*H, -1); // 0:E,1:W,2:S,3:N

    struct Node{ int x,y; float d; };
    struct Cmp{ bool operator()(const Node& a,const Node& b)const{return a.d>b.d;} };
    std::priority_queue<Node,std::vector<Node>,Cmp> pq;

    auto push = [&](int x,int y,float d,int p,int dir){
        int i=id(x,y);
        if (d<dist[i]){ dist[i]=d; prev[i]=p; prevDir[i]=dir; pq.push({x,y,d}); }
    };

    auto canStep = [&](int x,int y){
        return canStepForPath(m, x, y);
    };

    dist[id(a.x,a.y)] = 0.f;
    pq.push({a.x,a.y,0.f});

    auto stepCost = [&](int px,int py,int x,int y,int dirLast,int dirNow)->float{
        float cost = 1.f;

        if (dirLast != -1 && dirNow != dirLast) cost += 0.9f;      // virages encouragés
        float n = nf.at(x,y);
        cost += (0.6f * (1.f - n));                                // bruit
        const float CX = (m.w-1)*0.5f, CY=(m.h-1)*0.5f;
        float dc = std::hypot(x-CX, y-CY);
        cost += 0.12f * ((dc) / std::hypot(CX,CY));                // serpenter vers le centre
        float db = std::min({ (float)x, (float)y, (float)(m.w-1-x), (float)(m.h-1-y) });
        cost += 0.20f * (1.f - std::min(1.f, db / (std::min(m.w,m.h)*0.5f))); // éviter bords
        if (m.at(x,y).ground == Tile::Path) cost += 0.25f;         // évite trop repasser

        // 💧 traversée d’eau = “pont” (coût très élevé)
        if (m.at(x,y).ground == Tile::Water) cost += 8.0f;

        return cost;
    };
    // dirs: 0:E,1:W,2:S,3:N
    auto neighborStep = [&](int x,int y,int dir)->Point{
        switch(dir){ case 0: return {x+1,y}; case 1: return {x-1,y};
                     case 2: return {x,y+1}; default: return {x,y-1}; }
    };

    while(!pq.empty()){
        auto u = pq.top(); pq.pop();
        if (u.d != dist[id(u.x,u.y)]) continue;
        if (u.x==b.x && u.y==b.y) break;

        for(int nd=0; nd<4; ++nd){
            auto v = neighborStep(u.x,u.y,nd);
            if (!canStep(v.x,v.y)) continue;
            float w = stepCost(u.x,u.y,v.x,v.y, prevDir[id(u.x,u.y)], nd);
            push(v.x,v.y, u.d + w, id(u.x,u.y), nd);
        }
    }

    if (dist[id(b.x,b.y)] >= 1e20f) return false;

    // Retrace & grave
    int cur = id(b.x,b.y);
    while(cur != id(a.x,a.y)){
        int x = cur % W, y = cur / W;
        auto& c = m.at(x,y);
        c.ground = Tile::Path; c.buildable = false; c.walkable = true;
        cur = prev[cur];
        if (cur<0) break;
    }
    auto& c0 = m.at(a.x,a.y);
    c0.ground = Tile::Path; c0.buildable = false; c0.walkable = true;
    return true;
}

static Point lateralOffset(Point a, Point b, float k, int W, int H){
    // point à ~k de la ligne (perpendiculaire + clamp)
    float dx = float(b.x - a.x), dy = float(b.y - a.y);
    if (dx==0 && dy==0) return a;
    // vecteur normal ±
    float nx = -dy, ny = dx;
    float len = std::max(1.f, std::hypot(nx,ny));
    nx/=len; ny/=len;
    int cx = int((a.x + b.x)*0.5f + nx * k);
    int cy = int((a.y + b.y)*0.5f + ny * k);
    cx = std::clamp(cx, 1, W-2);
    cy = std::clamp(cy, 1, H-2);
    return {cx,cy};
}




static int pathDegree(const Map& m, int x, int y){
    int d=0;
    for(auto nb: neighbors4({x,y})){
        if (!m.inBounds(nb.x,nb.y)) continue;
        if (m.at(nb.x,nb.y).ground == Tile::Path) ++d;
    }
    return d;
}

// renvoie true si on a touché une route existante (reconnexion)
static bool randomWalkReconnect(Map& m, std::mt19937& rng, Point start, int maxLen){
    std::uniform_int_distribution<int> dir(0,3);
    auto step = [&](int d)->Point{
        switch(d){case 0:return Point{1,0};case 1:return Point{-1,0};case 2:return Point{0,1};default:return Point{0,-1};}
    };

    int d = dir(rng);
    Point p = start;
    int len=0;

    while(len<maxLen){
        auto v = step(d);
        int nx = p.x+v.x, ny=p.y+v.y;
        if (!m.inBounds(nx,ny)) break;
        auto& c = m.at(nx,ny);
        if (!c.walkable && c.ground != Tile::Water) break; // pads/base

        // si on touche un path existant : RECONNEXION → carrefour/boucle
        if (c.ground == Tile::Path){
            // on pose aussi la case précédente pour joindre proprement
            // (si elle n'est pas déjà path)
            if (m.at(p.x,p.y).ground != Tile::Path){
                m.at(p.x,p.y).ground=Tile::Path; m.at(p.x,p.y).buildable=false; m.at(p.x,p.y).walkable=true;
            }
            return true;
        }

        // on “carve” (y compris sur l’eau → petit pont secondaire possible)
        c.ground = Tile::Path; c.buildable=false; c.walkable=true;

        // de temps en temps, on tourne
        if ((dir(rng)%4)==0) d = (d + ((dir(rng)&1)?1:3)) & 3;

        p={nx,ny}; ++len;
    }
    return false;
}

// branches avec stratégie “reconnect-or-remove”
static void growBranches(Map& m, std::mt19937& rng, int attempts = 60, int maxLen = 28){
    std::uniform_int_distribution<int> rx(1, m.w-2), ry(1, m.h-2);

    for(int t=0;t<attempts;++t){
        // choisir une case Path avec assez de voisins (évite de partir d'une impasse)
        int sx=rx(rng), sy=ry(rng);
        if (m.at(sx,sy).ground != Tile::Path) continue;
        if (pathDegree(m,sx,sy) < 2 && (t&1)) continue; // favorise les endroits vivants

        // copie pour rollback si on ne reconnecte pas
        std::vector<std::pair<Point,Tile>> carved;

        auto mark = [&](int x,int y){
            carved.push_back({{x,y}, m.at(x,y).ground});
            auto& c = m.at(x,y);
            c.ground = Tile::Path; c.buildable=false; c.walkable=true;
        };

        // marche qui essaye de se reconnecter
        Point p{sx,sy};
        int len=0;
        bool reconnected=false;

        // on avance d’un pas initial
        for (int kick=0; kick<4 && !reconnected; ++kick){
            // direction initiale = vers une case libre si possible
            static const Point dirs[4]={{1,0},{-1,0},{0,1},{0,-1}};
            Point v = dirs[kick];
            int nx=p.x+v.x, ny=p.y+v.y;
            if (!m.inBounds(nx,ny)) continue;
            auto& c = m.at(nx,ny);
            if (!c.walkable && c.ground != Tile::Water) continue;
            if (c.ground == Tile::Path) continue;
            mark(nx,ny);
            p={nx,ny};
            break;
        }

        // promenade
        std::uniform_int_distribution<int> turn(0,3);
        while(len<maxLen){
            int pick = turn(rng);
            static const Point dirs[4]={{1,0},{-1,0},{0,1},{0,-1}};
            Point v = dirs[pick];
            int nx=p.x+v.x, ny=p.y+v.y;
            if (!m.inBounds(nx,ny)) break;
            auto& c = m.at(nx,ny);

            // touche une route existante => reconnecté !
            if (c.ground == Tile::Path){ reconnected=true; break; }

            if (!c.walkable && c.ground != Tile::Water) break;
            mark(nx,ny);
            p={nx,ny};
            ++len;

            // petite chance de changer de direction souvent → zigzag
            if ((turn(rng)%3)==0) continue;

            // essai de “saut” latéral
            int side = (turn(rng)&1)?1:3;
            Point v2 = dirs[(pick+side)&3];
            int lx=p.x+v2.x, ly=p.y+v2.y;
            if (m.inBounds(lx,ly) && (m.at(lx,ly).walkable || m.at(lx,ly).ground==Tile::Water)
                && m.at(lx,ly).ground!=Tile::Path){
                mark(lx,ly); p={lx,ly}; ++len;
            }
        }

        // pas reconnecté ? → rollback (évite les impasses)
        if (!reconnected){
            for(auto& rc : carved){
                auto& cell = m.at(rc.first.x, rc.first.y);
                cell.ground = rc.second;
                if (cell.ground==Tile::Grass){ cell.buildable=true; cell.walkable=true; }
                if (cell.ground==Tile::Water){ cell.buildable=false; cell.walkable=false; }
            }
        }
    }
}



static void fixDeadEnds(Map& m, std::mt19937& rng, int maxPasses = 3){
    for (int pass=0; pass<maxPasses; ++pass){
        bool changed=false;
        for(int y=1;y<m.h-1;++y){
            for(int x=1;x<m.w-1;++x){
                if (m.at(x,y).ground != Tile::Path) continue;
                int deg = pathDegree(m,x,y);
                if (deg != 1) continue; // cul-de-sac uniquement

                // Essayer de rejoindre un path proche (rayon 6)
                int bestDx=0, bestDy=0, bestD2=9999;
                for(int dy=-6; dy<=6; ++dy){
                    for(int dx=-6; dx<=6; ++dx){
                        int xx=x+dx, yy=y+dy;
                        if (!m.inBounds(xx,yy)) continue;
                        if (m.at(xx,yy).ground != Tile::Path) continue;
                        int d2 = dx*dx+dy*dy;
                        if (d2==0 || d2>bestD2) continue;
                        bestD2=d2; bestDx=dx; bestDy=dy;
                    }
                }
                if (bestD2<9999){
                    // on perce un petit couloir (avec ponts si eau)
                    int xx=x, yy=y;
                    int tx=x+bestDx, ty=y+bestDy;
                    while (xx!=tx || yy!=ty){
                        if (xx<tx) ++xx; else if (xx>tx) --xx;
                        if (yy<ty) ++yy; else if (yy>ty) --yy;
                        auto& c = m.at(xx,yy);
                        c.ground = Tile::Path; c.buildable=false; c.walkable=true;
                    }
                    changed=true;
                }else{
                    // pas moyen de se reconnecter → on enlève l’impasse
                    auto& c = m.at(x,y);
                    c.ground=Tile::Grass; c.walkable=true; c.buildable=true;
                    changed=true;
                }
            }
        }
        if (!changed) break;
    }
}






// Marche aléatoire avec embranchements
/*void growBranches(Map& m, std::mt19937& rng, int attempts = 40, int maxLen = 18){
    std::uniform_int_distribution<int> rx(1, m.w-2), ry(1, m.h-2), dir(0,3);
    auto can = [&](int x,int y){
        if (!m.inBounds(x,y)) return false;
        auto& c = m.at(x,y);
        return c.walkable && c.ground != Tile::Water;
    };
    auto step = [&](int d)->Point{
        switch(d){case 0:return Point{1,0};case 1:return Point{-1,0};case 2:return Point{0,1};default:return Point{0,-1};}
    };

    for(int t=0;t<attempts;++t){
        // prend une case path au hasard
        int sx=rx(rng), sy=ry(rng);
        if (m.at(sx,sy).ground != Tile::Path) continue;

        int d = dir(rng);
        Point p{sx,sy};
        int len=0;
        while(len<maxLen){
            auto v = step(d);
            int nx=p.x+v.x, ny=p.y+v.y;
            if (!can(nx,ny)) break;
            auto& c = m.at(nx,ny);
            if (c.ground == Tile::Path) break; // évite sur-dessiner trop
            c.ground = Tile::Path; c.buildable=false; c.walkable=true;

            // chance de tourner
            if ((dir(rng)%5)==0) d = (d + ((dir(rng)&1)?1:3)) & 3;
            p={nx,ny}; ++len;
        }
    }
}
*/






// ✅ DÉFINITION MEMBRE (dans MapGenerator.cpp)
void MapGenerator::placeBuildPadsNearPaths(Map& m, int targetPads, int step, int offset){
    std::vector<Point> pathCells;
    pathCells.reserve(m.w*m.h/4);
    for (int y=0; y<m.h; ++y)
        for (int x=0; x<m.w; ++x)
            if (m.at(x,y).ground == Tile::Path) pathCells.push_back({x,y});

    int placed = 0;
    for (std::size_t i=0; i<pathCells.size() && placed < targetPads; i += (std::size_t)step){
        int x = pathCells[i].x, y = pathCells[i].y;
        auto [nx,ny] = pathNormal(m, x, y);   // ← ton helper déjà défini au-dessus

        auto trySide = [&](int sx, int sy)->bool{
            int px = x + sx * offset;
            int py = y + sy * offset;
            const std::pair<int,int> sizes[] = { {3,3}, {2,3}, {2,2} };
            for (auto [w,h] : sizes){
                int ox = px - (sx>0 ? 0 : (sx<0 ? w-1 : w/2));
                int oy = py - (sy>0 ? 0 : (sy<0 ? h-1 : h/2));
                if (canPlacePad(m, ox, oy, w, h)){   // ← tes helpers canPlacePad/placePad
                    placePad(m, ox, oy, w, h);
                    return true;
                }
            }
            return false;
        };

        if (trySide(nx,ny) || trySide(-nx,-ny)) placed++;
    }
}


// ------------------------ GENERATE ----------------------------------------------

Map MapGenerator::generate(int w, int h, int eMin, int eMax, int xMin, int xMax){
    Map m(w,h);

    // Base: herbe, walkable+buildable
    for(auto& c : m.cells){ c = Cell{}; }

    // Eau
    placeForbiddenPatches(m, /*count*/ 4, /*radius*/ 4);

    // Entrées / sorties aléatoires
    std::uniform_int_distribution<int> de(eMin, eMax), dx(xMin, xMax);
    auto entries = randomEdgePoints(w,h,de(rng_));
    auto exits   = randomEdgePoints(w,h,dx(rng_));

    // Store entries and exits in map
    m.entries = entries;
    m.exits = exits;

    // Centre et base
    Point R{ w/2, h/2 };
    Point gate = placeResourceBaseArea(m, R.x, R.y, 3, 3);

    NoiseField nf = makeNoise(rng_, w, h);

    // Entrées -> zigzags -> gate
    for (auto e : entries){
        float k = std::min(w,h) * 0.38f;
        Point wp1 = lateralOffset(e, gate,  k, w, h);
        Point wp2 = lateralOffset(e, gate, -k, w, h);
        carvePathComplex(m, e,   wp1, nf, 1.0f);
        carvePathComplex(m, wp1, wp2, nf, 1.0f);
        carvePathComplex(m, wp2, gate, nf, 0.9f);
    }

    // gate -> zigzags -> Sorties
    for (auto x : exits){
        float k = std::min(w,h) * 0.38f;
        Point wp1 = lateralOffset(gate, x,  k, w, h);
        Point wp2 = lateralOffset(gate, x, -k, w, h);
        carvePathComplex(m, gate, wp1, nf, 0.9f);
        carvePathComplex(m, wp1, wp2, nf, 1.0f);
        carvePathComplex(m, wp2, x,   nf, 1.0f);
    }

    // Branches qui SE RECONNECTENT (carrefours/boucles)
    growBranches(m, rng_, /*attempts*/ 70, /*maxLen*/ 30);

    // Anti-impasses (reconnecte sinon supprime)
    fixDeadEnds(m, rng_);

    // Pads & décor
    placeBuildPadsNearPaths(m, std::max(6, (w*h)/160), 5, 1);
    sprinkleBlockers(m, 60, 0, 40);

     // ✅ IMPORTANT : retourner la map
    return m;

}






/*
void MapGenerator::placeResourceBaseArea(Map& m, int cx, int cy, int wTiles, int hTiles){
    // top-left du rectangle centré sur (cx,cy)
    int x0 = cx - wTiles/2;
    int y0 = cy - hTiles/2;

    // sécurise les bornes
    x0 = std::max(0, std::min(x0, m.w - wTiles));
    y0 = std::max(0, std::min(y0, m.h - hTiles));

    for(int y = 0; y < hTiles; ++y){
        for(int x = 0; x < wTiles; ++x){
            auto& c = m.at(x0+x, y0+y);
            // on force un “sol” propre pour le bâtiment
            c.ground    = Tile::Rock;   // ← FREE/pad (colonne 11 de l’atlas)
            c.walkable  = false;        // pas de creep sur la base
            c.buildable = false;        // on ne construit PAS dessus (c’est la base)
        }
    }

    // petit cordon sanitaire autour (empêche arbres/buissons)
    for(int y = -1; y <= hTiles; ++y){
        for(int x = -1; x <= wTiles; ++x){
            int xx = x0+x, yy = y0+y;
            if(!m.inBounds(xx,yy)) continue;
            auto& c = m.at(xx,yy);
            if (c.ground == Tile::Water) { c.ground = Tile::Grass; c.walkable = true; c.buildable = true; }
            if (c.ground == Tile::Path)  { /* on laisse le path (si tu veux un parvis retire-le ici) *//* }*/
            // rend la couronne non-buildable pour éviter d’y coller une tour
          /*   if (c.ground == Tile::Grass) { c.buildable = false; }
        } 
    }
}*/


Point MapGenerator::placeResourceBaseArea(Map& m, int cx, int cy, int wTiles, int hTiles){
    int x0 = cx - wTiles/2;
    int y0 = cy - hTiles/2;
    x0 = std::max(0, std::min(x0, m.w - wTiles));
    y0 = std::max(0, std::min(y0, m.h - hTiles));

    // 1) Réserver le rectangle pour la base
    for(int y = 0; y < hTiles; ++y){
        for(int x = 0; x < wTiles; ++x){
            auto& c = m.at(x0+x, y0+y);
            c.ground    = Tile::Rock;   // FREE/pad
            c.walkable  = true;         // Allow creatures to walk on base for pathfinding
            c.buildable = false;
        }
    }

    // 2) Couronne de sécurité (pas de décor collé)
    for(int y = -1; y <= hTiles; ++y){
        for(int x = -1; x <= wTiles; ++x){
            int xx = x0+x, yy = y0+y;
            if(!m.inBounds(xx,yy)) continue;
            auto& c = m.at(xx,yy);
            if (c.ground == Tile::Water) { c.ground = Tile::Grass; c.walkable = true; c.buildable = true; }
            if (c.ground == Tile::Grass) { c.buildable = false; }
        }
    }

    // 3) Choisir une "porte" sur un côté du rectangle (au milieu d'un côté)
    std::vector<Point> candidates;
    // droite
    if (m.inBounds(x0+wTiles, y0 + hTiles/2)) candidates.push_back({x0+wTiles, y0 + hTiles/2});
    // gauche
    if (m.inBounds(x0-1, y0 + hTiles/2))      candidates.push_back({x0-1, y0 + hTiles/2});
    // bas
    if (m.inBounds(x0 + wTiles/2, y0+hTiles)) candidates.push_back({x0 + wTiles/2, y0+hTiles});
    // haut
    if (m.inBounds(x0 + wTiles/2, y0-1))      candidates.push_back({x0 + wTiles/2, y0-1});

    // garde : si eau / non-walkable, on cherche un voisin herbe
    auto pickUsable = [&](const Point& p)->Point{
        if (m.inBounds(p.x,p.y) && m.at(p.x,p.y).ground!=Tile::Water){
            return p;
        }
        for(auto q: neighbors4(p)){
            if (m.inBounds(q.x,q.y) && m.at(q.x,q.y).ground!=Tile::Water) return q;
        }
        return p;
    };

    std::uniform_int_distribution<size_t> rs(0, candidates.empty()?0:candidates.size()-1);
    Point gate = candidates.empty() ? Point{cx, y0+hTiles} : pickUsable(candidates[rs(rng_)]);

    // on force la porte en Path pour assurer la connexion
    if (m.inBounds(gate.x,gate.y)){
        auto& g = m.at(gate.x,gate.y);
        g.ground = Tile::Path; g.buildable = false; g.walkable = true;
    }
    return gate;
}

