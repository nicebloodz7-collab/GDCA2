#include "GeodeEngine.hpp"

using namespace geode::prelude;

namespace sa::game {

void GeodeEngine::deliver(bool down, int button, bool p1) {
    if (orgDeliver) orgDeliver(down, button, p1);
}

void GeodeEngine::stepOnce(float dt, bool last) {
    if (orgStep) orgStep(dt, last);
}

int GeodeEngine::snapshot() {
    auto pl = PlayLayer::get();
    if (!pl) return -1;
    auto cp = pl->createCheckpoint(); // pasa por nuestro hook, que le anota el step en el tag
    if (!cp) return -1;
    snaps[next] = cp;
    return next++;
}

void GeodeEngine::restore(int handle) {
    auto it = snaps.find(handle);
    auto pl = PlayLayer::get();
    if (it == snaps.end() || !pl) return;
    internalLoad = true;
    pl->loadFromCheckpoint(it->second);
    internalLoad = false;
}

void GeodeEngine::release(int handle) { snaps.erase(handle); }

bool GeodeEngine::dead() {
    if (simDead) return true;
    auto pl = PlayLayer::get();
    return pl && pl->m_player1 && pl->m_player1->m_isDead;
}

// Solo silencia las muertes (ver hook de destroyPlayer). NO toca la cola de input:
// se llama desde dentro de handleButton, mientras el juego la está recorriendo.
void GeodeEngine::beginSim() {
    simulating = true;
    simDead = false;
}
void GeodeEngine::endSim() { simulating = false; }
void GeodeEngine::clearDead() { simDead = false; }

// Aparta la cola de input real para que los steps simulados no consuman clicks futuros.
void GeodeEngine::holdInput() {
    if (held || !layer) return;
    savedQueue = layer->m_queuedButtons;
    layer->m_queuedButtons.clear();
    held = true;
}
void GeodeEngine::releaseInput() {
    if (!held) return;
    if (layer) layer->m_queuedButtons = savedQueue;
    savedQueue.clear();
    held = false;
}

bool GeodeEngine::queueNonEmpty() const { return layer && !layer->m_queuedButtons.empty(); }

void GeodeEngine::releaseAll() {
    snaps.clear();
    simulating = simDead = internalLoad = held = false;
    savedQueue.clear();
}

} // namespace sa::game
