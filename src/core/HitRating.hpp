#pragma once
// Clasificación del timing de un click (sin depender de Geode).
#include <cstdlib>

namespace sa {

enum class RatingKind { Perfect, Good, Early, Late, Miss, Lag };

struct RatingEvent {
    RatingKind kind;
    int diff; // frames: negativo = temprano, positivo = tarde
};

// diff = step del jugador - step objetivo
inline RatingEvent classify(int diff, int perfectRange, int goodRange) {
    int a = std::abs(diff);
    if (a <= perfectRange) return {RatingKind::Perfect, diff};
    if (a <= goodRange) return {RatingKind::Good, diff};
    return {diff < 0 ? RatingKind::Early : RatingKind::Late, diff};
}

} // namespace sa
