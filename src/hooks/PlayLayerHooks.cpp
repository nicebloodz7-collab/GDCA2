// Hooks de PlayLayer: ciclo de vida, muerte, checkpoints y actualización de la UI.
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "../game/Runtime.hpp"

using namespace geode::prelude;
using sa::game::Runtime;

class $modify(SAPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        Runtime::get().onLevelStart(this);
    }

    void resetLevel() {
        Runtime::get().onReset();
        PlayLayer::resetLevel();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        Runtime::get().onPostUpdate(this);
    }

    // Durante una simulación la muerte solo se ANOTA: sin sonido, partículas ni reinicio.
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& rt = Runtime::get();
        if (rt.engine.simulating) {
            rt.engine.simDead = true;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
        if (rt.levelReady) rt.gate.onDeath(); // -> rating MISS
    }

    // Si una simulación llegara al final del nivel, no debe completarlo de verdad.
    void levelComplete() {
        if (Runtime::get().engine.simulating) return;
        PlayLayer::levelComplete();
    }

    // Cada checkpoint (los nuestros y los de practice) guarda el step en que se creó.
    CheckpointObject* createCheckpoint() {
        auto cp = PlayLayer::createCheckpoint();
        auto& rt = Runtime::get();
        if (cp && rt.levelReady) cp->setTag(rt.gate.step);
        return cp;
    }

    // Si el JUEGO carga un checkpoint de practice, re-sincronizamos el contador y la macro.
    void loadFromCheckpoint(CheckpointObject* cp) {
        auto& rt = Runtime::get();
        PlayLayer::loadFromCheckpoint(cp);
        if (!rt.levelReady || rt.engine.internalLoad || !cp) return;
        int s = cp->getTag();
        if (s >= 0) rt.gate.externalRewind(s, &rt.engine);
    }

    void onQuit() {
        Runtime::get().onLevelExit();
        PlayLayer::onQuit();
    }
};
