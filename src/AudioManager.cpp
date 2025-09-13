#include "AudioManager.hpp"
#include <algorithm>

bool AudioManager::load() {
    if (!menuMusic_.openFromFile("../assets/sounds/menu_theme.ogg")) return false;
    if (!gameMusic_.openFromFile("../assets/sounds/game_theme.ogg")) return false;
    if (!clickBuf_.loadFromFile("assets/sfx/click.wav")) return false;

    click_.emplace(clickBuf_); // ✅ ok car click_ est std::optional<sf::Sound>
    return true;
}

void AudioManager::playMenuLoop(float volume01) {
    musicVol_ = std::clamp(volume01, 0.f, 1.f);
    gameMusic_.stop();
    menuMusic_.setLooping(true);                    // ✅ SFML 3
    menuMusic_.setVolume(musicVol_ * 100.f);       // ✅ setVolume (sans typo)
    menuMusic_.play();
}

void AudioManager::playGameLoop(float volume01) {
    musicVol_ = std::clamp(volume01, 0.f, 1.f);
    menuMusic_.stop();
    gameMusic_.setLooping(true);                   // ✅
    gameMusic_.setVolume(musicVol_ * 100.f);
    gameMusic_.play();
}

void AudioManager::setMusicVolume(float v) { musicVol_ = std::clamp(v, 0.f, 1.f); }
void AudioManager::setSfxVolume(float v)    { sfxVol_   = std::clamp(v, 0.f, 1.f); }

void AudioManager::stopAll() { menuMusic_.stop(); gameMusic_.stop(); }

void AudioManager::playClick() {
    if (click_) {
        click_->setVolume(sfxVol_ * 100.f);
        click_->play();
    }
}
