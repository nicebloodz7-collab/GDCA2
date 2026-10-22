#pragma once
// Macro ya interpretada: lista de clicks (press + release) ordenados por frame.
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace sa {

struct RawInput {
    int frame;
    int button; // 1 = Jump, 2 = Left, 3 = Right
    bool p1;
    bool down;
};

struct Click {
    int press;
    int release; // -1 si la macro no tiene release
    int button;
    bool p1;
};

class Macro {
public:
    std::vector<Click> clicks;
    uint32_t levelID = 0;

    static Macro build(std::vector<RawInput> raw, uint32_t levelID = 0) {
        std::stable_sort(raw.begin(), raw.end(), [](auto& a, auto& b) { return a.frame < b.frame; });
        Macro m;
        m.levelID = levelID;
        std::map<std::pair<int, bool>, size_t> open;
        for (auto& in : raw) {
            std::pair<int, bool> k{in.button, in.p1};
            if (in.down) {
                m.clicks.push_back({in.frame, -1, in.button, in.p1});
                open[k] = m.clicks.size() - 1;
            } else if (auto it = open.find(k); it != open.end()) {
                m.clicks[it->second].release = in.frame;
                open.erase(it);
            }
        }
        return m;
    }

    bool empty() const { return clicks.empty(); }

    // Índice del primer click con press >= step (o clicks.size()).
    size_t firstAtOrAfter(int step) const {
        return std::lower_bound(clicks.begin(), clicks.end(), step,
                                [](const Click& c, int s) { return c.press < s; }) - clicks.begin();
    }

    // Click del mismo botón/jugador más cercano a `step` (por ambos lados), o -1.
    int nearest(int step, int button, bool p1, int maxDist) const {
        int best = -1, bestDist = maxDist + 1;
        size_t hi = firstAtOrAfter(step);
        for (size_t i = hi; i < clicks.size() && clicks[i].press - step <= maxDist; i++) {
            if (clicks[i].button != button || clicks[i].p1 != p1) continue;
            int d = std::abs(clicks[i].press - step);
            if (d < bestDist) { best = (int)i; bestDist = d; }
            break;
        }
        for (size_t i = hi; i-- > 0 && step - clicks[i].press <= maxDist;) {
            if (clicks[i].button != button || clicks[i].p1 != p1) continue;
            int d = std::abs(clicks[i].press - step);
            if (d < bestDist) { best = (int)i; bestDist = d; }
            break;
        }
        return best;
    }
};

} // namespace sa
