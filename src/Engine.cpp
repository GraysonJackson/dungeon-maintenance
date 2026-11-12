#include "Engine.h"
#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "ComponentFactory.h"
#include "ControllerComponent.h"
#include "Input.h"
#include "Graphics.h"
#include "SpriteComponent.h"
#include "Textures.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <stdexcept>

bool Engine::isRunning = false;
std::vector<std::unique_ptr<GameObject>> Engine::gameObjects, Engine::pendingObjects;
std::string Engine::heldItem, Engine::dataPath, Engine::notification;
double Engine::deltaTime = 1.0/60.0, Engine::notificationTime = 0;
int Engine::currentRoom = 0, Engine::requestedRoom = -1;
bool Engine::waterRestored = false, Engine::won = false, Engine::minimap = false;

namespace {
bool overlaps(double x, double y, double w, double h, BodyComponent& b) {
    return x < b.x()+b.width && x+w > b.x() && y < b.y()+b.height && y+h > b.y();
}
bool actor(const GameObject& object) {
    return object.get<ControllerComponent>() || object.get<SlimeBehaviorComponent>() || object.get<HeatSeekingComponent>();
}
bool isCarried(const GameObject& object) {
    auto* item = object.get<CarryableComponent>();
    return item && item->carried;
}
}

bool Engine::init(const std::string& path, bool headless) {
    dataPath = path;
    if (SDL_Init(SDL_INIT_TIMER) != 0) return false;
    if (!Graphics::init(width,height,headless)) return false;
    Textures::loadFolder(dataPath+"/textures");
    isRunning=true;
    loadRoom(0);
    return true;
}
void Engine::clean() {
    gameObjects.clear(); pendingObjects.clear();
    Textures::clear();
    Graphics::clean();
    isRunning=false;
    SDL_Quit();
}
void Engine::handleEvents() {
    Input::beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) isRunning=false;
        Input::processEvent(event);
    }
    if (Input::isKeyPressed(SDLK_ESCAPE)) isRunning=false;
}
void Engine::run(int frameLimit) {
    Uint64 previous=SDL_GetPerformanceCounter();
    int frames=0;
    while (isRunning && (!frameLimit || frames < frameLimit)) {
        Uint64 now=SDL_GetPerformanceCounter();
        double seconds=static_cast<double>(now-previous)/SDL_GetPerformanceFrequency();
        previous=now;
        handleEvents(); step(seconds); render();
        ++frames;
        SDL_Delay(1);
    }
}
void Engine::step(double seconds) {
    deltaTime=std::clamp(seconds,0.0001,0.05);
    update();
}
void Engine::update() {
    if (Input::isKeyPressed(SDLK_m)) minimap=!minimap;
    if (Input::isKeyPressed(SDLK_r)) requestedRoom=won ? 0 : currentRoom;
    notificationTime=std::max(0.0,notificationTime-deltaTime);
    if (!won) {
        for (auto& object : gameObjects) if (object->alive) object->update();
        syncCarriedItem();
    }
    flushObjects();
    if (requestedRoom >= 0) { int next=requestedRoom; requestedRoom=-1; loadRoom(next); }
}
void Engine::addGameObject(std::unique_ptr<GameObject> object) {
    if (find(object->id)) throw std::runtime_error("Duplicate object id: "+object->id);
    pendingObjects.push_back(std::move(object));
}
void Engine::flushObjects() {
    gameObjects.erase(std::remove_if(gameObjects.begin(),gameObjects.end(),
        [](const auto& o) { return !o->alive; }),gameObjects.end());
    while (!pendingObjects.empty()) {
        auto batch=std::move(pendingObjects);
        pendingObjects.clear();
        for (auto& object : batch) {
            if (!object->alive) continue;
            auto* ptr=object.get();
            gameObjects.push_back(std::move(object));
            ptr->initialize();
        }
    }
}
GameObject* Engine::find(const std::string& id) {
    for (const auto& object : gameObjects) if (object->alive && object->id == id) return object.get();
    for (const auto& object : pendingObjects) if (object->alive && object->id == id) return object.get();
    return nullptr;
}
void Engine::loadRoom(int room) {
    auto objects=ComponentFactory().load(dataPath+(room == 0 ? "/pump.xml" : "/forge.xml"));
    gameObjects.clear(); pendingObjects.clear(); heldItem.clear(); Input::reset();
    currentRoom=room; won=false; requestedRoom=-1;
    if (room == 0) waterRestored=false;
    // Add the whole scene before initialization so components can find their neighbors.
    gameObjects=std::move(objects);
    for (auto& object : gameObjects) object->initialize();
    flushObjects();
    message(room == 0 ? "RESTORE THE PUMP. VALVES CAN MOVE SLIMES." :
                          "COOLING RESTORED. BRING COAL TO THE HEATER.");
}
double Engine::distance(const BodyComponent& a, const BodyComponent& b) {
    return std::hypot(a.centerX()-b.centerX(),a.centerY()-b.centerY());
}
void Engine::message(const std::string& text) { notification=text; notificationTime=4; }

