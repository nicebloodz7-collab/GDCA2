#pragma once
// InputGate: decide qué pasa con cada click del jugador.
//
//  * MacroSnap : clicks tempranos -> se retrasan al frame de la macro; tardíos -> rollback a un snapshot.
//  * Trajectory: se simula el futuro con la física real. Si el click es seguro NO se interviene;
//                si lleva a la muerte se busca el retraso (o rollback) más cercano que sobreviva.
//  * Hybrid    : como Trajectory, pero prefiere el timing de la macro cuando existe.
//
// Sin dependencias de Geode: todo lo que toca el juego pasa por la interfaz Engine.

#include <algorithm>
#include <functional>
#include <map>
#include <optional>
#include <vector>

#include "ClickProfile.hpp"
#include "Config.hpp"
#include "Engine.hpp"
#include "HitRating.hpp"
#include "LagMonitor.hpp"
#include "Macro.hpp"

namespace sa {

class InputGate {
public:
    Config cfg;
    ClickProfile profile;
    LagMonitor lag;
    std::function<void(const RatingEvent&)> onRating;
    int step = 0; // step de física que se está ejecutando (0 = primero tras el reset)

    struct Stats {
        int simulations = 0, simSteps = 0, delays = 0, rollbacks = 0, interventions = 0;
    } stats;

    // ---- Macro -----------------------------------------------------------
    void setMacro(Macro m) {
        macro = std::move(m);
        analyze();
    }
    bool hasMacro() const { return !macro.empty(); }
    Macro const& macroRef() const { return macro; }

    // ---- Ciclo de vida ---------------------------------------------------
    void reset(Engine* e = nullptr) {
        dropAnchors(e);
        pending.clear();
        shift.clear();
        log.clear();
        deferred.reset();
        request = -1;
        preSnap = -1;
        step = 0;
        missEmitted = false;
        std::fill(consumed.begin(), consumed.end(), false);
        profile.beginAttempt();
    }

    // El JUEGO cargó un checkpoint de practice: re-sincronizamos todo.
    void externalRewind(int toStep, Engine* e = nullptr) {
        dropAnchors(e);
        pending.clear();
        shift.clear();
        deferred.reset();
        request = -1;
        log.erase(std::remove_if(log.begin(), log.end(), [&](auto& l) { return l.step >= toStep; }), log.end());
        for (size_t i = 0; i < macro.clicks.size(); i++) {
            if (macro.clicks[i].press >= toStep) consumed[i] = false;
        }
        step = toStep;
    }

    // ---- Hook de input: devuelve true si el input se "traga" --------------
    // false = el llamador debe pasarlo al juego (handleButton original).
    bool onButton(bool down, int button, bool p1) {
        Key k{button, p1};

        if (!down) {
            int d = shift.count(k) ? shift[k] : 0;
            if (d > 0) { // el press se retrasó: el release se retrasa lo mismo
                pending.push_back({step + d, button, p1, false});
                shift[k] = 0;
                return true;
            }
            logInput(step, button, p1, false);
            return false;
        }

        // ---- press
        shift[k] = 0;
        int mi = hasMacro() ? macro.nearest(step, button, p1, kRateRange) : -1;
        int rawDiff = mi >= 0 ? step - macro.clicks[mi].press : 0;
        bool habit = false, extra = false;

        if (mi >= 0) {
            if (cfg.hitRating) emit(classify(rawDiff, cfg.perfectRange, cfg.goodRange));
            if (cfg.adaptive) {
                habit = profile.isHabit(mi, rawDiff, cfg.adaptiveMinAttempts);
                profile.record(mi, rawDiff);
            }
        } else if (cfg.adaptive) {
            extra = profile.isIntentionalExtra(step, cfg.adaptiveMinAttempts);
            profile.recordExtra(step);
        }

        if (!cfg.clickAssist || extra) {
            logInput(step, button, p1, true);
            return false;
        }
        if (cfg.mode == AssistMode::MacroSnap) return macroSnapPress(button, p1, habit);
        return deferPress(button, p1, mi, habit);
    }

