#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "ComponentFactory.h"
#include "Engine.h"
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
void needBody(GameObject& owner) {
    if (!owner.get<BodyComponent>()) throw std::runtime_error(owner.id + " needs BodyComponent");
}
void steer(GameObject& owner, GameObject* target, double speed, CreatureState& state) {
    auto* body = owner.get<BodyComponent>();
    body->vx() = body->vy() = 0;
    if (!target) { state = CreatureState::Waiting; return; }
    auto* destination = target->get<BodyComponent>();
    double remaining = Engine::distance(*body,*destination);
    double stop = target->get<CarryableComponent>() ? 54 : 12;
    if (remaining <= stop) { state = CreatureState::Settled; return; }
    auto path = Engine::pathTo(*body,destination->centerX(),destination->centerY(),
                               !target->get<CarryableComponent>());
    if (path.empty()) { state = CreatureState::Waiting; return; }
    auto [x,y] = path.front();
    double dx = x-body->centerX(), dy = y-body->centerY(), length = std::hypot(dx,dy);
    if (length < 0.001) return;
    double stepSpeed = std::min(speed,length/Engine::deltaTime);
    body->vx() = dx/length*stepSpeed;
    body->vy() = dy/length*stepSpeed;
    state = CreatureState::Moving;
}
}

void ValveComponent::initialize() { needBody(parent()); if (open) createPuddle(); }
void ValveComponent::createPuddle() {
    if (Engine::find(puddleId)) return;
    std::ostringstream xml;
    xml << "<GameObject id='" << puddleId << "'><BodyComponent x='" << puddleX
        << "' y='" << puddleY << "' width='40' height='40'/>"
        << "<SpriteComponent textureKey='puddle'/><PuddleComponent/></GameObject>";
    Engine::addGameObject(ComponentFactory().fromString(xml.str()));
}
void ValveComponent::toggle() {
    open = !open;
    if (open) createPuddle();
    else if (auto* puddle = Engine::find(puddleId)) puddle->destroy();
    Engine::message(open ? "VALVE OPEN. WATER DRAWS SLIMES." : "VALVE CLOSED. PUDDLE REMOVED.");
}
void SlimeBehaviorComponent::initialize() { needBody(parent()); }
void SlimeBehaviorComponent::update() {
    auto* body = parent().get<BodyComponent>();
    GameObject* target = nullptr;
    double best = std::numeric_limits<double>::max();
    for (const auto& object : Engine::objects()) {
        if (!object->alive || !object->get<PuddleComponent>()) continue;
        auto* source = object->get<BodyComponent>();
        auto path = Engine::pathTo(*body,source->centerX(),source->centerY());
        if (path.empty()) continue;
        double cost = 0, lastX = body->centerX(), lastY = body->centerY();
        for (auto [x,y] : path) { cost += std::hypot(x-lastX,y-lastY); lastX=x; lastY=y; }
        if (cost < best) { best=cost; target=object.get(); }
    }
    targetId = target ? target->id : "";
    steer(parent(),target,speed,state);
}
void HeatSeekingComponent::initialize() { needBody(parent()); }
void HeatSeekingComponent::update() {
    auto* body = parent().get<BodyComponent>();
    GameObject* target = nullptr;
    if (auto* previous = Engine::find(targetId)) {
        auto* heat = previous->get<HeatSourceComponent>();
        if (heat && heat->heater && heat->active && state == CreatureState::Settled) target=previous;
    }
    if (!target) for (const auto& object : Engine::objects()) {
        auto* heat=object->get<HeatSourceComponent>();
        if (object->alive && heat && heat->heater && heat->active &&
            Engine::distance(*body,*object->get<BodyComponent>())<100) { target=object.get(); break; }
    }
    if (!target) {
        auto* held = Engine::find(Engine::heldItem);
        if (held && held->get<CarryableComponent>() && held->get<CarryableComponent>()->kind == "coal" &&
            Engine::distance(*body,*held->get<BodyComponent>()) <= range) target=held;
    }
    if (!target) {
        double best = std::numeric_limits<double>::max();
        for (const auto& object : Engine::objects()) {
            auto* heat = object->get<HeatSourceComponent>();
            if (!object->alive || !heat || !heat->active || !heat->heater) continue;
            double d = Engine::distance(*body,*object->get<BodyComponent>());
            if (d < best) { best=d; target=object.get(); }
        }
    }
    if (!target) target=Engine::find("furnace");
    targetId = target ? target->id : "";
    steer(parent(),target,speed,state);
}
