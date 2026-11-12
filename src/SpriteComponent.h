#pragma once
#include "Component.h"
#include <string>

class SpriteComponent : public Component {
public:
    SpriteComponent(GameObject& owner, std::string key) : Component(owner), textureKey(std::move(key)) {}
    void initialize() override;
    void update() override {}
    void draw() override;
    std::string textureKey;
};
