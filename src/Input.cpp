#include "Input.h"

std::set<SDL_Keycode> Input::keysDown, Input::keysPressed;
std::pair<int, int> Input::mousePosition{0, 0};
void Input::beginFrame() { keysPressed.clear(); }
void Input::reset() { keysDown.clear(); keysPressed.clear(); }
bool Input::isKeyDown(SDL_Keycode key) { return keysDown.count(key) != 0; }
bool Input::isKeyPressed(SDL_Keycode key) { return keysPressed.count(key) != 0; }
std::pair<int, int> Input::getMousePosition() { return mousePosition; }
void Input::processEvent(const SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (!event.key.repeat && !isKeyDown(event.key.keysym.sym))
            keysPressed.insert(event.key.keysym.sym);
        keysDown.insert(event.key.keysym.sym);
    } else if (event.type == SDL_KEYUP) {
        keysDown.erase(event.key.keysym.sym);
    } else if (event.type == SDL_MOUSEMOTION) {
        mousePosition = {event.motion.x, event.motion.y};
    } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        reset();
    }
}
