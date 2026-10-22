#pragma once
#include <Geode/Geode.hpp>

#include "../core/Config.hpp"
#include "../core/Macro.hpp"

namespace sa::ui {

// Modo 1: línea vertical verde en el mundo, en el punto donde debe ocurrir el próximo click,
// con un rectángulo del tamaño de la hitbox del jugador (la roja) a su altura actual.
class GuideVertical {
public:
    void attach(PlayLayer* pl);
    void detach();
    void update(PlayLayer* pl, sa::Macro const& macro, int step, float dxPerStep, bool enabled);

private:
    geode::Ref<cocos2d::CCDrawNode> node;
};

} // namespace sa::ui
