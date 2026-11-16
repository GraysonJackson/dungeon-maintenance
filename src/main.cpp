#define SDL_MAIN_HANDLED
#include "Engine.h"
#include <SDL2/SDL.h>
#include <charconv>
#include <filesystem>
#include <iostream>
#include <stdexcept>

int runSelfTests(const std::string& captureDirectory);

int main(int argc, char** argv) {
    try {
        bool test=false;
        int frames=0;
        std::string data, captures;
        for (int i=1;i<argc;++i) {
            std::string arg=argv[i];
            if (arg=="--self-test") test=true;
            else if (arg=="--data" && i+1<argc) data=argv[++i];
            else if (arg=="--frames" && i+1<argc) frames=std::stoi(argv[++i]);
            else if (arg=="--fps" && i+1<argc) {
                const std::string value=argv[++i];
                const char* end=value.data()+value.size();
                auto [parsed, error]=std::from_chars(value.data(),end,Engine::targetFps);
                if (error != std::errc{} || parsed != end || Engine::targetFps <= 0)
                    throw std::runtime_error("--fps requires a positive whole number");
            }
            else if (arg=="--capture-dir" && i+1<argc) captures=argv[++i];
            else throw std::runtime_error("Usage: dungeon-maintenance [--fps N] [--self-test] [--data DIR] [--frames N] [--capture-dir DIR]");
        }
        if (test && !SDL_getenv("SDL_VIDEODRIVER")) SDL_setenv("SDL_VIDEODRIVER","dummy",1);
        SDL_SetMainReady();
        if (data.empty()) {
            std::filesystem::path executable=std::filesystem::absolute(argv[0]).parent_path();
            char* base=SDL_GetBasePath();
            if (base) { executable=base; SDL_free(base); }
            for (const auto& candidate : {std::filesystem::path("data"), executable/"data", executable/"../data"})
                if (std::filesystem::exists(candidate/"pump.xml")) { data=candidate.string(); break; }
        }
        if (data.empty()) throw std::runtime_error("Cannot find data/pump.xml. Use --data DIR.");
        if (!Engine::init(data,test)) throw std::runtime_error(SDL_GetError());
        int result=0;
        if (test) result=runSelfTests(captures);
        else Engine::run(frames);
        Engine::clean();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        Engine::clean();
        return 1;
    }
}
