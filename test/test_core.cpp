// g++ -std=c++20 -Wall -I../src/core test_core.cpp -o test_core && ./test_core
#include "InputGate.hpp"

#include <cstdio>
#include <deque>
#include <map>

using namespace sa;

// ---------------------------------------------------------------------------
// Motor falso: x avanza 1 por step; salto = vy 8; gravedad -1.
// Pinchos de suelo: muere si y==0 con x en [100,103).
// Pinchos de techo: muere si y>0 con x en [ceilA, ceilA+3].
// Saltar en el step 95 es "el click perfecto" de la macro.
// ---------------------------------------------------------------------------
struct World { int x = 0, y = 0, vy = 0; bool dead = false; int jumpAt = -1; };
struct Tap { int step; bool down; int button; bool p1; };

struct Fake : Engine {
    InputGate* gate = nullptr;
    World w;
    std::map<int, World> snaps;
    int nextH = 0, ceilA = 0;
    bool simulating = false, held = false;
    int heldCalls = 0;
    std::deque<Tap> queue;

    bool queueDue() const { return !queue.empty() && queue.front().step <= gate->step; }

    void deliver(bool down, int button, bool) override {
        if (down && button == 1 && w.y == 0) { w.vy = 8; w.jumpAt = w.x; }
    }
    void stepOnce(float, bool) override {
        while (!held && queueDue()) {
            auto c = queue.front(); queue.pop_front();
            if (!gate->onButton(c.down, c.button, c.p1)) deliver(c.down, c.button, c.p1);
        }
        w.x++;
        w.y += w.vy; w.vy -= 1;
        if (w.y <= 0) { w.y = 0; w.vy = 0; }
        if (w.y == 0 && w.x >= 100 && w.x < 103) w.dead = true;
        if (w.y > 0 && w.x >= ceilA && w.x <= ceilA + 3) w.dead = true;
    }
    int snapshot() override { snaps[nextH] = w; return nextH++; }
    void restore(int h) override { w = snaps.at(h); }
    void release(int h) override { snaps.erase(h); }
    bool dead() override { return w.dead; }
    void beginSim() override { simulating = true; }
    void endSim() override { simulating = false; }
    void clearDead() override {}
    void holdInput() override { held = true; heldCalls++; }
    void releaseInput() override { held = false; }
};

static int landing(int press) { // x del primer step con y==0 tras saltar en `press`
    Fake f; InputGate g; f.gate = &g;
    for (int s = 0; s < 200; s++) {
        g.step = s;
        if (s == press) f.deliver(true, 1, true);
        f.stepOnce(0, true);
        if (s > press && f.w.y == 0) return s + 1;
    }
    return -1;
}

static int fails = 0, total = 0;
static void check(const char* name, bool ok, const char* detail = "") {
    total++; if (!ok) fails++;
    std::printf("%-58s %s %s\n", name, ok ? "OK  " : "FAIL", detail);
}

struct Result { World w; std::vector<RatingEvent> ev; InputGate::Stats st; };

static Result attempt(InputGate& g, std::vector<Tap> clicks, bool noMacro = false) {
    Fake f; f.gate = &g; f.ceilA = landing(95) + 1;
    (void)noMacro;
    g.reset(&f);
    for (auto& c : clicks) f.queue.push_back(c);
    Result r;
    g.onRating = [&](const RatingEvent& e) { r.ev.push_back(e); };
    for (int s = 0; s < 150 && !f.w.dead; s++) g.runStep(f, 1.f / 240.f, true, f.queueDue());
    r.w = f.w; r.st = g.stats;
    return r;
}

static InputGate make(AssistMode mode, bool withMacro, int window = 10) {
    InputGate g;
    g.cfg.clickAssist = true; g.cfg.mode = mode; g.cfg.window = window; g.cfg.hitRating = true;
    g.cfg.horizon = 30; g.cfg.simBudget = 600;
    if (withMacro) g.setMacro(Macro::build({{95, 1, true, true}, {99, 1, true, false}}));
    return g;
}
#define P1(step) Tap{step, true, 1, true}

