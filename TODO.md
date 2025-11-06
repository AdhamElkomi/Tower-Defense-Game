# Leaderboard Implementation TODO

## 1. Add Dependencies
- [ ] Update CMakeLists.txt to include sqlite3 and nlohmann_json via vcpkg

## 2. Create LeaderboardController
- [ ] Create include/LeaderboardController.hpp
- [ ] Create src/LeaderboardController.cpp
- [ ] Implement username validation/normalization (isValidUsername, normalizeUsername)
- [ ] Implement data persistence (SQLite preferred, JSON fallback)
- [ ] Implement upsert logic (keep max score per username/difficulty)
- [ ] Implement queries (top N global/per difficulty, today, search, pagination, sorting)
- [ ] Implement export/import JSON
- [ ] Add anti-cheat hooks (stub IScoreValidator)

## 3. Create LeaderboardScene
- [ ] Create include/LeaderboardScene.hpp
- [ ] Create src/LeaderboardScene.cpp
- [ ] Implement UI: header, tabs (Global, By Difficulty, Today), list (rank, username, score, difficulty, date)
- [ ] Implement pagination (page size 10/25, next/prev)
- [ ] Implement sorting (by score desc, by date desc)
- [ ] Implement search by partial username
- [ ] Implement footer ("Press ESC to return")

## 4. Modify MenuScene
- [ ] Add circular leaderboard button (bordeaux #6E001A, icon assets/ui/leaderboard_icon.png, diameter 88-120px, hover/click effects)
- [ ] Add username prompt overlay (title "Enter Username", text field with live validation, OK/Back buttons)
- [ ] Implement username input handling and validation
- [ ] Modify flow: after difficulty/start, show username panel instead of launching game immediately

## 5. Modify App
- [ ] Update include/App.hpp: add UsernamePrompt and Leaderboard states
- [ ] Update src/App.cpp: handle new states and transitions
- [ ] Add logic for switching to UsernamePrompt after difficulty selection
- [ ] Add logic for switching to LeaderboardScene from menu

## 6. Modify GameScene
- [ ] Add username and difficulty members to GameScene
- [ ] Implement score calculation on game over (sum kills * weights: Golem=10, Grunt=4, Rogue=1)
- [ ] Call LeaderboardController.upsertBest on game over
- [ ] Pass username and difficulty to GameScene constructor

## 7. Integrate Flow
- [ ] Ensure after username OK, start game with chosen difficulty
- [ ] Ensure on game over, upsert score
- [ ] Ensure leaderboard button opens LeaderboardScene
- [ ] Test full flow: Menu -> Difficulty/Start -> Username -> Game -> Game Over -> Back to Menu -> Leaderboard

## Followup Steps
- [ ] Install dependencies (vcpkg install sqlite3 nlohmann-json)
- [ ] Test username validation
- [ ] Test leaderboard queries and upsert
- [ ] Verify UI rendering and interactions
- [ ] Test anti-cheat hooks (stub)
