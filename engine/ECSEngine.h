#pragma once

#include <optional>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SystemManager.h"
#include "WindowManager.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>

namespace ECSEngine {

// What a system is allowed to touch. A system is handed one of these instead of the engine itself,
// so it reaches the four resource managers only through this interface and never owns them.
template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
};

// The engine. It owns the five managers (the four in EngineView plus the system manager) and runs
// the frame loop: drain events, clear the window, then let the systems draw and update.
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    // Runs until the window is closed or a system returns kExit.
    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    // The system manager is not part of EngineView: systems are added by the game, not by a system.
    SystemManager<Components...>& GetSystemManager() { return mSystemManager; }

private:
    bool ProcessEvents();

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    WindowManager mWindowManager;

    bool mExit = false;  // set when the window closes; ProcessEvents reports it to Run
};

template <typename... Components>
ECSEngine<Components...>::ECSEngine(unsigned int width, unsigned int height,
                                    const std::string& name)
    : mWindowManager(width, height, name) {}

template <typename... Components>
ECSEngine<Components...>::~ECSEngine() {}

template <typename... Components>
void ECSEngine<Components...>::Run() {
    // mExit is set by ProcessEvents when the window closes. It carries the loop rather than
    // IsOpen(), so a close still gets one clean frame of shutdown instead of tearing the window out
    // from under the systems.
    while (!mExit) {
        ProcessEvents();

        mWindowManager.GetWindow()->clear();
        if (!mSystemManager.Run(this)) break;
    }
}

// Drain everything the OS queued since last frame. Key events are written straight into every
// InputComponent's bitset, so the systems later in the frame only ever look at the bitset and never
// at SFML.
template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();

    while (const std::optional event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            mExit = true;
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            const size_t scan = static_cast<size_t>(key->scancode);
            for (const Entity& e : mEntityManager) {
                // .template is required here: EntityManager<Components...> is a dependent type, so
                // the compiler cannot know HasComponent names a template until it is instantiated.
                // MSVC lets this slide, GCC does not.
                if (mEntityManager.template HasComponent<InputComponent>(e.GetID()))
                    mEntityManager.template GetComponent<InputComponent>(e.GetID()).keydown.set(scan);
            }
        } else if (const auto* released = event->getIf<sf::Event::KeyReleased>()) {
            const size_t scan = static_cast<size_t>(released->scancode);
            for (const Entity& e : mEntityManager) {
                if (mEntityManager.template HasComponent<InputComponent>(e.GetID()))
                    mEntityManager.template GetComponent<InputComponent>(e.GetID()).keydown.reset(scan);
            }
        }
    }

    return !mExit;
}

}  // namespace ECSEngine
