#include "RatingPopup.hpp"

using namespace geode::prelude;

namespace sa::ui {

void RatingPopup::show(PlayLayer* pl, sa::RatingEvent const& ev) {
    std::string text;
    ccColor3B color;
    switch (ev.kind) {
        case sa::RatingKind::Perfect: text = "PERFECT"; color = {60, 255, 90}; break;
        case sa::RatingKind::Good: text = fmt::format("GOOD {:+}", ev.diff); color = {255, 220, 60}; break;
        case sa::RatingKind::Early: text = fmt::format("EARLY {:+}", ev.diff); color = {255, 70, 70}; break;
        case sa::RatingKind::Late: text = fmt::format("LATE {:+}", ev.diff); color = {255, 70, 70}; break;
        case sa::RatingKind::Miss: text = "MISS"; color = {255, 40, 40}; break;
        case sa::RatingKind::Lag: text = fmt::format("LAG {}ms", ev.diff); color = {255, 150, 40}; break;
    }

    auto ws = CCDirector::get()->getWinSize();
    auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setScale(0.5f);
    label->setColor(color);
    label->setPosition(ccp(ws.width * 0.5f, ws.height * 0.62f));
    pl->m_uiLayer->addChild(label, 50);

    label->runAction(CCSequence::create(
        CCSpawn::create(CCMoveBy::create(0.55f, ccp(0.f, 22.f)), CCFadeOut::create(0.55f), nullptr),
        CCCallFunc::create(label, callfunc_selector(CCNode::removeFromParent)),
        nullptr));
}

} // namespace sa::ui
