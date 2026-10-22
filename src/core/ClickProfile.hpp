#pragma once
// Click assist adaptativo: aprende, a lo largo de los intentos, qué clicks
// del jugador son "intencionales" aunque difieran de la macro.
#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace sa {

class ClickProfile {
public:
    int attempts = 0;
    int tolerance = 1; // cuánto puede variar el offset entre intentos para ser "hábito"

    void beginAttempt() { attempts++; }

    // ¿Este offset (step del jugador - press de macro) es un hábito consistente?
    bool isHabit(int clickIdx, int offset, int minAttempts) const {
        auto it = stats.find(clickIdx);
        if (it == stats.end()) return false;
        return it->second.consistent >= minAttempts &&
               std::abs(offset - (int)std::lround(it->second.ema)) <= tolerance;
    }

    // Registra el offset de este intento (uno por click de macro y por intento).
    void record(int clickIdx, int offset) {
        auto& s = stats[clickIdx];
        if (s.lastAttempt == attempts) return;
        if (s.lastAttempt < 0) {
            s.ema = offset;
            s.consistent = 1;
        } else if (std::abs(offset - (int)std::lround(s.ema)) <= tolerance) {
            s.ema = s.ema * 0.5 + offset * 0.5;
            s.consistent++;
        } else {
            s.ema = offset; // cambió de patrón: empieza de nuevo
            s.consistent = 1;
        }
        s.lastAttempt = attempts;
    }

    // Clicks "extra" (sin click de macro cerca): son intencionales si se repiten en el mismo step.
    bool isIntentionalExtra(int step, int minAttempts) const {
        for (auto& e : extras) {
            if (std::abs(e.step - step) <= tolerance + 1 && e.count >= minAttempts) return true;
        }
        return false;
    }

    void recordExtra(int step) {
        for (auto& e : extras) {
            if (std::abs(e.step - step) <= tolerance + 1) {
                if (e.lastAttempt != attempts) { e.count++; e.lastAttempt = attempts; }
                return;
            }
        }
        extras.push_back({step, 1, attempts});
    }

    std::string serialize() const {
        std::ostringstream o;
        for (auto& [i, s] : stats) o << "c " << i << ' ' << s.ema << ' ' << s.consistent << '\n';
        for (auto& e : extras) o << "x " << e.step << ' ' << e.count << '\n';
        return o.str();
    }

    void deserialize(std::string const& text) {
        stats.clear();
        extras.clear();
        std::istringstream in(text);
        std::string tag;
        while (in >> tag) {
            if (tag == "c") {
                int i, cons; double ema;
                if (in >> i >> ema >> cons) stats[i] = {ema, cons, -1};
            } else if (tag == "x") {
                int step, count;
                if (in >> step >> count) extras.push_back({step, count, -1});
            }
        }
    }

    void clear() { stats.clear(); extras.clear(); attempts = 0; }
    size_t knownClicks() const { return stats.size(); }

private:
    struct Stat { double ema = 0; int consistent = 0; int lastAttempt = -1; };
    struct Extra { int step; int count; int lastAttempt; };
    std::map<int, Stat> stats;
    std::vector<Extra> extras;
};

} // namespace sa
