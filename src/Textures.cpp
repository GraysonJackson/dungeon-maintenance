#include "Textures.h"
#include <stdexcept>
#include <vector>

std::unordered_map<std::string, std::unique_ptr<SDL_Texture, Textures::SDL_Deleter>> Textures::textures;
SDL_Texture* Textures::get(const std::string& key) {
    auto it = textures.find(key);
    return it == textures.end() ? nullptr : it->second.get();
}
void Textures::clear() { textures.clear(); }

void Textures::loadDefaults(SDL_Renderer* renderer) {
    const std::vector<std::pair<std::string, SDL_Color>> kinds{
        {"player", {235,198,104,255}}, {"slime", {116,212,175,255}},
        {"creature", {237,146,83,255}}, {"crate", {154,110,72,255}},
        {"wall", {83,95,104,255}}, {"pump", {103,161,183,255}},
        {"valve", {207,119,101,255}}, {"part", {188,210,220,255}},
        {"coal", {249,154,71,255}}, {"heater", {203,144,89,255}},
        {"furnace", {162,116,99,255}}, {"puddle", {67,143,186,255}}
    };
    for (const auto& [key, color] : kinds) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface(
            SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_RGBA32), SDL_FreeSurface);
        if (!surface) throw std::runtime_error(SDL_GetError());
        auto fill = [&](int x, int y, int w, int h, SDL_Color c) {
            SDL_Rect rect{x,y,w,h};
            SDL_FillRect(surface.get(), &rect, SDL_MapRGBA(surface->format,c.r,c.g,c.b,c.a));
        };
        const SDL_Color dark{29,40,47,255}, pale{237,228,205,255};
        fill(0,0,32,32,{0,0,0,0});
        if (key == "wall") {
            fill(0,0,32,32,color); fill(0,0,32,2,{117,127,132,255});
            fill(0,15,32,2,dark); fill(16,0,2,15,dark); fill(7,17,2,15,dark);
        } else if (key == "player") {
            fill(7,3,18,9,color); fill(11,12,10,7,pale); fill(6,19,20,9,{85,140,158,255});
            fill(7,28,6,4,dark); fill(20,28,6,4,dark); fill(18,14,2,2,dark);
            fill(23,19,4,12,color);
        } else if (key == "slime" || key == "creature") {
            fill(4,12,24,16,color); fill(8,6,16,8,color);
            if (key == "creature") { fill(7,0,5,10,color); fill(20,0,5,10,color); }
            fill(10,15,3,4,dark); fill(20,15,3,4,dark); fill(5,27,22,3,dark);
            fill(8,9,6,3,{210,241,216,255});
        } else if (key == "puddle") {
            fill(3,9,26,15,color); fill(8,5,17,23,color);
            fill(7,12,11,2,{131,200,220,255}); fill(18,20,8,2,{131,200,220,255});
        } else if (key == "crate") {
            fill(1,1,30,30,dark); fill(3,3,26,26,color);
            fill(6,5,3,22,{207,161,102,255}); fill(23,5,3,22,{207,161,102,255});
            fill(3,14,26,4,dark);
        } else if (key == "part") {
            fill(4,9,24,14,color); fill(1,5,6,22,color); fill(25,5,6,22,color);
            fill(10,12,12,8,dark);
        } else if (key == "coal") {
            fill(6,10,20,17,dark); fill(10,5,12,22,color);
            fill(12,11,6,12,{255,217,123,255});
        } else if (key == "valve") {
            fill(13,0,6,32,{111,133,140,255}); fill(4,4,24,24,color);
            fill(8,8,16,16,dark); fill(14,4,4,24,color); fill(4,14,24,4,color);
        } else {
            fill(2,2,28,28,dark); fill(4,4,24,24,color);
            fill(8,9,16,15,dark);
            if (key == "pump") {
                fill(12,12,8,8,{107,200,221,255}); fill(0,23,32,5,color);
            } else {
                fill(11,16,10,7,{249,164,84,255}); fill(14,11,4,10,{255,215,129,255});
            }
        }
        auto* texture = SDL_CreateTextureFromSurface(renderer,surface.get());
        if (!texture) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        textures[key].reset(texture);
    }
}
