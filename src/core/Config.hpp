#pragma once
// Configuración ya "digerida" (sin depender de Geode). La llena game/Settings.cpp.
#include <string>

namespace sa {

enum class GuideMode { Off, Vertical, Rhythm, Both };
enum class AssistMode { MacroSnap, Trajectory, Hybrid };

struct Config {
    // ---- 1. Guía visual por macro
    GuideMode guide = GuideMode::Off;
    int rhythmLookahead = 150; // steps visibles hacia adelante en la barra rítmica

    // ---- 2. Feedback de precisión
    bool hitRating = true;
    int perfectRange = 1; // |diff| <= perfectRange  -> PERFECT
    int goodRange = 4;    // |diff| <= goodRange     -> GOOD, más allá -> EARLY/LATE

    // ---- 3. Click assist (+ adaptativo + calculador de trayectoria)
    bool clickAssist = false;
    AssistMode mode = AssistMode::Hybrid;
    int window = 10;          // tolerancia en frames de macro (steps de física)
    bool lateRollback = true; // corregir clicks tardíos volviendo a un snapshot
    bool adaptive = false;
    int adaptiveMinAttempts = 2;
    int horizon = 30;    // steps que se simulan hacia adelante
    int assumeHold = 4;  // duración del hold asumida si la macro no la dice
    int simBudget = 400; // máximo de steps simulados por click

    // ---- 4. Hitbox squeezer
    bool squeezer = false;
    float squeezePercent = 8.f;

    // ---- 5. Lag spike compensator
    bool lagComp = false;
    int lagThresholdMs = 45;
    int lagMaxBonus = 6;
    bool lagIndicator = true;

    bool debugLog = false;
};

} // namespace sa
