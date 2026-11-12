#pragma once

class GameObject;

class Component {
public:
    explicit Component(GameObject& owner) : _parent(owner) {}
    virtual ~Component() = default;
    virtual void initialize() {}
    virtual void update() = 0;
    virtual void draw() = 0;
    virtual int updateOrder() const { return 0; }
    GameObject& parent() const { return _parent; }

private:
    GameObject& _parent;
};
