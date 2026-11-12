#pragma once
#include <SDL2/SDL.h>
#include <set>
#include <utility>

class Input {
public:
    static void beginFrame();
    static void processEvent(const SDL_Event& event);
    static bool isKeyDown(SDL_Keycode key);
    static bool isKeyPressed(SDL_Keycode key);
    static std::pair<int, int> getMousePosition();
    static void reset();
private:
    static std::set<SDL_Keycode> keysDown, keysPressed;
    static std::pair<int, int> mousePosition;
};
