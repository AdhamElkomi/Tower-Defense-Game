#pragma once
#include "Map.hpp"
#include <random>
#include <vector>
#include <memory>

struct EntryExit { Point p; };

class MapGenerator {
public:
    explicit MapGenerator(uint32_t seed = std::random_device{}());

    // Génère une carte w×h :
    // - chemins depuis 'entriesMin..Max' entrées vers centre puis centre -> 'exitsMin..Max' sorties
    // - patches d’eau (lacs/rivières)
    // - pads de construction (zones libres) à 1 tuile des chemins
    // - décor (arbres/buissons)
    Map generate(int w, int h,
                 int entriesMin=1, int entriesMax=2,
                 int exitsMin=1,   int exitsMax=2);
    

private:
    std::mt19937 rng_;

    // ---- Chemins ----
    std::vector<Point> randomEdgePoints(int w,int h,int count);
    bool carvePath(Map& m, Point a, Point b);  // BFS simplifié

    // ---- Eau (forbidden walk) ----
    // Place des patches d’eau de rayon ~radius (avec légère irrégularité)
    void placeForbiddenPatches(Map& m, int count, int radius);

    // ---- Décor ----
    // NB: 'rocks' est ignoré (déprécié) car Tile::Rock = zone libre (FREE) désormais.
    void sprinkleBlockers(Map& m, int trees, int rocks /*ignored*/, int bush);

    // ---- Pads buildables proches des chemins (zones libres) ----
    // Crée des plateformes rectangulaires (3×3, 2×3, 2×2) à ~1 tuile du path,
    // alignées grille, hors eau et sans chevauchement.
    // Les pads sont posés en Tile::Rock (FREE), buildable=true, walkable=false.
    void placeBuildPadsNearPaths(Map& m,
                                 int targetPads = 10, // nombre de pads visés
                                 int step       = 5,  // espacement le long du path
                                 int offset     = 1   // distance au path en tuiles
                                 );

    // Était : void placeResourceBaseArea(Map& m, int cx, int cy, int wTiles=3, int hTiles=3);
    // Devient :
    Point placeResourceBaseArea(Map& m, int cx, int cy, int wTiles=3, int hTiles=3);


};