bool Engine::canOccupy(double x, double y, double w, double h, const GameObject* ignore, bool pathfinding) {
    if (x < 0 || y < 0 || x+w > worldWidth || y+h > worldHeight) return false;
    for (const auto& object : gameObjects) {
        if (!object->alive || object.get() == ignore || isCarried(*object) || (pathfinding && actor(*object))) continue;
        auto* body=object->get<BodyComponent>();
        if (body && body->solid && overlaps(x,y,w,h,*body)) return false;
    }
    return true;
}
bool Engine::tryAxis(BodyComponent& body, double amount, bool horizontal, bool push, int depth) {
    if (amount == 0) return true;
    double x=body.x()+(horizontal ? amount : 0), y=body.y()+(horizontal ? 0 : amount);
    if (x<0 || y<0 || x+body.width>worldWidth || y+body.height>worldHeight) return false;
    for (const auto& object : gameObjects) {
        if (!object->alive || object.get() == &body.parent() || isCarried(*object)) continue;
        auto* other=object->get<BodyComponent>();
        if (!other || !other->solid || !overlaps(x,y,body.width,body.height,*other)) continue;
        if (!push || depth>=4 || !object->get<PushableComponent>() ||
            !tryAxis(*other,amount,horizontal,true,depth+1)) return false;
    }
    body.x()=x; body.y()=y;
    return true;
}
void Engine::move(BodyComponent& body, double dx, double dy, bool canPush) {
    if (!body.solid) { body.x()+=dx; body.y()+=dy; return; }
    for (auto [amount,horizontal] : {std::pair<double,bool>{dx,true},{dy,false}}) {
        std::vector<std::pair<BodyComponent*,std::pair<double,double>>> before;
        if (canPush) for (const auto& object : gameObjects)
            if (object->alive && object->get<PushableComponent>()) {
                auto* b=object->get<BodyComponent>(); before.push_back({b,{b->x(),b->y()}});
            }
        if (!tryAxis(body,amount,horizontal,canPush,0))
            for (auto [b,pos] : before) { b->x()=pos.first; b->y()=pos.second; }
    }
}

std::vector<std::pair<double,double>> Engine::pathTo(const BodyComponent& body, double x, double y, bool avoidActors) {
    constexpr int cols=worldWidth/tileSize, rows=worldHeight/tileSize;
    auto cell=[](double px,double py) {
        return std::clamp(static_cast<int>(py/tileSize),0,rows-1)*cols+
               std::clamp(static_cast<int>(px/tileSize),0,cols-1);
    };
    int start=cell(body.centerX(),body.centerY()), goal=cell(x,y);
    std::array<int,cols*rows> previous; previous.fill(-1);
    std::queue<int> queue; queue.push(start); previous[start]=start;
    while (!queue.empty() && previous[goal] == -1) {
        int current=queue.front(); queue.pop();
        for (auto [dx,dy] : {std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}) {
            int cx=current%cols+dx, cy=current/cols+dy;
            if (cx<0 || cy<0 || cx>=cols || cy>=rows) continue;
            int next=cy*cols+cx;
            if (previous[next] != -1 || !canOccupy(cx*tileSize+(tileSize-body.width)/2,
                cy*tileSize+(tileSize-body.height)/2,body.width,body.height,&body.parent(),!avoidActors)) continue;
            previous[next]=current; queue.push(next);
        }
    }
    if (previous[goal] == -1) return {};
    std::vector<std::pair<double,double>> path;
    for (int at=goal; at!=start; at=previous[at])
        path.push_back({(at%cols+0.5)*tileSize,(at/cols+0.5)*tileSize});
    std::reverse(path.begin(),path.end());
    path.push_back({x,y});
    return path;
}

