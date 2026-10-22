#pragma once
#include <Geode/Geode.hpp>

#include <vector>

#include "../core/InputGate.hpp"
#include "../ui/Hud.hpp"
#include "GeodeEngine.hpp"

namespace sa::game {

// Estado global del mod (un solo PlayLayer a la vez). Los hooks solo llaman a esto.
class Runtime {
public:
    static Runtime& get();

    sa::InputGate gate;
    GeodeEngine engine;
    sa::ui::Hud hud;

    bool macroLoaded = false;
    bool levelReady = false;

    // medición de velocidad horizontal (para ubicar la guía vertical)
    float dxPerStep = 0.f, lastX = 0.f;
    bool haveLastX = false;

    // diagnóstico
    int fullTicks = 0, halfTicks = 0, squeezed = 0;

    std::vector<sa::RatingEvent> ratingQueue;

    sa::Config& cfg() { return gate.cfg; }

    // ¿Hay algo que hacer en el hook de física?
    bool needGate() const;
    bool active(GJBaseGameLayer* layer) const;

    void onLevelStart(PlayLayer* pl);
    void onReset();
    void onPostUpdate(PlayLayer* pl);
    void onLevelExit();
    void onFrame(double dt);
    void trackPlayer(GJBaseGameLayer* layer);

private:
    std::filesystem::path profilePath(uint32_t levelID) const;
    uint32_t levelID = 0;
};

} // namespace sa::game
