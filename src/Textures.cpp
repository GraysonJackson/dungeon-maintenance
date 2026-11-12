#include "Textures.h"
#include "Graphics.h"
#include <filesystem>
#include <stdexcept>

std::unordered_map<std::string, std::unique_ptr<SDL_Texture, Textures::SDL_Deleter>> Textures::textures;

SDL_Texture* Textures::get(const std::string& key) {
    auto it = textures.find(key);
    return it == textures.end() ? nullptr : it->second.get();
}

void Textures::clear() { textures.clear(); }

void Textures::loadFolder(const std::string& path) {
    if (!std::filesystem::is_directory(path))
        throw std::runtime_error("Texture folder not found: " + path);
    decltype(textures) loaded;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".bmp") continue;
        std::unique_ptr<SDL_Texture,SDL_Deleter> texture(Graphics::loadTexture(entry.path().string()));
        loaded.emplace(entry.path().stem().string(),std::move(texture));
    }
    if (loaded.empty()) throw std::runtime_error("No BMP textures found in: " + path);
    textures.swap(loaded);
}
