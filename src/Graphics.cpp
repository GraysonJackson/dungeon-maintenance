#include "Graphics.h"
#include "PixelFont.h"
#include "Textures.h"
#include <memory>
#include <stdexcept>

SDL_Window* Graphics::window = nullptr;
SDL_Renderer* Graphics::renderer = nullptr;

bool Graphics::init(int width, int height, bool headless) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) return false;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0");
    window = SDL_CreateWindow("Dungeon Maintenance",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,
        width,height,headless ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window,-1,headless ? SDL_RENDERER_SOFTWARE :
                                  SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if (!renderer) return false;
    return SDL_RenderSetLogicalSize(renderer,width,height) == 0 &&
           SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND) == 0;
}

void Graphics::clean() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = nullptr; window = nullptr;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

SDL_Texture* Graphics::loadTexture(const std::string& path) {
    std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surface(SDL_LoadBMP(path.c_str()),SDL_FreeSurface);
    if (!surface) throw std::runtime_error("Cannot load texture " + path + ": " + SDL_GetError());
    // Magenta marks transparent pixels in the bundled BMP sprites.
    if (SDL_SetColorKey(surface.get(),SDL_TRUE,SDL_MapRGB(surface->format,255,0,255)) != 0)
        throw std::runtime_error("Cannot set transparency for " + path + ": " + SDL_GetError());
    std::unique_ptr<SDL_Texture,Textures::SDL_Deleter> texture(SDL_CreateTextureFromSurface(renderer,surface.get()));
    if (!texture || SDL_SetTextureBlendMode(texture.get(),SDL_BLENDMODE_BLEND) != 0)
        throw std::runtime_error("Cannot create texture " + path + ": " + SDL_GetError());
    return texture.release();
}

void Graphics::clear(SDL_Color color) {
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    SDL_RenderClear(renderer);
}

void Graphics::present() {
    if (!SDL_GetRenderTarget(renderer)) SDL_RenderPresent(renderer);
}

void Graphics::fillRect(int x, int y, int width, int height, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer,color.r,color.g,color.b,color.a);
    SDL_Rect area{x,y,width,height};
    SDL_RenderFillRect(renderer,&area);
}

void Graphics::drawTexture(const std::string& name, int x, int y, int width, int height,
                           double angle, SDL_Color tint) {
    auto* texture = Textures::get(name);
    if (!texture) throw std::runtime_error("Unknown texture: " + name);
    SDL_SetTextureColorMod(texture,tint.r,tint.g,tint.b);
    SDL_Rect destination{x,y,width,height};
    SDL_RenderCopyEx(renderer,texture,nullptr,&destination,angle,nullptr,SDL_FLIP_NONE);
}

void Graphics::drawText(const std::string& text, int x, int y, int scale, SDL_Color color) {
    ::drawText(renderer,text,x,y,scale,color);
}

void Graphics::capture(const std::string& path, void (*drawScene)()) {
    int width, height;
    SDL_RenderGetLogicalSize(renderer,&width,&height);
    std::unique_ptr<SDL_Texture,Textures::SDL_Deleter> target(
        SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_TARGET,width,height));
    if (!target) throw std::runtime_error(SDL_GetError());
    auto* previous = SDL_GetRenderTarget(renderer);
    if (SDL_SetRenderTarget(renderer,target.get()) != 0) throw std::runtime_error(SDL_GetError());
    try {
        // Read before Present: the backbuffer after Present is unspecified.
        drawScene();
        std::unique_ptr<SDL_Surface,decltype(&SDL_FreeSurface)> surface(
            SDL_CreateRGBSurfaceWithFormat(0,width,height,32,SDL_PIXELFORMAT_ARGB8888),SDL_FreeSurface);
        if (!surface || SDL_RenderReadPixels(renderer,nullptr,surface->format->format,surface->pixels,surface->pitch) != 0)
            throw std::runtime_error(SDL_GetError());
        if (SDL_SaveBMP(surface.get(),path.c_str()) != 0) throw std::runtime_error(SDL_GetError());
    } catch (...) {
        SDL_SetRenderTarget(renderer,previous);
        throw;
    }
    SDL_SetRenderTarget(renderer,previous);
}
