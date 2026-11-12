#pragma once
#include "Component.h"

class ControllerComponent : public Component {
public:
    ControllerComponent(GameObject& owner, double speed = 180) : Component(owner), speed(speed) {}
    void initialize() override;
    void update() override;
    void draw() override {}
    double speed;
};
