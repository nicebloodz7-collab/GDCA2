#include "Runtime.hpp"

#include <fstream>
#include <sstream>

#include "MacroReader.hpp"
#include "Settings.hpp"

using namespace geode::prelude;

namespace sa::game {

Runtime& Runtime::get() {
    static Runtime rt;
    return rt;
}

bool Runtime::needGate() const {
    auto const& c = gate.cfg;
    return c.clickAssist || c.lagComp || (macroLoaded && (c.hitRating || c.guide != sa::GuideMode::Off));
}

bool Runtime::active(GJBaseGameLayer* layer) const {
    return levelReady && layer == PlayLayer::get() && needGate();
}

std::filesystem::path Runtime::profilePath(uint32_t id) const {
    return Mod::get()->getSaveDir() / fmt::format("adaptive-{}.txt", id);
}

void Runtime::onLevelStart(PlayLayer* pl) {
    gate.cfg = readConfig();
    gate.lag.configure(gate.cfg.lagThresholdMs, gate.cfg.lagMaxBonus);
    levelID = pl->m_level ? (uint32_t)pl->m_level->m_levelID.value() : 0;

    // ---- macro
    macroLoaded = false;
    gate.setMacro(sa::Macro{});
    auto path = Mod::get()->getSettingValue<std::filesystem::path>("macro-path");
    auto& c = gate.cfg;
    bool needsMacro = c.guide != sa::GuideMode::Off || (c.clickAssist && c.mode != sa::AssistMode::Trajectory);

    if (!path.empty()) {
        auto res = readMacro(path);
        if (res.ok()) {
            auto levelOfMacro = res.macro.levelID;
            gate.setMacro(std::move(res.macro));
            macroLoaded = true;
            auto msg = fmt::format("Showcase Assist: {} clicks cargados", gate.macroRef().clicks.size());
            if (levelOfMacro != 0 && levelOfMacro != levelID) msg += " (¡ojo! la macro es de otro nivel)";
            Notification::create(msg, NotificationIcon::Success)->show();
        } else {
            Notification::create("Showcase Assist: " + res.error, NotificationIcon::Error)->show();
        }
    } else if (needsMacro) {
        Notification::create("Showcase Assist: elegí una macro en los ajustes del mod", NotificationIcon::Warning)->show();
    }

    // ---- perfil del asistente adaptativo (se guarda por nivel)
    gate.profile.clear();
    if (c.adaptive) {
        std::ifstream f(profilePath(levelID));
        if (f) {
            std::stringstream ss;
            ss << f.rdbuf();
            gate.profile.deserialize(ss.str());
        }
    }

    gate.onRating = [this](sa::RatingEvent const& ev) { ratingQueue.push_back(ev); };
    ratingQueue.clear();
    hud.attach(pl);

    fullTicks = halfTicks = squeezed = 0;
    haveLastX = false;
    dxPerStep = 0.f;
    gate.reset(&engine);
    levelReady = true;
}

void Runtime::onReset() {
    if (!levelReady) return; // resetLevel también se llama dentro de setupHasCompleted
    if (gate.cfg.debugLog) {
        log::info("[ShowcaseAssist] intento: fullTicks={} halfTickCalls={} step={} sims={} simSteps={} delays={} "
                  "rollbacks={} squeezedRects={} lagSpikes={}",
                  fullTicks, halfTicks, gate.step, gate.stats.simulations, gate.stats.simSteps, gate.stats.delays,
                  gate.stats.rollbacks, squeezed, gate.lag.spikes());
    }
    gate.cfg = readConfig(); // los cambios de ajustes aplican al próximo intento
    gate.lag.configure(gate.cfg.lagThresholdMs, gate.cfg.lagMaxBonus);
    gate.reset(&engine);
    engine.releaseAll();
    ratingQueue.clear();
    fullTicks = halfTicks = squeezed = 0;
    gate.stats = {};
    haveLastX = false;
    dxPerStep = 0.f;
}

void Runtime::onFrame(double dt) {
    gate.lag.onFrame(dt);
    if (gate.cfg.lagComp && gate.cfg.lagIndicator && gate.lag.spikeNow()) {
        ratingQueue.push_back({sa::RatingKind::Lag, (int)gate.lag.lastSpikeMs()});
    }
}

void Runtime::trackPlayer(GJBaseGameLayer* layer) {
    if (!layer->m_player1) return;
    float x = layer->m_player1->getPositionX();
    if (haveLastX && x > lastX) {
        float dx = x - lastX;
        dxPerStep = dxPerStep == 0.f ? dx : dxPerStep * 0.9f + dx * 0.1f;
    }
    lastX = x;
    haveLastX = true;
}

void Runtime::onPostUpdate(PlayLayer* pl) {
    if (!levelReady) return;
    for (auto const& ev : ratingQueue) hud.rating(pl, ev);
    ratingQueue.clear();
    hud.update(pl, gate.macroRef(), gate.step, dxPerStep, gate.cfg);
}

void Runtime::onLevelExit() {
    if (levelReady && gate.cfg.adaptive && gate.profile.knownClicks() > 0) {
        std::ofstream f(profilePath(levelID));
        if (f) f << gate.profile.serialize();
    }
    hud.detach();
    engine.releaseAll();
    levelReady = false;
}

} // namespace sa::game
