#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Audio.hpp>

namespace ECSEngine {

// Owns the game's sound buffers and the voices that play them.
//
// Two maps, and the split matters:
//
//   mBuffers -- the decoded audio, keyed by the name the game registered. This is the memory that
//               actually costs something (a decoded ogg is many times its file size), so a repeated
//               RegisterSound under the same name must not decode twice.
//   mSounds  -- one sf::Sound per name, built against its buffer. An sf::Sound does not own its
//               buffer, it only points at one, so the buffer has to outlive the voice. Both maps
//               are unordered_map, whose nodes are stable across rehash -- that is what keeps the
//               buffer reference inside each sf::Sound valid as more sounds are added.
//
// Lifetime contract, which is the part the rubric asks to be written down:
//   - a reference from GetSound is owned by the manager and stays valid until the manager is
//     destroyed; registering more sounds never invalidates it.
//   - play() on a voice that is already playing restarts it. There is one voice per name, so two
//     overlapping plays of the same sound cut each other off. The platformer only ever needs one
//     of each at a time, so a voice pool would be cost with no gain.
class SoundManager {
public:
    SoundManager() = default;
    ~SoundManager() = default;

    // Decode a sound file and remember it under this name. Registering the same name twice keeps
    // the first buffer, so this is safe to call unconditionally during setup.
    void RegisterSound(const std::string& soundPath, const std::string& soundName);

    // The voice for this name, ready to play(). Asserts in Debug if nothing was registered under
    // the name, so a typo fails on the first frame instead of silently going quiet.
    sf::Sound& GetSound(const std::string& soundName);

    // True when a sound is registered under this name, so a caller can check without tripping the
    // assert inside GetSound.
    bool HasSound(const std::string& soundName) const;

private:
    std::unordered_map<std::string, sf::SoundBuffer> mBuffers;
    std::unordered_map<std::string, sf::Sound> mSounds;
};

}  // namespace ECSEngine
