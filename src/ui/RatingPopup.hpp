#pragma once
#include <Geode/Geode.hpp>

#include "../core/HitRating.hpp"

namespace sa::ui {

// Texto flotante PERFECT / GOOD -N / EARLY / LATE / MISS / LAG.
// (La fuente bigFont no tiene emojis: se usan colores verde / amarillo / rojo / naranja.)
class RatingPopup {
public:
    void show(PlayLayer* pl, sa::RatingEvent const& ev);
};

} // namespace sa::ui
