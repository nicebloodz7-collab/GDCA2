#pragma once
#include <Geode/Geode.hpp>

#include <functional>
#include <map>

#include "../core/Engine.hpp"

namespace sa::game {

// Adaptador: implementa sa::Engine sobre el juego real.
// Snapshots = CheckpointObject (createCheckpoint / loadFromCheckpoint).
class GeodeEngine : public sa::Engine {
public:
    GJBaseGameLayer* layer = nullptr;
    std::function<void(bool, int, bool)> orgDeliver; // GJBaseGameLayer::handleButton original
    std::function<void(float, bool)> orgStep;        // GJBaseGameLayer::processCommands original (dt, isLastTick)

    bool simulating = false;   // las muertes se anotan en simDead en vez de ejecutarse
    bool simDead = false;
    bool internalLoad = false; // true mientras NOSOTROS cargamos un snapshot

    void deliver(bool down, int button, bool p1) override;
    void stepOnce(float dt, bool last) override;
    int snapshot() override;
    void restore(int handle) override;
    void release(int handle) override;
    bool dead() override;
    void beginSim() override;
    void endSim() override;
    void clearDead() override;
    void holdInput() override;
    void releaseInput() override;

    bool queueNonEmpty() const;
    void releaseAll();

private:
    std::map<int, geode::Ref<CheckpointObject>> snaps;
    int next = 0;
    gd::vector<PlayerButtonCommand> savedQueue;
    bool held = false;
};

} // namespace sa::game
