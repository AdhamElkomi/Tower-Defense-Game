# 🏰 Tower Defense Game

> A competitive and strategic tower defense game with top-down vision, procedural map generation, player lives, and a global leaderboard.

---

## 🎯 Project Vision
The goal of this project is to design a **captivating and replayable tower defense game** where players must defend against waves of enemies, manage resources, and aim for the highest possible score.

Key elements:
- **Top-down map view**
- **Procedural generation** of maps: random spawn points, exits, and obstacles
- **Player lives** system: clear rules that punish mistakes but remain fair
- **Competitive scoring**: username input + persistent leaderboard
- **Compliance with supervisor Yohan’s requirements**

---

## 🧩 Core Gameplay Loop
1. Build towers strategically  
2. Defend against incoming waves  
3. Earn resources and score  
4. Reinforce defenses and survive  
5. Compare results on the leaderboard  

---

## 🗺️ Features & Milestones

### ✅ Milestone 0 – Setup
- GitHub repository, CI/CD, code guidelines

### 🎮 Milestone 1 – Core Gameplay
- Pathfinding (A*) for enemies
- Wave system
- Basic towers and shooting mechanics
- Player lives system

### 🏗️ Milestone 2 – Procedural Generation & UI
- Procedural map generation
- In-game HUD (lives, gold, wave counter, timers)
- Local leaderboard

### 🌐 Milestone 3 – Online & Polish
- Online leaderboard
- Performance optimization
- Audio (SFX/VFX)
- Final release build

---

## 🛠️ Tech Stack
- **Game engine:** (to be defined → Unity / Godot / Web)
- **Languages:** C++
- **Build & CI/CD:** GitHub Actions
- **Target platforms:** Windows, macOS, Linux

---

## 🧪 Quality Guidelines
- **Branching strategy:**  
  - `main`: stable  
  - `feat/*`, `fix/*`, `chore/*`  
- **Commits:** Conventional Commits  
- **Pull requests:** minimum 1 reviewer, all CI tests must pass  
- **Playtesting:** mandatory before merging into `main`  

---

## 🚀 Getting Started

### Prerequisites
- Install [Git](https://git-scm.com/)  
- Install the required engine/toolchain (Unity, Godot, Node.js, etc.)  

### Clone the repository
```bash
git clone https://github.com/<your-org>/tower-defense.git
cd tower-defense
