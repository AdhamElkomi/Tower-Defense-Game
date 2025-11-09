# TODO: Implement Leaderboard System

## Tasks
- [x] Modify src/GameScene.cpp: Add code to save/update leaderboard data in docs/leaderboard.txt when game over occurs. Format: username,score,difficulty,date (YYYY-MM-DD HH:MM:SS). Update if same username exists with higher score.
- [x] Modify src/LeaderboardScene.cpp: Load entries from docs/leaderboard.txt, sort by score descending, compute ranks, and display in the list.

## Followup
- [ ] Test saving at game over and loading in leaderboard scene.
- [ ] Ensure date format is consistent.
