#include "SpriteComponent.h"
#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "Engine.h"
#include "Textures.h"
#include <stdexcept>

void SpriteComponent::initialize() {
    if (!parent().get<BodyComponent>()) throw std::runtime_error(parent().id + " needs BodyComponent");
    if (!Textures::get(textureKey)) throw std::runtime_error("Unknown texture: " + textureKey);
}
void SpriteComponent::draw() {
    auto* body = parent().get<BodyComponent>();
    auto* texture = Textures::get(textureKey);
    SDL_SetTextureColorMod(texture,255,255,255);
    if (auto* valve = parent().get<ValveComponent>())
        SDL_SetTextureColorMod(texture,valve->open ? 120 : 255, valve->open ? 255 : 170,170);
    if (auto* heat = parent().get<HeatSourceComponent>(); heat && !heat->active)
        SDL_SetTextureColorMod(texture,100,110,120);
    if (auto* repair = parent().get<RepairComponent>(); repair && repair->complete)
        SDL_SetTextureColorMod(texture,125,255,170);
    SDL_Rect dst{Engine::offsetX+static_cast<int>(body->x()), Engine::offsetY+static_cast<int>(body->y()),
                 static_cast<int>(body->width), static_cast<int>(body->height)};
    SDL_RenderCopyEx(Engine::getRenderer(),texture,nullptr,&dst,body->angle(),nullptr,SDL_FLIP_NONE);
}
