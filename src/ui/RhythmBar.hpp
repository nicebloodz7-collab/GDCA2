#pragma once
#include <Geode/Geode.hpp>

#include "../core/Config.hpp"
#include "../core/Macro.hpp"

namespace sa::ui {

// Modo 2: barra rítmica 1-tecla en la parte superior (estilo osu!mania).
// Las notas viajan de derecha a izquierda hacia la barra de impacto.
class RhythmBar {
public:
    void attach(PlayLayer* pl);
    void detach();
    void update(sa::Macro const& macro, int step, sa::Config const& cfg, bool enabled);

private:
    geode::Ref<cocos2d::CCDrawNode> node;
};

} // namespace sa::ui
