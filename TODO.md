# TODO: Implement Difficulty Selection Menu and Parameters

## 1. Update MenuScene to Show Difficulty Submenu
- Modify btnDifficulty_ callback to open a submenu instead of cycling.
- Add a difficulty submenu panel to the left of the main menu.
- Add buttons for Easy, Normal, Hard, Legendary in the submenu.
- When a difficulty is selected, close submenu, set difficulty_, and launch game.

## 2. Update GameScene to Accept and Apply Difficulty
- Modify GameScene constructor to take a difficulty string.
- Add logic to apply enemy, wave, economy, player parameters based on difficulty.
- Update wave generation, creature spawning, resource counts, etc., using the multipliers.

## 3. Update App.cpp to Pass Difficulty
- Modify goToGame() to pass the selected difficulty from MenuScene to GameScene.

## 4. Test and Verify
- Test menu appearance and selection.
- Test game launch with each difficulty.
- Verify parameters are applied (e.g., enemy HP, speed, waves, resources).