GameObject* Engine::nearbyInteractable(const BodyComponent& player) {
    GameObject* nearest=nullptr; double best=74;
    for (const auto& object : gameObjects) {
        if (!object->alive || !(object->get<ValveComponent>() || object->get<RepairComponent>() ||
            (object->get<HeatSourceComponent>() && object->get<HeatSourceComponent>()->heater))) continue;
        double d=distance(player,*object->get<BodyComponent>());
        if (d<best) { best=d; nearest=object.get(); }
    }
    return nearest;
}
bool Engine::stationBlocked(const BodyComponent& station) {
    for (const auto& object : gameObjects)
        if (object->alive && (object->get<SlimeBehaviorComponent>() || object->get<HeatSeekingComponent>()) &&
            distance(station,*object->get<BodyComponent>())<110) return true;
    return false;
}
void Engine::interact(GameObject& player) {
    auto* object=nearbyInteractable(*player.get<BodyComponent>());
    if (!object) { message("MOVE CLOSER TO A VALVE OR REPAIR STATION."); return; }
    if (auto* valve=object->get<ValveComponent>()) { valve->toggle(); return; }
    auto* held=find(heldItem);
    auto* item=held ? held->get<CarryableComponent>() : nullptr;
    if (auto* heat=object->get<HeatSourceComponent>(); heat && heat->heater) {
        if (heat->active) { message("HEATER IS LIT. LET THE CREATURE SETTLE HERE."); return; }
        if (!item || item->kind != "coal") { message("CARRY HOT COAL HERE TO LIGHT THE HEATER."); return; }
        heat->active=true; message("HEATER LIT. MOVE AWAY ONCE THE CREATURE SETTLES."); return;
    }
    auto* repair=object->get<RepairComponent>();
    if (!repair) return;
    if (repair->complete) { if (currentRoom==0) requestedRoom=1; return; }
    if (stationBlocked(*object->get<BodyComponent>())) { message("CREATURE BLOCKING CONTROLS. LURE IT AWAY."); return; }
    if (!item || item->kind != repair->requiredItem) { message("BRING THE "+repair->requiredItem+" TO THIS STATION."); return; }
    if (currentRoom==1 && !waterRestored) { message("THE FORGE NEEDS COOLING FROM THE PUMP."); return; }
    held->destroy(); heldItem.clear(); repair->complete=true;
    if (currentRoom==0) { waterRestored=true; message("PUMP REPAIRED. PRESS E HERE TO ENTER THE FORGE."); }
    else { won=true; message("BOTH SYSTEMS RESTORED. INSPECTION PASSED!"); }
}
void Engine::pickUpOrDrop(GameObject& player) {
    auto* body=player.get<BodyComponent>();
    if (auto* held=find(heldItem)) {
        auto* b=held->get<BodyComponent>();
        for (auto [dx,dy] : {std::pair<int,int>{0,38},{38,0},{0,-38},{-38,0}}) {
            double x=body->centerX()+dx-b->width/2, y=body->centerY()+dy-b->height/2;
            if (!canOccupy(x,y,b->width,b->height,held)) continue;
            b->x()=x; b->y()=y; held->get<CarryableComponent>()->carried=false;
            heldItem.clear(); message("ITEM DROPPED."); return;
        }
        message("NO ROOM TO DROP THE ITEM."); return;
    }
    GameObject* nearest=nullptr; double best=64;
    for (const auto& object : gameObjects) {
        if (!object->alive || !object->get<CarryableComponent>()) continue;
        double d=distance(*body,*object->get<BodyComponent>());
        if (d<best) { best=d; nearest=object.get(); }
    }
    if (!nearest) { message("NO ITEM NEARBY. SPACE PICKS UP OR DROPS ONE ITEM."); return; }
    heldItem=nearest->id; nearest->get<CarryableComponent>()->carried=true;
    syncCarriedItem(); message("CARRYING "+nearest->get<CarryableComponent>()->kind+".");
}
void Engine::syncCarriedItem() {
    auto* held=find(heldItem); auto* player=find("player");
    if (!held || !player) return;
    auto* b=held->get<BodyComponent>(); auto* p=player->get<BodyComponent>();
    b->x()=p->centerX()-b->width/2; b->y()=p->y()-b->height+5;
}

