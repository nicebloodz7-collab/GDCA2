#pragma once
#include "../core/Config.hpp"
#include "../core/HitRating.hpp"
#include "../core/Macro.hpp"
#include "GuideVertical.hpp"
#include "RatingPopup.hpp"
#include "RhythmBar.hpp"

namespace sa::ui {

// Agrupa los elementos visuales. Cada uno se activa por separado desde Config.
class Hud {
public:
    void attach(PlayLayer* pl) {
        guide.attach(pl);
        bar.attach(pl);
    }
    void detach() {
        guide.detach();
        bar.detach();
    }
    void update(PlayLayer* pl, sa::Macro const& macro, int step, float dxPerStep, sa::Config const& cfg) {
        guide.update(pl, macro, step, dxPerStep,
                     cfg.guide == sa::GuideMode::Vertical || cfg.guide == sa::GuideMode::Both);
        bar.update(macro, step, cfg, cfg.guide == sa::GuideMode::Rhythm || cfg.guide == sa::GuideMode::Both);
    }
    void rating(PlayLayer* pl, sa::RatingEvent const& ev) { popup.show(pl, ev); }

private:
    GuideVertical guide;
    RhythmBar bar;
    RatingPopup popup;
};

} // namespace sa::ui
