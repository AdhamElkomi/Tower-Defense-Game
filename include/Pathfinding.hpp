#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <limits>

struct Map;      // ton type déjà existant
struct Cell;     // Map::at(x,y) -> Cell (contient ground, buildable, etc.)

// Représente un chemin en tuiles
using TilePath = std::vector<sf::Vector2i>;

// Pour convertir un chemin tuile -> coordonnées pixel (centres)
inline std::vector<sf::Vector2f> tilePathToPixels(const TilePath& tp, float tileSize){
    std::vector<sf::Vector2f> out; out.reserve(tp.size());
    for (auto p : tp) out.emplace_back((p.x+0.5f)*tileSize, (p.y+0.5f)*tileSize);
    return out;
}

// ====== Configuration de passabilité ======
// Fonction callback: true => tuile marchable pour les créatures
using WalkableFn = std::function<bool(const Cell& cell, int x, int y)>;

// Occupation dynamique (tours, obstacles temporaires)
struct OccupancyGrid {
    int W=0, H=0;
    // 0 = libre, 1 = bloqué
    std::vector<uint8_t> occ; // taille W*H
    bool blocked(int x,int y) const {
        if (x<0||y<0||x>=W||y>=H) return true;
        return occ[y*W+x] != 0;
    }
};

// ====== A* simple 4-neighbors ======
TilePath astarOnGrid(
    const Map& map,
    sf::Vector2i start,
    sf::Vector2i goal,
    const WalkableFn& isWalkable,
    const OccupancyGrid* occ // peut être nullptr si pas d’occupation dynamique
);

// ====== Routage contraint Base ======
// `baseTiles`: ensemble des tuiles considérées comme « base de ressources » (zone)
// `exits`: liste des tuiles qui sont des sorties possibles
// Retourne chemin complet Entrée->BaseTileChoisie->SortieChoisie le plus court (si existe)
TilePath routeViaBaseToBestExit(
    const Map& map,
    sf::Vector2i start,
    const std::vector<sf::Vector2i>& baseTiles,
    const std::vector<sf::Vector2i>& exits,
    const WalkableFn& isWalkable,
    const OccupancyGrid* occ
);
