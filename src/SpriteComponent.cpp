#include "SpriteComponent.h"
#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "Engine.h"
#include "Graphics.h"
#include "Textures.h"
#include <stdexcept>

void SpriteComponent::initialize() {
    if (!parent().get<BodyComponent>()) throw std::runtime_error(parent().id + " needs BodyComponent");
    if (!Textures::get(textureKey)) throw std::runtime_error("Unknown texture: " + textureKey);
}
void SpriteComponent::draw() {
    auto* body = parent().get<BodyComponent>();
    SDL_Color tint{255,255,255,255};
    if (auto* valve = parent().get<ValveComponent>())
        tint={static_cast<Uint8>(valve->open ? 120 : 255),static_cast<Uint8>(valve->open ? 255 : 170),170,255};
    if (auto* heat = parent().get<HeatSourceComponent>(); heat && !heat->active)
        tint={100,110,120,255};
    if (auto* repair = parent().get<RepairComponent>(); repair && repair->complete)
        tint={125,255,170,255};
    auto point=Engine::view.worldToScreen(body->x(),body->y());
    Graphics::drawTexture(textureKey,point.x,point.y,static_cast<int>(body->width),
        static_cast<int>(body->height),body->angle(),tint);
}
