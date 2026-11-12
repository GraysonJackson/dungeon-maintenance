#pragma once
#include "Component.h"
#include <string>

class PassiveComponent : public Component {
public:
    using Component::Component;
    void update() override {}
    void draw() override {}
};

class PushableComponent : public PassiveComponent {
public:
    using PassiveComponent::PassiveComponent;
};

class CarryableComponent : public PassiveComponent {
public:
    CarryableComponent(GameObject& owner, std::string kind) : PassiveComponent(owner), kind(std::move(kind)) {}
    std::string kind;
    bool carried = false;
};

class PuddleComponent : public PassiveComponent {
public:
    using PassiveComponent::PassiveComponent;
};

class HeatSourceComponent : public PassiveComponent {
public:
    HeatSourceComponent(GameObject& owner, bool active = false, bool heater = true)
        : PassiveComponent(owner), active(active), heater(heater) {}
    bool active, heater;
};

class ValveComponent : public Component {
public:
    ValveComponent(GameObject& owner, std::string puddleId, double x, double y, bool open)
        : Component(owner), puddleId(std::move(puddleId)), puddleX(x), puddleY(y), open(open) {}
    void initialize() override;
    void update() override {}
    void draw() override {}
    void toggle();
    void createPuddle();
    std::string puddleId;
    double puddleX, puddleY;
    bool open;
};

class RepairComponent : public PassiveComponent {
public:
    RepairComponent(GameObject& owner, std::string item) : PassiveComponent(owner), requiredItem(std::move(item)) {}
    std::string requiredItem;
    bool complete = false;
};

enum class CreatureState { Waiting, Moving, Settled };

class SlimeBehaviorComponent : public Component {
public:
    SlimeBehaviorComponent(GameObject& owner, double speed = 85) : Component(owner), speed(speed) {}
    void initialize() override;
    void update() override;
    void draw() override {}
    double speed;
    CreatureState state = CreatureState::Waiting;
    std::string targetId;
};

class HeatSeekingComponent : public Component {
public:
    HeatSeekingComponent(GameObject& owner, double speed = 105, double range = 280)
        : Component(owner), speed(speed), range(range) {}
    void initialize() override;
    void update() override;
    void draw() override {}
    double speed, range;
    CreatureState state = CreatureState::Waiting;
    std::string targetId;
};