    // ---- Un step de física (reemplaza al processCommands del juego) --------
    void runStep(Engine& e, float dt, bool last, bool queueNonEmpty) {
        cur = &e;
        lastDt = dt;
        maybeAnchor(e);

        preSnap = -1;
        if (cfg.clickAssist && cfg.mode != AssistMode::MacroSnap && queueNonEmpty) {
            preSnap = e.snapshot(); // estado ANTES de que el click de este step se procese
        }

        flushPending(e);
        e.stepOnce(dt, last);
        step++;

        if (deferred) resolveDeferred(e, last);
        else if (request >= 0) rollbackFromRequest(e, last);

        if (preSnap >= 0) { e.release(preSnap); preSnap = -1; }
        prune(e);
        cur = nullptr;
    }

    void onDeath() {
        if (cfg.hitRating && !missEmitted) emit({RatingKind::Miss, 0});
        missEmitted = true;
    }

    bool hasAnchor(int frame) const { return anchors.count(frame) > 0; }

private:
    using Key = std::pair<int, bool>;
    static constexpr int kRateRange = 15;

    struct Pending { int frame; int button; bool p1; bool down; };
    struct LoggedInput { int step; int button; bool p1; bool down; };
    struct Deferred { int button; bool p1; int macroIdx; bool habit; };
    struct Cand { bool rollback; int d; int anchor; };

    Macro macro;
    std::vector<int> prevGap, nextGap;
    std::map<int, std::vector<int>> frameIndex;
    std::vector<bool> consumed;
    std::vector<Pending> pending;
    std::map<Key, int> shift;
    std::vector<LoggedInput> log;
    std::map<int, int> anchors; // frame de macro -> handle de snapshot
    std::optional<Deferred> deferred;
    Engine* cur = nullptr;
    int preSnap = -1;
    int request = -1;
    float lastDt = 1.f / 240.f;
    bool missEmitted = false;

    // ---- utilidades -------------------------------------------------------
    void emit(RatingEvent ev) { if (onRating) onRating(ev); }
    void logInput(int s, int b, bool p1, bool down) { log.push_back({s, b, p1, down}); }
    int lagBonus() const { return cfg.lagComp ? lag.bonus() : 0; }
    int effWindow() const { return cfg.window + lagBonus(); }

    void analyze() {
        auto& cl = macro.clicks;
        prevGap.assign(cl.size(), 9999);
        nextGap.assign(cl.size(), 9999);
        std::map<Key, int> prev, next;
        for (size_t i = 0; i < cl.size(); i++) {
            Key k{cl[i].button, cl[i].p1};
            if (prev.count(k)) prevGap[i] = cl[i].press - prev[k];
            prev[k] = cl[i].press;
        }
        for (size_t i = cl.size(); i-- > 0;) {
            Key k{cl[i].button, cl[i].p1};
            if (next.count(k)) nextGap[i] = next[k] - cl[i].press;
            next[k] = cl[i].press;
        }
        frameIndex.clear();
        for (size_t i = 0; i < cl.size(); i++) frameIndex[cl[i].press].push_back((int)i);
        consumed.assign(cl.size(), false);
    }

    int earlyWin(size_t i) const { return std::max(0, std::min(effWindow(), prevGap[i] / 2)); }
    int lateWin(size_t i) const { return std::max(0, std::min(effWindow(), nextGap[i] / 2)); }

    // Press de macro futuro (frame >= step) que el jugador adelantó dentro de la ventana.
    int findEarly(int button, bool p1) const {
        for (size_t i = macro.firstAtOrAfter(step); i < macro.clicks.size(); i++) {
            auto& c = macro.clicks[i];
            if (c.press - step > effWindow()) break;
            if (consumed[i] || c.button != button || c.p1 != p1) continue;
            if (c.press - step <= earlyWin(i)) return (int)i;
        }
        return -1;
    }

    // Press de macro ya pasado (frame < step) que el jugador se comió tarde.
    int findLate(int button, bool p1) const {
        for (size_t i = macro.firstAtOrAfter(step); i-- > 0;) {
            auto& c = macro.clicks[i];
            if (step - c.press > effWindow()) break;
            if (consumed[i] || c.button != button || c.p1 != p1) continue;
            if (step - c.press <= lateWin(i)) return (int)i;
        }
        return -1;
    }

    // No hacemos rollback si el jugador ya hizo otros inputs de ese botón entre M y ahora.
    bool canRollback(int M, int button, bool p1, int F) const {
        for (auto& l : log) {
            if (l.button == button && l.p1 == p1 && l.step >= M && l.step < F) return false;
        }
        return true;
    }

