#pragma once
#include <SDL2/SDL.h>
#include <string>

class Graphics {
public:
    static bool init(int width, int height, bool headless = false);
    static void clean();
    static SDL_Texture* loadTexture(const std::string& path);
    static void clear(SDL_Color color);
    static void present();
    static void fillRect(int x, int y, int width, int height, SDL_Color color);
    static void drawTexture(const std::string& name, int x, int y, int width, int height,
                            double angle = 0, SDL_Color tint = {255,255,255,255});
    static void drawText(const std::string& text, int x, int y, int scale = 2,
                         SDL_Color color = {222,225,214,255});
    static void capture(const std::string& path, void (*drawScene)());
private:
    static SDL_Window* window;
    static SDL_Renderer* renderer;
};
