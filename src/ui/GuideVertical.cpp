#include "GuideVertical.hpp"

using namespace geode::prelude;

namespace sa::ui {

void GuideVertical::attach(PlayLayer* pl) {
    detach();
    node = CCDrawNode::create();
    pl->m_objectLayer->addChild(node, 1000); // mismo espacio de coordenadas que el jugador
}

void GuideVertical::detach() {
    if (node) node->removeFromParent();
    node = nullptr;
}

void GuideVertical::update(PlayLayer* pl, sa::Macro const& macro, int step, float dxPerStep, bool enabled) {
    if (!node) return;
    node->clear();
    if (!enabled || macro.empty() || !pl->m_player1) return;

    // próximo click (se mantiene 2 steps después de pasar, para verlo "cruzar")
    size_t i = macro.firstAtOrAfter(step - 2);
    if (i >= macro.clicks.size()) return;
    auto const& click = macro.clicks[i];

    auto* player = pl->m_player1;
    CCRect const hitbox = player->getObjectRect(); // la hitbox "roja"
    float remaining = (float)(click.press - step);
    // posición X estimada del click: velocidad actual medida (px por step) * steps restantes
    float x = player->getPositionX() + remaining * dxPerStep;
    float y = hitbox.getMidY();

    float alpha = remaining <= 0.f ? 1.f : 0.8f;
    node->drawSegment(ccp(x, y - 170.f), ccp(x, y + 170.f), 1.2f, ccColor4F{0.2f, 1.f, 0.3f, alpha});

    CCRect target(x - hitbox.size.width / 2.f, hitbox.origin.y, hitbox.size.width, hitbox.size.height);
    node->drawRect(target, ccColor4F{1.f, 0.f, 0.f, 0.12f}, 1.2f, ccColor4F{1.f, 0.f, 0.f, 1.f});
}

} // namespace sa::ui
