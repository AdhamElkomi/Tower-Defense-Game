#pragma once
#include "Map.hpp"
#include <random>
#include <vector>

struct EntryExit { Point p; };

class MapGenerator {
public:
    explicit MapGenerator(uint32_t seed = std::random_device{}());
    Map generate(int w, int h, int entriesMin=1, int entriesMax=2, int exitsMin=1, int exitsMax=2);

private:
    std::mt19937 rng_;

    std::vector<Point> randomEdgePoints(int w,int h,int count);
    bool carvePath(Map& m, Point a, Point b);  // A* ou BFS simplifié
    void placeForbiddenPatches(Map& m, int count, int radius);
    void sprinkleBlockers(Map& m, int trees, int rocks, int bush);
};
