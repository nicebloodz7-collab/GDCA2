// Hooks de GJBaseGameLayer: input, física (step por step) y tiempo de frame (lag).
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "../game/Runtime.hpp"

using namespace geode::prelude;
using sa::game::Runtime;

class $modify(SABaseLayer, GJBaseGameLayer) {
    // Todos los clicks (press/release) pasan por acá, ya dentro del step en el que se procesan.
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& rt = Runtime::get();
        if (rt.active(this) && rt.gate.onButton(down, button, isPlayer1)) return; // retrasado / resuelto por el núcleo
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    // 2.2081: processCommands(dt, isHalfTick, isLastTick) -> una llamada por step de física
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        auto& rt = Runtime::get();
        if (!rt.active(this)) return GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);

        // Los half ticks no cuentan como step de la macro: pasan directo al juego.
        if (isHalfTick) {
            rt.halfTicks++;
            return GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        }
        rt.fullTicks++;

        auto& e = rt.engine;
        e.layer = this;
        e.orgDeliver = [this](bool d, int b, bool p1) { GJBaseGameLayer::handleButton(d, b, p1); };
        e.orgStep = [this](float t, bool last) { GJBaseGameLayer::processCommands(t, false, last); };

        rt.gate.runStep(e, dt, isLastTick, e.queueNonEmpty());
        rt.trackPlayer(this);
    }

    // Tiempo real de cada frame renderizado (antes de que el juego simule sus steps).
    void update(float dt) {
        auto& rt = Runtime::get();
        if (rt.active(this)) rt.onFrame(dt);
        GJBaseGameLayer::update(dt);
    }
};
