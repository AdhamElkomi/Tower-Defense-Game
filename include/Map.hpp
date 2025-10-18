#pragma once
#include <cstdint>
#include <vector>

enum class Tile : uint8_t {
    Grass,       // walkable + buildable
    Path,        // walkable, no build
    Water,       // forbidden
    Forest,      // blocker (tree)
    Rock,        // blocker (rock)
    Bush         // decor (walkable, no build)
};

struct Cell {
    Tile ground{Tile::Grass};  // sol
    bool walkable{true};
    bool buildable{true};
};

struct Point { int x{}, y{}; };

struct Map {
    int w{}, h{};
    std::vector<Cell> cells; // size = w*h
    std::vector<Point> entries;
    std::vector<Point> exits;
    Map() = default;
    Map(int W, int H) : w(W), h(H), cells(W*H) {}
    inline int idx(int x,int y) const { return y*w + x; }
    inline bool inBounds(int x,int y) const { return x>=0 && y>=0 && x<w && y<h; }
    Cell& at(int x,int y) { return cells[idx(x,y)]; }
    const Cell& at(int x,int y) const { return cells[idx(x,y)]; }
};
