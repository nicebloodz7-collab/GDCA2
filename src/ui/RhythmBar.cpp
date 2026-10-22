#include "RhythmBar.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace sa::ui {

void RhythmBar::attach(PlayLayer* pl) {
    detach();
    node = CCDrawNode::create();
    pl->m_uiLayer->addChild(node, 40);
}

void RhythmBar::detach() {
    if (node) node->removeFromParent();
    node = nullptr;
}

void RhythmBar::update(sa::Macro const& macro, int step, sa::Config const& cfg, bool enabled) {
    if (!node) return;
    node->clear();
    if (!enabled || macro.empty()) return;

    auto ws = CCDirector::get()->getWinSize();
    const float barW = ws.width * 0.5f, barH = 20.f;
    const float left = ws.width * 0.5f - barW * 0.5f, right = left + barW;
    const float y = ws.height - 30.f;
    const float impactX = left + barW * 0.12f;
    const float lookahead = (float)std::max(30, cfg.rhythmLookahead);

    node->drawRect(CCRect(left, y - barH / 2.f, barW, barH), ccColor4F{0.f, 0.f, 0.f, 0.45f}, 1.f,
                   ccColor4F{1.f, 1.f, 1.f, 0.5f});
    node->drawSegment(ccp(impactX, y - barH / 2.f - 3.f), ccp(impactX, y + barH / 2.f + 3.f), 1.5f,
                      ccColor4F{0.2f, 1.f, 0.3f, 1.f});

    auto toX = [&](float t) { return impactX + (t / lookahead) * (right - impactX); };

    bool nextFound = false;
    for (size_t i = macro.firstAtOrAfter(step - 12); i < macro.clicks.size(); i++) {
        auto const& c = macro.clicks[i];
        float t = (float)(c.press - step);
        if (t > lookahead) break;
        float x = toX(t);
        if (x < left) continue;

        if (c.release >= 0) { // duración del hold
            float xr = std::min(right, toX((float)(c.release - step)));
            if (xr > x) {
                node->drawRect(CCRect(x, y - 2.f, xr - x, 4.f), ccColor4F{0.5f, 0.9f, 1.f, 0.35f}, 0.f,
                               ccColor4F{0.f, 0.f, 0.f, 0.f});
            }
        }

        ccColor4F col = t < 0 ? ccColor4F{0.6f, 0.6f, 0.6f, 0.5f} : ccColor4F{1.f, 1.f, 1.f, 1.f};
        if (t >= 0 && !nextFound) { // la próxima nota, resaltada
            col = ccColor4F{0.3f, 0.9f, 1.f, 1.f};
            nextFound = true;
        }
        node->drawRect(CCRect(x - 2.5f, y - barH / 2.f + 3.f, 5.f, barH - 6.f), col, 0.5f, col);
    }
}

} // namespace sa::ui