void Engine::drawInterface() {
    const SDL_Color muted{145,160,166,255}, gold{235,198,104,255}, mint{116,212,175,255};
    Graphics::drawText("DUNGEON MAINTENANCE",40,28,3,gold);
    Graphics::drawText(currentRoom==0 ? "01 / PLUMBING PROBLEMS" : "02 / OCCUPATIONAL HAZARD",40,68,2);
    Graphics::drawText("MAINTENANCE CHECKLIST",650,28,2,muted);
    Graphics::drawText(waterRestored ? "[X] WATER SYSTEM" : "[ ] WATER SYSTEM",650,55,2,waterRestored?mint:muted);
    Graphics::drawText(won ? "[X] FORGE" : "[ ] FORGE",650,79,2,won?mint:muted);
    auto* held=find(heldItem);
    Graphics::drawText("CARRY: "+(held?held->get<CarryableComponent>()->kind:"EMPTY"),40,111,2,mint);
    Graphics::drawText("WASD / ARROWS: MOVE   E: USE   SPACE: CARRY   M: MAP   R: RESTART   ESC: QUIT",40,762,2,muted);
    std::string hint=currentRoom==0 ? "FIND THE PART. CLOSE THE LEAK. OPEN THE DIVERSION VALVE." :
                                      "LEAD THE CREATURE TO THE HEATER. THEN FUEL THE FURNACE.";
    if (auto* player=find("player")) if (auto* near=nearbyInteractable(*player->get<BodyComponent>())) {
        hint="E: "+near->id;
        if (auto* valve=near->get<ValveComponent>()) hint+=valve->open?" / OPEN":" / CLOSED";
        if (auto* repair=near->get<RepairComponent>(); repair && repair->complete) hint+=" / REPAIRED - ENTER FORGE";
    }
    Graphics::drawText(notificationTime>0 ? notification : hint,40,792,2,notificationTime>0?gold:mint);
}
void Engine::drawMinimap() {
    if (!minimap) return;
    constexpr int x=800,y=556,w=264,h=178;
    Graphics::fillRect(x,y,w,h,{14,25,32,245});
    Graphics::drawText("LIVE MAP / M",x+10,y+10,1,{235,198,104,255});
    for (const auto& object : gameObjects) {
        if (!object->alive) continue;
        auto* b=object->get<BodyComponent>();
        SDL_Color c{92,113,126,255};
        if (object->get<ControllerComponent>()) c={245,214,119,255};
        else if (object->get<SlimeBehaviorComponent>()) c={116,212,175,255};
        else if (object->get<HeatSeekingComponent>()) c={237,146,83,255};
        else if (object->get<PuddleComponent>()) c={79,168,211,255};
        else if (object->get<CarryableComponent>()) c={239,231,206,255};
        Graphics::fillRect(x+8+static_cast<int>(b->x()*0.24),y+27+static_cast<int>(b->y()*0.24),
             std::max(3,static_cast<int>(b->width*0.24)),std::max(3,static_cast<int>(b->height*0.24)),c);
    }
}
void Engine::render() {
    Graphics::clear({19,30,38,255});
    for (int y=0;y<worldHeight;y+=tileSize) for (int x=0;x<worldWidth;x+=tileSize) {
        bool alt=(x/tileSize+y/tileSize)%2;
        SDL_Color c=currentRoom==0 ? SDL_Color{static_cast<Uint8>(38+alt*3),static_cast<Uint8>(53+alt*3),static_cast<Uint8>(61+alt*3),255}
                                 : SDL_Color{static_cast<Uint8>(57+alt*3),static_cast<Uint8>(47+alt*3),static_cast<Uint8>(44+alt*3),255};
        Graphics::fillRect(x+offsetX,y+offsetY,tileSize-1,tileSize-1,c);
    }
    for (int layer=0;layer<4;++layer) for (auto& object : gameObjects) {
        if (!object->alive) continue;
        int objectLayer=object->get<PuddleComponent>()?0:isCarried(*object)?3:actor(*object)?2:1;
        if (objectLayer==layer) object->draw();
    }
    const SDL_Color label{181,193,191,255};
    if (currentRoom==0) {
        Graphics::drawText("PUMP",offsetX+842,offsetY+98,1,label);
        Graphics::drawText("LEAK VALVE",offsetX+620,offsetY+338,1,label);
        Graphics::drawText("DIVERSION",offsetX+300,offsetY+338,1,label);
        Graphics::drawText("PART",offsetX+200,offsetY+100,1,label);
    } else {
        Graphics::drawText("FURNACE",offsetX+832,offsetY+98,1,label);
        Graphics::drawText("HEATER",offsetX+240,offsetY+178,1,label);
        Graphics::drawText("HOT COAL",offsetX+660,offsetY+420,1,label);
    }
    drawInterface(); drawMinimap();
    if (won) {
        Graphics::fillRect(240,320,640,168,{19,30,38,245});
        Graphics::drawText("INSPECTION PASSED",290,350,3,{116,212,175,255});
        Graphics::drawText("WATER AND FORGE RESTORED",292,400,2);
        Graphics::drawText("R: PLAY AGAIN",292,440,2,{235,198,104,255});
    }
    Graphics::present();
}
void Engine::capture(const std::string& path) {
    Graphics::capture(path,&Engine::render);
}
