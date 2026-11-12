#pragma once
#include "Component.h"

class BodyComponent : public Component {
public:
    BodyComponent(GameObject& owner, double x, double y, double width = 28,
        double height = 28, double vx = 0, double vy = 0, double angle = 0, bool solid = false)
        : Component(owner), width(width), height(height), solid(solid),
          _x(x), _y(y), _vx(vx), _vy(vy), _angle(angle) {}
    double& x() { return _x; }
    double& y() { return _y; }
    double& vx() { return _vx; }
    double& vy() { return _vy; }
    double& angle() { return _angle; }
    double centerX() const { return _x + width / 2; }
    double centerY() const { return _y + height / 2; }
    void update() override;
    void draw() override {}
    int updateOrder() const override { return 20; }
    double width, height;
    bool solid;
private:
    double _x, _y, _vx, _vy, _angle;
};