    // ---- MacroSnap --------------------------------------------------------
    bool macroSnapPress(int button, bool p1, bool habit) {
        Key k{button, p1};
        if (!habit) {
            int idx = findEarly(button, p1);
            if (idx >= 0) {
                consumed[idx] = true;
                int d = macro.clicks[idx].press - step;
                shift[k] = d;
                if (d > 0) {
                    pending.push_back({macro.clicks[idx].press, button, p1, true});
                    stats.delays++;
                    stats.interventions++;
                    return true;
                }
                logInput(step, button, p1, true);
                return false;
            }
            if (cfg.lateRollback && request < 0) {
                int li = findLate(button, p1);
                if (li >= 0 && anchors.count(macro.clicks[li].press) &&
                    canRollback(macro.clicks[li].press, button, p1, step)) {
                    consumed[li] = true;
                    request = li;
                    return true; // se resuelve al final del step
                }
            }
        }
        logInput(step, button, p1, true);
        return false;
    }

    // ---- Trajectory / Hybrid ----------------------------------------------
    bool deferPress(int button, bool p1, int macroIdx, bool habit) {
        if (!cur || preSnap < 0 || deferred) { // sin snapshot previo no se puede evaluar
            logInput(step, button, p1, true);
            return false;
        }
        deferred = Deferred{button, p1, macroIdx, habit};
        cur->beginSim(); // el step corre sin este click: que una muerte no tenga efectos reales
        return true;
    }

    void resolveDeferred(Engine& e, bool last) {
        const int F = step - 1;
        const Deferred P = *deferred;
        deferred.reset();
        e.endSim();
        e.clearDead();

        std::vector<Cand> cands;
        auto add = [&](Cand c) {
            for (auto& x : cands) {
                if (x.rollback == c.rollback && x.d == c.d && x.anchor == c.anchor) return;
            }
            cands.push_back(c);
        };
        add({false, 0, 0}); // 1º: el click tal cual. Si es seguro, NO se interviene.

        const int W = effWindow();
        if (cfg.mode == AssistMode::Hybrid && P.macroIdx >= 0 && !P.habit) {
            int M = macro.clicks[P.macroIdx].press;
            if (M > F && M - F <= W) add({false, M - F, 0});
            else if (M <= F && cfg.lateRollback && F - M <= W && anchors.count(M) &&
                     canRollback(M, P.button, P.p1, F)) add({true, 0, M});
        }
        for (int d = 1; d <= W; d++) add({false, d, 0});

        int hold = cfg.assumeHold;
        if (P.macroIdx >= 0 && macro.clicks[P.macroIdx].release >= 0) {
            hold = std::max(1, macro.clicks[P.macroIdx].release - macro.clicks[P.macroIdx].press);
        }

        int budget = cfg.simBudget;
        Cand chosen{false, 0, 0};
        for (auto& c : cands) {
            int r = simulate(e, c, P, F, hold, budget);
            if (r < 0) break;       // sin presupuesto: no intervenimos
            if (r == 1) { chosen = c; break; }
        }
        finalize(e, last, chosen, P, F);
    }

    // 1 = sobrevive, 0 = muere, -1 = sin presupuesto
    int simulate(Engine& e, Cand c, const Deferred& P, int F, int hold, int& budget) {
        int snap = c.rollback ? anchors.at(c.anchor) : preSnap;
        int startStep = c.rollback ? c.anchor : F;
        int pressStep = c.rollback ? c.anchor : F + c.d;
        int end = std::max(F, pressStep) + cfg.horizon;
        int need = end - startStep;
        if (need > budget) return -1;
        budget -= need;
        stats.simulations++;
        stats.simSteps += need;

        e.beginSim();
        e.holdInput();
        e.restore(snap);
        bool dead = false;
        for (int s = startStep; s < end && !dead; s++) {
            for (auto& l : log) {
                if (l.step == s) e.deliver(l.down, l.button, l.p1);
            }
            if (s == pressStep) e.deliver(true, P.button, P.p1);
            if (s == pressStep + hold) e.deliver(false, P.button, P.p1);
            e.stepOnce(lastDt, false);
            if (e.dead()) dead = true;
        }
        e.releaseInput();
        e.endSim();
        e.clearDead();
        return dead ? 0 : 1;
    }

