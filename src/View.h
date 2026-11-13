#pragma once
#include <SDL2/SDL.h>
#include <algorithm>
#include <cmath>

class View {
public:
    View(int x, int y, int width, int height)
        : viewport{x,y,width,height}, centerX(width/2.0), centerY(height/2.0) {}

    void follow(double x, double y, int worldWidth, int worldHeight) {
        centerX=std::clamp(x,viewport.w/2.0,worldWidth-viewport.w/2.0);
        centerY=std::clamp(y,viewport.h/2.0,worldHeight-viewport.h/2.0);
    }

    SDL_Point worldToScreen(double x, double y) const {
        return {viewport.x+static_cast<int>(std::round(x-centerX+viewport.w/2.0)),
                viewport.y+static_cast<int>(std::round(y-centerY+viewport.h/2.0))};
    }

    SDL_Rect viewport;
    double centerX, centerY;
};
