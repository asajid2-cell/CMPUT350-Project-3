#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "Scene.h"
#include "ScoreComponent.h"
#include "ShaderManager.h"
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

// What a system is allowed to touch. Engine-owned resources (window, sounds, sprites) come from
// here; the entities a system works on come from the scene it runs in. The scene calls let a
// system stack a new scene on top (a pause screen) or end the current one.
template <typename... Components>
class EngineView {
public:
    virtual SoundManager& GetSoundManager() = 0;
    virtual SpriteManager& GetSpriteManager() = 0;
    virtual ShaderManager& GetShaderManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;

    // The scene currently being run (the top of the stack).
    virtual std::shared_ptr<SceneView<Components...>> GetScene() = 0;

    // Creates an empty scene that shares this engine's window. Add it with AddScene.
    virtual std::shared_ptr<SceneView<Components...>> MakeScene() = 0;

    // Adds a scene on top of the stack. It starts running (and drawing) on the next frame.
    virtual void AddScene(std::shared_ptr<SceneView<Components...>> scene) = 0;

    // Removes the scene on top of the stack.
    virtual void PopScene() = 0;
};

// The engine owns the window and the shared resource managers, and runs the scene stack. Each frame
// it drains events, clears the window, runs the top scene, and displays. Only the top scene runs;
// the scenes underneath are paused, waiting for it to pop.
template <typename... Components>
class ECSEngine : public EngineView<Components...> {
public:
    ECSEngine(unsigned int width, unsigned int height, const std::string& name);
    ~ECSEngine();

    // Runs until the window closes or the last scene pops itself.
    void Run();

    SoundManager& GetSoundManager() override { return mSoundManager; }

    SpriteManager& GetSpriteManager() override { return mSpriteManager; }

    ShaderManager& GetShaderManager() override { return mShaderManager; }

    WindowManager& GetWindowManager() override { return mWindowManager; }

    std::shared_ptr<SceneView<Components...>> GetScene() override { return mScenes.back(); }

    std::shared_ptr<SceneView<Components...>> MakeScene() override {
        return std::shared_ptr<SceneView<Components...>>(new ECSScene<Components...>(mWindowManager));
    }

    void AddScene(std::shared_ptr<SceneView<Components...>> scene) override {
        mNewScenes.push_back(std::move(scene));
    }

    void PopScene() override {
        if (!mScenes.empty()) mScenes.pop_back();
    }

private:
    bool ProcessEvents();

    SpriteManager mSpriteManager;
    SoundManager mSoundManager;
    ShaderManager mShaderManager;
    WindowManager mWindowManager;

    std::vector<std::shared_ptr<SceneView<Components...>>> mScenes;     // stack; back() runs
    std::vector<std::shared_ptr<SceneView<Components...>>> mNewScenes;  // added during a frame

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
    while (!mExit) {
        ProcessEvents();

        // Scenes added during the last frame join the stack now, so the run below always has one.
        for (auto& scene : mNewScenes) mScenes.push_back(std::move(scene));
        mNewScenes.clear();

        // Nothing left to run: the game is over.
        if (mScenes.empty()) break;

        mWindowManager.GetWindow()->clear();

        // Only the top scene runs. If it finishes, it comes off and whatever is underneath resumes.
        if (mScenes.back()->DoOneFrame(this) == kSceneDone) mScenes.pop_back();

        mWindowManager.GetWindow()->display();
    }
}

// Drain everything the OS queued since last frame. Key events are written straight into the
// InputComponent bitsets of the scene that is running, so the systems only ever look at the bitset
// and never at SFML.
template <typename... Components>
bool ECSEngine<Components...>::ProcessEvents() {
    auto window = mWindowManager.GetWindow();

    while (const std::optional event = window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            mExit = true;
        } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            // With no scene on the stack there is nothing to hold input yet.
            if (mScenes.empty()) continue;
            auto& entities = mScenes.back()->GetEntityManager();
            const size_t scan = static_cast<size_t>(key->scancode);
            for (const Entity& e : entities) {
                // .template is required here: EntityManager<Components...> is a dependent type, so
                // the compiler cannot know HasComponent names a template until it is instantiated.
                // MSVC lets this slide, GCC does not.
                if (entities.template HasComponent<InputComponent>(e.GetID()))
                    entities.template GetComponent<InputComponent>(e.GetID()).keydown.set(scan);
            }
        } else if (const auto* released = event->getIf<sf::Event::KeyReleased>()) {
            if (mScenes.empty()) continue;
            auto& entities = mScenes.back()->GetEntityManager();
            const size_t scan = static_cast<size_t>(released->scancode);
            for (const Entity& e : entities) {
                if (entities.template HasComponent<InputComponent>(e.GetID()))
                    entities.template GetComponent<InputComponent>(e.GetID()).keydown.reset(scan);
            }
        }
    }

    return !mExit;
}

}  // namespace ECSEngine
