#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ECSEngine {

// What a system tells the manager after its update. kContinue runs the rest of the frame; kExit
// stops the frame loop.
enum tSystemReturn { kContinue, kExit };

template <typename... Components>
class EngineView;

// One step of the game loop. A system reads and writes components through the EngineView it is
// given and returns kContinue, or kExit to end the game.
template <typename... Components>
class System {
public:
    virtual ~System() = default;
    virtual tSystemReturn Run(EngineView<Components...>* scene) = 0;

    // Name used in diagnostics when a system asks the manager to exit.
    virtual const std::string& GetName() const = 0;
};

// Runs the systems in the order they were added, once per frame. The manager owns them, so adding
// a system transfers ownership of the unique_ptr.
template <typename... Components>
class SystemManager {
public:
    // Runs every system; returns false as soon as one returns kExit.
    bool Run(EngineView<Components...>* scene) {
        for (auto& system : mSystems) {
            if (system->Run(scene) == kExit) {
                std::cout << system->GetName() << " requested exit from system manager\n";
                return false;
            }
        }
        return true;
    }

    // Takes ownership of system and appends it to the update order.
    void AddSystem(std::unique_ptr<System<Components...>>&& system) {
        mSystems.push_back(std::move(system));
    }

    // Destroys every system. The engine does not do this between frames; it exists for teardown.
    void ClearSystems() { mSystems.clear(); }

private:
    std::vector<std::unique_ptr<System<Components...>>> mSystems;
};

}  // namespace ECSEngine
