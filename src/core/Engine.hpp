#pragma once
// Lo que el núcleo necesita del juego. En Geode lo implementa game/GeodeEngine;
// en los tests, un motor de física falso.
namespace sa {

struct Engine {
    virtual ~Engine() = default;
    virtual void deliver(bool down, int button, bool p1) = 0; // handleButton ORIGINAL
    virtual void stepOnce(float dt, bool last) = 0;           // processCommands ORIGINAL (last = isLastTick)
    virtual int snapshot() = 0;                               // estado al inicio del step actual -> handle o -1
    virtual void restore(int handle) = 0;
    virtual void release(int handle) = 0;
    virtual bool dead() = 0;       // ¿murió (real o simulado)?
    virtual void beginSim() = 0;   // desde aquí una muerte NO tiene efectos reales (solo se anota)
    virtual void endSim() = 0;
    virtual void clearDead() = 0;
    // Aparta/devuelve la cola de input del juego. Solo se llama con el step TERMINADO
    // (nunca desde dentro de handleButton: el juego está recorriendo esa cola).
    virtual void holdInput() = 0;
    virtual void releaseInput() = 0;
};

} // namespace sa
