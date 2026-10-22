#include "Settings.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace sa::game {

static std::string str(char const* key) { return Mod::get()->getSettingValue<std::string>(key); }
static int num(char const* key) { return (int)Mod::get()->getSettingValue<int64_t>(key); }
static bool flag(char const* key) { return Mod::get()->getSettingValue<bool>(key); }

Config readConfig() {
    Config c;

    // 1. Guía visual
    auto g = str("guide-mode");
    c.guide = g == "vertical" ? GuideMode::Vertical
            : g == "rhythm"   ? GuideMode::Rhythm
            : g == "both"     ? GuideMode::Both
                              : GuideMode::Off;
    c.rhythmLookahead = num("rhythm-lookahead");

    // 2. Rating
    c.hitRating = flag("hit-rating");
    c.perfectRange = num("rating-perfect-range");
    c.goodRange = std::max(c.perfectRange, num("rating-good-range"));

    // 3. Click assist
    c.clickAssist = flag("click-assist");
    auto m = str("assist-mode");
    c.mode = m == "macro-snap" ? AssistMode::MacroSnap
           : m == "trajectory" ? AssistMode::Trajectory
                               : AssistMode::Hybrid;
    c.window = num("assist-window");
    c.lateRollback = flag("late-rollback");
    c.adaptive = flag("adaptive-assist");
    c.adaptiveMinAttempts = num("adaptive-attempts");
    c.horizon = num("trajectory-horizon");
    c.assumeHold = num("trajectory-assume-hold");
    c.simBudget = num("trajectory-budget");

    // 4. Squeezer
    c.squeezer = flag("hitbox-squeezer");
    c.squeezePercent = (float)Mod::get()->getSettingValue<double>("squeezer-percent");

    // 5. Lag
    c.lagComp = flag("lag-compensator");
    c.lagThresholdMs = num("lag-threshold-ms");
    c.lagMaxBonus = num("lag-max-bonus");
    c.lagIndicator = flag("lag-indicator");

    c.debugLog = flag("debug-log");
    return c;
}

} // namespace sa::game
