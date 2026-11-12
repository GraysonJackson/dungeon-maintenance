#pragma once
#include <SDL2/SDL.h>
#include <memory>
#include <string>
#include <unordered_map>

class Textures {
public:
    struct SDL_Deleter { void operator()(SDL_Texture* texture) const { SDL_DestroyTexture(texture); } };
    static void loadDefaults(SDL_Renderer* renderer);
    static SDL_Texture* get(const std::string& key);
    static void clear();
private:
    static std::unordered_map<std::string, std::unique_ptr<SDL_Texture, SDL_Deleter>> textures;
};
