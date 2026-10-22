#pragma once
// Detecta micro-lags a partir del dt real de cada frame renderizado.
#include <algorithm>
#include <cmath>

namespace sa {

class LagMonitor {
public:
    double thresholdSec = 0.045;
    int maxBonus = 6;
    double stepsPerSec = 240.0;

    void configure(int thresholdMs, int maxBonus_) {
        thresholdSec = thresholdMs / 1000.0;
        maxBonus = maxBonus_;
    }

    // Se llama UNA vez por frame, ANTES de que el juego simule los steps de ese frame.
    void onFrame(double dt) {
        bool spike = avg_ > 0 && dt > thresholdSec && dt > 2.5 * avg_;
        if (spike) {
            int total = (int)std::lround(dt * stepsPerSec);
            int normal = (int)std::lround(avg_ * stepsPerSec);
            extra_ = std::max(0, total - normal);
            since_ = 0;
            spikes_++;
            lastSpikeMs_ = dt * 1000.0;
        } else {
            since_ = std::min(since_ + 1, 1000);
            avg_ = avg_ > 0 ? avg_ * 0.95 + dt * 0.05 : dt;
        }
    }

    bool spikeNow() const { return since_ == 0; }
    // Steps extra de tolerancia para clicks registrados durante/justo después de un spike.
    int bonus() const { return since_ <= 1 ? std::min(maxBonus, extra_) : 0; }
    int spikes() const { return spikes_; }
    double lastSpikeMs() const { return lastSpikeMs_; }

private:
    double avg_ = 0;
    int since_ = 1000;
    int extra_ = 0;
    int spikes_ = 0;
    double lastSpikeMs_ = 0;
};

} // namespace sa
