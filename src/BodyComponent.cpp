#include "BodyComponent.h"
#include "BehaviorComponents.h"
#include "ControllerComponent.h"
#include "Engine.h"
#include <cmath>

void BodyComponent::update() {
    if (auto* item = parent().get<CarryableComponent>(); item && item->carried) return;
    double dx = vx()*Engine::deltaTime, dy = vy()*Engine::deltaTime;
    int steps = std::max(1,static_cast<int>(std::ceil(std::max(std::abs(dx),std::abs(dy))/4.0)));
    for (int i=0; i<steps; ++i)
        Engine::move(*this,dx/steps,dy/steps,parent().get<ControllerComponent>() != nullptr);
}