int main() {
    std::printf("aterrizaje del salto perfecto (95): x=%d\n\n", landing(95));
    char buf[96];

    // ---------------- sin asistencia (referencia) ----------------
    { InputGate g; g.cfg.clickAssist = false;
      auto r = attempt(g, {P1(85)}); check("sin asistencia: temprano 85 muere", r.w.dead);
      r = attempt(g, {P1(98)});      check("sin asistencia: tarde 98 muere", r.w.dead);
      r = attempt(g, {});            check("sin click: no salta (no auto-click)", r.w.jumpAt < 0 && r.w.dead); }

    // ---------------- 1. MacroSnap ----------------
    { auto g = make(AssistMode::MacroSnap, true);
      auto r = attempt(g, {P1(90)}); check("macro-snap: temprano 90 -> salta en 95", !r.w.dead && r.w.jumpAt == 95);
      r = attempt(g, {P1(85)});      check("macro-snap: temprano 85 (10) -> salta en 95", !r.w.dead && r.w.jumpAt == 95);
      r = attempt(g, {P1(98)});      check("macro-snap: tarde 98 -> rollback, salta en 95", !r.w.dead && r.w.jumpAt == 95);
      r = attempt(g, {P1(99)});      check("macro-snap: tarde 99 -> rollback, salta en 95", !r.w.dead && r.w.jumpAt == 95);
      r = attempt(g, {P1(80)});      check("macro-snap: 15 antes NO se corrige (muere)", r.w.dead && r.w.jumpAt == 80);
      r = attempt(g, {P1(75)});      check("macro-snap: 20 antes NO se corrige (muere)", r.w.dead && r.w.jumpAt == 75);
      r = attempt(g, {});            check("macro-snap: sin click NO salta solo", r.w.jumpAt < 0 && r.w.dead);
      r = attempt(g, {P1(98), {110, false, 1, true}});
      check("macro-snap: tarde 98 + release 110", !r.w.dead && r.w.jumpAt == 95); }

    // ---------------- 3. Trajectory (sin macro) ----------------
    { auto g = make(AssistMode::Trajectory, false);
      auto r = attempt(g, {P1(90)});
      check("trayectoria: click seguro (90) -> NO interviene", !r.w.dead && r.w.jumpAt == 90 && r.st.interventions == 0);
      r = attempt(g, {P1(95)});
      check("trayectoria: click exacto (95) -> NO interviene", !r.w.dead && r.w.jumpAt == 95 && r.st.interventions == 0);
      r = attempt(g, {P1(85)});
      std::snprintf(buf, sizeof buf, "(salta en %d)", r.w.jumpAt);
      check("trayectoria: temprano 85 -> retraso mínimo que sobrevive", !r.w.dead && r.w.jumpAt == 86, buf);
      r = attempt(g, {P1(98)});
      check("trayectoria sin macro: tarde 98 no se puede salvar (documentado)", r.w.dead);
      r = attempt(g, {});
      check("trayectoria: sin click NO salta solo", r.w.jumpAt < 0); }

    // ---------------- Hybrid (con macro) ----------------
    { auto g = make(AssistMode::Hybrid, true);
      auto r = attempt(g, {P1(90)});
      check("híbrido: click seguro 90 -> NO interviene", !r.w.dead && r.w.jumpAt == 90 && r.st.interventions == 0);
      r = attempt(g, {P1(85)});
      std::snprintf(buf, sizeof buf, "(salta en %d)", r.w.jumpAt);
      check("híbrido: temprano 85 -> prefiere el timing de la macro (95)", !r.w.dead && r.w.jumpAt == 95, buf);
      r = attempt(g, {P1(98)});
      std::snprintf(buf, sizeof buf, "(salta en %d)", r.w.jumpAt);
      check("híbrido: tarde 98 -> rollback a 95", !r.w.dead && r.w.jumpAt == 95 && r.st.rollbacks == 1, buf);
      r = attempt(g, {P1(70)});
      check("híbrido: 25 antes -> fuera de ventana, muere", r.w.dead); }

    // presupuesto agotado -> no interviene
    { auto g = make(AssistMode::Trajectory, false); g.cfg.simBudget = 5;
      auto r = attempt(g, {P1(85)});
      check("trayectoria: sin presupuesto de simulación -> no interviene", r.w.dead && r.w.jumpAt == 85); }

    // ---------------- Adaptativo ----------------
    { auto g = make(AssistMode::MacroSnap, true); g.cfg.adaptive = true; g.cfg.adaptiveMinAttempts = 2;
      auto r1 = attempt(g, {P1(93)}); auto r2 = attempt(g, {P1(93)}); auto r3 = attempt(g, {P1(93)});
      check("adaptativo: intento 1 se corrige a 95", r1.w.jumpAt == 95);
      check("adaptativo: intento 2 se corrige a 95", r2.w.jumpAt == 95);
      check("adaptativo: intento 3 reconoce el hábito (93) y no corrige", r3.w.jumpAt == 93 && !r3.w.dead);
      auto r4 = attempt(g, {P1(88)});
      check("adaptativo: un click distinto al hábito vuelve a corregirse", r4.w.jumpAt == 95);
      auto text = g.profile.serialize(); ClickProfile p; p.deserialize(text);
      check("adaptativo: el perfil se guarda y se carga", p.knownClicks() == g.profile.knownClicks() && !text.empty()); }

    { ClickProfile p; p.beginAttempt(); p.recordExtra(120); p.beginAttempt(); p.recordExtra(121);
      check("adaptativo: click extra repetido = intencional", p.isIntentionalExtra(120, 2));
      check("adaptativo: click extra aislado no lo es", !p.isIntentionalExtra(200, 2)); }

    // ---------------- 2. Rating ----------------
    { auto c = [](int d) { return classify(d, 1, 4); };
      check("rating: 0 y ±1 = PERFECT", c(0).kind == RatingKind::Perfect && c(1).kind == RatingKind::Perfect && c(-1).kind == RatingKind::Perfect);
      check("rating: ±2..4 = GOOD", c(-2).kind == RatingKind::Good && c(4).kind == RatingKind::Good);
      check("rating: -5 = EARLY, +5 = LATE", c(-5).kind == RatingKind::Early && c(5).kind == RatingKind::Late); }
    { auto g = make(AssistMode::MacroSnap, true); g.cfg.clickAssist = false; // solo observa
      auto r = attempt(g, {P1(93)});
      check("rating con macro (sin assist): 93 -> GOOD -2", r.ev.size() >= 1 && r.ev[0].kind == RatingKind::Good && r.ev[0].diff == -2);
      g.onDeath(); }
    { auto g = make(AssistMode::Trajectory, false);
      auto r = attempt(g, {P1(85)});
      check("rating sin macro: la corrección de trayectoria se califica", !r.ev.empty() && r.ev[0].diff == -1); }
    { InputGate g; g.cfg.hitRating = true; std::vector<RatingEvent> ev; g.onRating = [&](auto& e) { ev.push_back(e); };
      g.onDeath(); g.onDeath();
      check("rating: MISS al morir (una sola vez por intento)", ev.size() == 1 && ev[0].kind == RatingKind::Miss); }

    // ---------------- 5. Lag compensator ----------------
    { auto g = make(AssistMode::MacroSnap, true, 5);
      auto r = attempt(g, {P1(85)});
      check("lag: sin spike, 10 antes con ventana 5 NO se corrige", r.w.dead);
      g.cfg.lagComp = true; g.lag.configure(45, 6);
      for (int i = 0; i < 30; i++) g.lag.onFrame(1.0 / 60.0);
      g.lag.onFrame(0.080);
      check("lag: spike de 80 ms detectado", g.lag.spikeNow() && g.lag.bonus() == 6);
      r = attempt(g, {P1(85)});
      std::snprintf(buf, sizeof buf, "(salta en %d)", r.w.jumpAt);
      check("lag: durante el spike la tolerancia crece y SÍ se corrige", !r.w.dead && r.w.jumpAt == 95, buf);
      for (int i = 0; i < 5; i++) g.lag.onFrame(1.0 / 60.0);
      check("lag: pasado el spike el bono desaparece", g.lag.bonus() == 0);
      LagMonitor m; for (int i = 0; i < 30; i++) m.onFrame(1.0 / 60.0); m.onFrame(1.0 / 40.0);
      check("lag: una caída leve (60->40 fps) no cuenta como spike", !m.spikeNow()); }

    // ---------------- Macro ----------------
    { auto m = Macro::build({{100, 1, true, true}, {110, 1, true, false}, {200, 1, true, true}, {205, 1, true, false}, {300, 1, false, true}});
      check("macro: se emparejan press/release", m.clicks.size() == 3 && m.clicks[0].release == 110 && m.clicks[1].release == 205 && m.clicks[2].release == -1);
      check("macro: nearest por ambos lados", m.nearest(104, 1, true, 15) == 0 && m.nearest(196, 1, false, 15) == -1); }

    std::printf("\n%d/%d OK%s\n", total - fails, total, fails ? "  <-- HAY FALLOS" : "");
    return fails ? 1 : 0;
}
