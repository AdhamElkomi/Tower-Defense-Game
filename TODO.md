# TODO: Implement Mage Tower Hold Attack Animation

## 1. Update Defense.hpp
- [x] Add MageFireProjectile struct with pos, vel, life, alive, angle
- [x] Add to MageTower: bool isHolding_, float holdTime_, float maxHold_, std::vector<MageFireProjectile> projectiles_
- [x] Add canHold() method to Tower base class
- [x] Fix MageTower radius() to return radius_

## 2. Update Defense.cpp
- [x] Initialize hold variables in MageTower constructor
- [x] Implement update: increment holdTime if holding, clamp to maxHold
- [x] Implement tryFireAt: calculate power, spawn projectiles in semicircle, reset hold
- [x] Implement draw: show power gauge if holding, draw projectiles as orange circles
- [x] Add update logic for projectiles: move, check hits, apply damage

## 3. Update GameScene.cpp
- [x] Add hold start on leftDown for Mage towers
- [x] Add fire on leftUp if holding for Mage
- [x] Ensure aiming logic works with hold

## 4. Test and Refine
- [x] Test hold mechanics: gauge appears, fills on hold
- [x] Test fire: arc spawns, propagates, hits creatures
- [x] Adjust visuals for professionalism (colors, sizes)
- [x] Ensure no crashes, balance power

## 5. Calibrate Flame Power and Size
- [x] Add powerMultiplier to MageFireProjectile
- [x] Set powerMultiplier in tryFireAt based on holdTime_
- [x] Vary projectile size in draw based on powerMultiplier
- [x] Use stored powerMultiplier for damage in update

## 6. Modify Mage Tower to Fire Only When Gauge Full
- [x] Change firing logic: hold to fill gauge, click to fire only when full
- [x] Update handleInput: on leftDown, if gauge full, fire; else start holding
- [x] Update tryFireAt: only fire if holdTime_ >= maxHold_
- [x] Reset holdTime_ after firing
