# TODO: Update Score on Kills

## Tasks
- [x] Add kill counters (golemKills_, gruntKills_, rogueKills_) and public getters to CreatureSystem.hpp
- [x] Update CreatureSystem::applyDamagePoint signature to remove int& parameters and increment internal counters in CreatureSystem.cpp
- [x] Update all applyDamagePoint calls in Defense.cpp to remove dummy arguments
- [x] Update all applyDamagePoint calls in GameScene.cpp to remove dummy arguments
- [x] In GameScene::update, after creeps_.update, update kill counters from CreatureSystem and recalculate score_
- [ ] Test the game to ensure score updates correctly on kills
- [ ] Verify leaderboard or score display works
