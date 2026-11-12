#include "ControllerComponent.h"
#include "BodyComponent.h"
#include "Engine.h"
#include "Input.h"
#include <cmath>
#include <stdexcept>

void ControllerComponent::initialize() {
    if (!parent().get<BodyComponent>()) throw std::runtime_error("Controller needs BodyComponent");
}
void ControllerComponent::update() {
    auto* body = parent().get<BodyComponent>();
    double x = (Input::isKeyDown(SDLK_d)||Input::isKeyDown(SDLK_RIGHT))
             - (Input::isKeyDown(SDLK_a)||Input::isKeyDown(SDLK_LEFT));
    double y = (Input::isKeyDown(SDLK_s)||Input::isKeyDown(SDLK_DOWN))
             - (Input::isKeyDown(SDLK_w)||Input::isKeyDown(SDLK_UP));
    double length = std::hypot(x,y);
    body->vx() = length ? x/length*speed : 0;
    body->vy() = length ? y/length*speed : 0;
    if (Input::isKeyPressed(SDLK_SPACE)) Engine::pickUpOrDrop(parent());
    if (Input::isKeyPressed(SDLK_e)) Engine::interact(parent());
}
