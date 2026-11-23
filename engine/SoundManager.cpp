#include "SoundManager.h"

#include <cassert>

namespace ECSEngine {

void SoundManager::RegisterSound(const std::string& soundPath, const std::string& soundName) {
    // Already decoded under this name: keep the existing buffer and voice, do nothing. That is what
    // makes a second call harmless rather than a reload.
    if (mBuffers.find(soundName) != mBuffers.end()) return;

    // operator[] on the map default-constructs the buffer in place and hands back a reference.
    // Because the map owns it, the buffer's address is stable for the manager's whole lifetime --
    // which is the property sf::Sound depends on, since it stores a pointer to it.
    sf::SoundBuffer& buffer = mBuffers[soundName];
    const bool loaded = buffer.loadFromFile(soundPath);
    assert(loaded && "SoundManager::RegisterSound could not load the file");
    (void)loaded;  // in a release build an unloaded buffer just plays silence

    mSounds.emplace(soundName, sf::Sound{buffer});
}

sf::Sound& SoundManager::GetSound(const std::string& soundName) {
    auto it = mSounds.find(soundName);
    assert(it != mSounds.end() && "SoundManager::GetSound: sound was never registered");
    return it->second;
}

bool SoundManager::HasSound(const std::string& soundName) const {
    return mSounds.find(soundName) != mSounds.end();
}

}  // namespace ECSEngine
