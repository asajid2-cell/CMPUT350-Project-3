#pragma once

#include <memory>

#include "EntityManager.h"
#include "SystemManager.h"
#include "WindowManager.h"

namespace ECSEngine {

template <typename... Components>
class EngineView;

// What a scene reports after a frame. kSceneContinue keeps it on the stack; kSceneDone removes it,
// which reveals whatever was underneath (a pause screen popping back to the game).
enum tSceneReturn { kSceneContinue, kSceneDone };

// One independent piece of the game: the menu, the level, a pause screen. Each scene owns its own
// entities and its own systems, so two scenes never see each other's entities. A scene shares the
// engine's window but keeps its own camera (view centre and scale).
//
// Systems reach a scene through this interface and get the engine separately, so a system can read
// the scene's entities while still using engine-owned resources such as sounds.
template <typename... Components>
class SceneView {
public:
    virtual ~SceneView() = default;

    virtual EntityManager<Components...>& GetEntityManager() = 0;
    virtual WindowManager& GetWindowManager() = 0;
    virtual SystemManager<Components...>& GetSystemManager() = 0;

    // Runs one frame of this scene's systems. Returns kSceneDone once a system asks to exit; the
    // engine then pops the scene.
    virtual tSceneReturn DoOneFrame(EngineView<Components...>* engine) = 0;
};

// The one concrete scene. It holds the managers the spec asks a scene to own, and it is created
// through ECSEngine::MakeScene so nothing else has to know its type.
template <typename... Components>
class ECSScene : public SceneView<Components...> {
public:
    EntityManager<Components...>& GetEntityManager() override { return mEntityManager; }
    WindowManager& GetWindowManager() override { return mWindowManager; }
    SystemManager<Components...>& GetSystemManager() override { return mSystemManager; }

    // Hands this scene to its own systems with both views: engine resources and this scene. The
    // system manager reports whether it wants to keep running, so a false means the scene is done.
    [[nodiscard]] tSceneReturn DoOneFrame(EngineView<Components...>* engine) override {
        if (!mSystemManager.Run(engine, this)) return kSceneDone;
        return kSceneContinue;
    }

private:
    // Only the engine creates scenes, and it does it through MakeScene.
    template <typename... T>
    friend class ECSEngine;

    // The scene copies the engine's manager: same window, and the camera the engine started with.
    explicit ECSScene(WindowManager& windowManager) : mWindowManager(windowManager) {}

    EntityManager<Components...> mEntityManager;
    SystemManager<Components...> mSystemManager;
    WindowManager mWindowManager;
};

}  // namespace ECSEngine
