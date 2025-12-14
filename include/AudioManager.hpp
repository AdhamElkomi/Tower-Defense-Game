#pragma once
#include <SFML/Audio.hpp>
#include <optional>

class AudioManager {
public:
bool load();
void playMenuLoop(float volume01);
void playGameLoop(float volume01);
void stopAll();

void setMusicVolume(float volume01); // 0..1
void setSfxVolume(float volume01); // 0..1
void setGameLoopVolume(float volume01); // 0..1 - adjust current game loop volume

void playClick();


private:
sf::Music menuMusic_;
sf::Music gameMusic_;
sf::SoundBuffer clickBuf_;
 std::optional<sf::Sound> click_; // ✅ emplaced après chargement


float musicVol_{0.7f};
float sfxVol_{0.8f};
};

