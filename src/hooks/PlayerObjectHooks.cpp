// Hitbox Squeezer: achica el rect de los hazards SOLO para la colisión (lo visual no cambia).
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <algorithm>

#include "../game/Runtime.hpp"

using namespace geode::prelude;
using sa::game::Runtime;

class $modify(SAPlayerObject, PlayerObject) {
    bool collidedWithObject(float dt, GameObject* object, CCRect rect, bool skipCheck) {
        auto& rt = Runtime::get();
        if (rt.levelReady && rt.cfg().squeezer && object && object->m_objectType == GameObjectType::Hazard) {
            float p = std::clamp(rt.cfg().squeezePercent, 0.f, 40.f) / 100.f;
            rect.origin.x += rect.size.width * p * 0.5f;
            rect.origin.y += rect.size.height * p * 0.5f;
            rect.size.width *= (1.f - p);
            rect.size.height *= (1.f - p);
            rt.squeezed++;
        }
        return PlayerObject::collidedWithObject(dt, object, rect, skipCheck);
    }
};
