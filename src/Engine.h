#pragma once
#include "GameObject.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

class BodyComponent;

class Engine {
public:
    static constexpr int width = 1120, height = 820;
    static constexpr int worldWidth = 1040, worldHeight = 600, tileSize = 40;
    static constexpr int offsetX = 40, offsetY = 144;
    static bool init(const std::string& dataPath, bool headless = false);
    static void run(int frameLimit = 0);
    static void clean();
    static void handleEvents();
    static void update();
    static void render();
    static void step(double seconds);
    static void addGameObject(std::unique_ptr<GameObject> object);
    static void flushObjects();
    static GameObject* find(const std::string& id);
    static const std::vector<std::unique_ptr<GameObject>>& objects() { return gameObjects; }
    static void loadRoom(int room);
    static void interact(GameObject& player);
    static void pickUpOrDrop(GameObject& player);
    static void move(BodyComponent& body, double dx, double dy, bool canPush);
    static bool canOccupy(double x, double y, double w, double h,
                          const GameObject* ignore = nullptr, bool pathfinding = false);
    static std::vector<std::pair<double, double>> pathTo(const BodyComponent& body, double x, double y,
                                                       bool avoidActors = false);
    static bool stationBlocked(const BodyComponent& station);
    static GameObject* nearbyInteractable(const BodyComponent& player);
    static double distance(const BodyComponent& a, const BodyComponent& b);
    static void message(const std::string& text);
    static void capture(const std::string& path);
    static double deltaTime;
    static int currentRoom;
    static bool waterRestored, won, minimap;
    static std::string heldItem, dataPath;
private:
    static bool tryAxis(BodyComponent& body, double amount, bool horizontal, bool push, int depth);
    static void syncCarriedItem();
    static void drawInterface();
    static void drawMinimap();
    static bool isRunning;
    static std::vector<std::unique_ptr<GameObject>> gameObjects, pendingObjects;
    static std::string notification;
    static double notificationTime;
    static int requestedRoom;
};
