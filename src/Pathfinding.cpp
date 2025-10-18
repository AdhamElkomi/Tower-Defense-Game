#include "Pathfinding.hpp"
#include "MapGenerator.hpp" 
#include <queue>
#include <unordered_map>
#include <cmath>
#include <algorithm>

struct Node {
    int x,y;
};
static inline int idOf(int x,int y,int W){ return y*W+x; }
static inline int manhattan(sf::Vector2i a, sf::Vector2i b){
    return std::abs(a.x-b.x) + std::abs(a.y-b.y);
}

TilePath astarOnGrid(
    const Map& map,
    sf::Vector2i start,
    sf::Vector2i goal,
    const WalkableFn& isWalkable,
    const OccupancyGrid* occ)
{
    const int W = map.w, H = map.h;
    auto inB = [&](int x,int y){ return x>=0 && y>=0 && x<W && y<H; };

    auto passable = [&](int x,int y)->bool{
        if (!inB(x,y)) return false;
        if (occ && occ->blocked(x,y)) return false;
        return isWalkable(map.at(x,y), x, y);
    };

    if (!passable(start.x,start.y) || !passable(goal.x,goal.y))
        return {};

    // A* open set (min-heap sur f = g + h)
    struct QNode { int id; int f; };
    auto cmp = [](const QNode& a, const QNode& b){ return a.f > b.f; };
    std::priority_queue<QNode, std::vector<QNode>, decltype(cmp)> open(cmp);

    const int N = W*H;
    std::vector<int> g(N, std::numeric_limits<int>::max());
    std::vector<int> parent(N, -1);
    std::vector<uint8_t> inOpen(N, 0);

    const int sid = idOf(start.x,start.y,W);
    const int gid = idOf(goal.x, goal.y, W);

    g[sid] = 0;
    open.push({sid, manhattan(start, goal)});
    inOpen[sid] = 1;

    const int dx[4]={1,-1,0,0};
    const int dy[4]={0,0,1,-1};

    while(!open.empty()){
        int cur = open.top().id; open.pop();
        inOpen[cur] = 0;
        if (cur == gid) break;

        int cx = cur % W, cy = cur / W;
        for (int k=0;k<4;++k){
            int nx=cx+dx[k], ny=cy+dy[k];
            if (!passable(nx,ny)) continue;
            int nid = idOf(nx,ny,W);

            int ng = g[cur] + 10; // coût uniforme
            if (ng < g[nid]){
                g[nid] = ng;
                parent[nid] = cur;
                int h = 10 * manhattan({nx,ny}, goal);
                int f = ng + h;
                if (!inOpen[nid]){
                    open.push({nid,f});
                    inOpen[nid]=1;
                }
            }
        }
    }

    if (parent[gid] == -1 && gid != sid) return {};

    // Reconstruit chemin
    TilePath tp;
    int cur = gid;
    while (cur != -1){
        int x = cur % W, y = cur / W;
        tp.push_back({x,y});
        if (cur == sid) break;
        cur = parent[cur];
    }
    std::reverse(tp.begin(), tp.end());
    return tp;
}

TilePath routeViaBaseToBestExit(
    const Map& map,
    sf::Vector2i start,
    const std::vector<sf::Vector2i>& baseTiles,
    const std::vector<sf::Vector2i>& exits,
    const WalkableFn& isWalkable,
    const OccupancyGrid* occ)
{
    if (baseTiles.empty() || exits.empty()) return {};

    TilePath best;
    int bestCost = std::numeric_limits<int>::max();

    // Find the closest base tile to start
    sf::Vector2i closestBase = baseTiles[0];
    int minDistToBase = std::numeric_limits<int>::max();
    for (auto b : baseTiles) {
        auto pathToBase = astarOnGrid(map, start, b, isWalkable, occ);
        if (!pathToBase.empty()) {
            int dist = pathToBase.size();
            if (dist < minDistToBase) {
                minDistToBase = dist;
                closestBase = b;
            }
        }
    }

    // From closest base, find the best exit (shortest path, different from entry if possible)
    for (auto e : exits) {
        // Skip if exit is the same as start (entry)
        if (e == start) continue;

        auto b2e = astarOnGrid(map, closestBase, e, isWalkable, occ);
        if (b2e.empty()) continue;

        int cost = b2e.size();
        if (cost < bestCost) {
            bestCost = cost;
            auto s2b = astarOnGrid(map, start, closestBase, isWalkable, occ);
            best = s2b;
            // Concat without duplicating the base
            best.insert(best.end(), b2e.begin() + 1, b2e.end());
        }
    }

    // If no path found, try any exit
    if (best.empty()) {
        for (auto e : exits) {
            auto b2e = astarOnGrid(map, closestBase, e, isWalkable, occ);
            if (b2e.empty()) continue;

            int cost = b2e.size();
            if (cost < bestCost) {
                bestCost = cost;
                auto s2b = astarOnGrid(map, start, closestBase, isWalkable, occ);
                best = s2b;
                best.insert(best.end(), b2e.begin() + 1, b2e.end());
            }
        }
    }

    return best;
}