    void finalize(Engine& e, bool last, Cand c, const Deferred& P, int F) {
        Key k{P.button, P.p1};
        if (P.macroIdx >= 0) consumed[P.macroIdx] = true;

        if (c.rollback) {
            rollbackTo(e, last, c.anchor, P.button, P.p1, F);
            stats.rollbacks++;
            stats.interventions++;
            return;
        }

        e.holdInput();
        e.restore(preSnap);
        step = F;
        for (auto& l : log) {
            if (l.step == F) e.deliver(l.down, l.button, l.p1);
        }
        if (c.d == 0) {
            e.deliver(true, P.button, P.p1);
            logInput(F, P.button, P.p1, true);
        } else {
            pending.push_back({F + c.d, P.button, P.p1, true});
            shift[k] = c.d;
            stats.delays++;
            stats.interventions++;
            if (cfg.hitRating && P.macroIdx < 0) emit(classify(-c.d, cfg.perfectRange, cfg.goodRange));
        }
        e.stepOnce(lastDt, last);
        e.releaseInput();
        step = F + 1;
    }

    // ---- Rollback ----------------------------------------------------------
    void rollbackFromRequest(Engine& e, bool last) {
        int idx = request;
        request = -1;
        auto const& c = macro.clicks[idx];
        rollbackTo(e, last, c.press, c.button, c.p1, step - 1);
        stats.rollbacks++;
        stats.interventions++;
    }

    // Vuelve al snapshot de M, mete el press en M y re-simula hasta F (inclusive).
    void rollbackTo(Engine& e, bool last, int M, int button, bool p1, int F) {
        auto sit = anchors.find(M);
        if (sit == anchors.end()) return;

        for (auto it = anchors.begin(); it != anchors.end();) { // snapshots de una línea de tiempo que ya no existe
            if (it->first > M) { e.release(it->second); it = anchors.erase(it); }
            else ++it;
        }

        std::vector<LoggedInput> replay;
        for (auto& l : log) {
            if (l.step >= M) replay.push_back(l);
        }
        replay.insert(replay.begin(), {M, button, p1, true});
        log.erase(std::remove_if(log.begin(), log.end(), [&](auto& l) { return l.step >= M; }), log.end());

        e.holdInput();
        e.restore(sit->second);
        for (int s = M; s <= F; s++) {
            step = s;
            if (s > M) maybeAnchor(e);
            for (auto& l : replay) {
                if (l.step != s) continue;
                e.deliver(l.down, l.button, l.p1);
                logInput(s, l.button, l.p1, l.down);
            }
            e.stepOnce(lastDt, s == F ? last : false);
            step = s + 1;
            if (e.dead()) break;
        }
        e.releaseInput();
        step = std::max(step, F + 1);
    }

    // ---- Snapshots anclados a los clicks de la macro --------------------------
    void maybeAnchor(Engine& e) {
        if (!cfg.clickAssist || !cfg.lateRollback || macro.empty()) return;
        auto it = frameIndex.find(step);
        if (it == frameIndex.end() || anchors.count(step)) return;
        bool any = false;
        for (int i : it->second) any = any || !consumed[i];
        if (!any) return;
        int h = e.snapshot();
        if (h >= 0) anchors[step] = h;
    }

    void flushPending(Engine& e) {
        if (pending.empty()) return;
        std::stable_sort(pending.begin(), pending.end(), [](auto& a, auto& b) { return a.frame < b.frame; });
        std::vector<Pending> keep;
        for (auto& p : pending) {
            if (p.frame <= step) {
                e.deliver(p.down, p.button, p.p1);
                logInput(step, p.button, p.p1, p.down);
            } else {
                keep.push_back(p);
            }
        }
        pending = std::move(keep);
    }

    void prune(Engine& e) {
        int maxW = cfg.window + cfg.lagMaxBonus;
        for (auto it = anchors.begin(); it != anchors.end();) {
            if (it->first + maxW < step) { e.release(it->second); it = anchors.erase(it); }
            else ++it;
        }
        int minStep = step - maxW - 2;
        log.erase(std::remove_if(log.begin(), log.end(), [&](auto& l) { return l.step < minStep; }), log.end());
    }

    void dropAnchors(Engine* e) {
        if (e) {
            for (auto& [f, h] : anchors) e->release(h);
        }
        anchors.clear();
    }
};

} // namespace sa
