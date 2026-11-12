#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "ComponentFactory.h"
#include "ControllerComponent.h"
#include "Engine.h"
#include "Input.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
int checks=0;
void check(bool condition, const std::string& description) {
    if (!condition) throw std::runtime_error("TEST FAILED: "+description);
    ++checks; std::cout << "PASS " << description << '\n';
}
void tick(std::set<SDL_Keycode> keys={}, int frames=1) {
    const std::set<SDL_Keycode> all{SDLK_w,SDLK_a,SDLK_s,SDLK_d,SDLK_e,SDLK_SPACE,SDLK_r,SDLK_m};
    for (int i=0;i<frames;++i) {
        Input::beginFrame();
        for (auto key : all) {
            SDL_Event event{}; event.type=keys.count(key)?SDL_KEYDOWN:SDL_KEYUP;
            event.key.keysym.sym=key; Input::processEvent(event);
        }
        Engine::step(1.0/60.0);
    }
}
BodyComponent& body(const std::string& id) {
    auto* object=Engine::find(id);
    if (!object || !object->get<BodyComponent>()) throw std::runtime_error("Missing body: "+id);
    return *object->get<BodyComponent>();
}
void use(SDL_Keycode key) { tick(); tick({key}); tick(); }
void navigate(double x,double y,bool waitForFollower=false) {
    for (int frame=0;frame<2500;++frame) {
        auto& player=body("player");
        if (std::hypot(player.centerX()-x,player.centerY()-y)<5) { tick(); return; }
        auto path=Engine::pathTo(player,x,y);
        if (path.empty()) throw std::runtime_error("No player route");
        auto [tx,ty]=path.front();
        std::set<SDL_Keycode> keys;
        if (tx-player.centerX()>1.5) keys.insert(SDLK_d);
        if (tx-player.centerX()<-1.5) keys.insert(SDLK_a);
        if (ty-player.centerY()>1.5) keys.insert(SDLK_s);
        if (ty-player.centerY()<-1.5) keys.insert(SDLK_w);
        tick(keys);
        if (waitForFollower) tick();
    }
    auto& p=body("player");
    throw std::runtime_error("Player stuck at "+std::to_string(p.centerX())+","+std::to_string(p.centerY())+
                             " going to "+std::to_string(x)+","+std::to_string(y));
}
void capture(const std::string& directory,const std::string& name) {
    if (!directory.empty()) Engine::capture(directory+"/"+name+".bmp");
}
}

int runSelfTests(const std::string& captures) {
    if (!captures.empty()) std::filesystem::create_directories(captures);
    ComponentFactory factory;
    auto object=factory.fromString("<GameObject id='sample'><BodyComponent x='3' y='4' vx='5' vy='6' angle='15'/></GameObject>");
    auto* b=object->get<BodyComponent>();
    check(b && &b->parent()==object.get() && !object->get<ControllerComponent>(),"component lookup and parent reference");
    check(b->vx()==5 && b->vy()==6 && b->angle()==15,"XML body velocity and angle");
    Engine::deltaTime=1.0/60.0;
    object->update();
    check(std::abs(b->x()-(3+5.0/60))<0.0001 && std::abs(b->y()-(4+6.0/60))<0.0001,
          "BodyComponent integrates both velocity axes using delta time");
    for (const std::string invalid : {
        "<GameObject id='bad'><Unknown/></GameObject>",
        "<GameObject id='bad'><BodyComponent width='-2'/></GameObject>",
        "<GameObject id='bad'><BodyComponent/><BodyComponent/></GameObject>",
        "<GameObject id='bad'><BodyComponent x='oops'/></GameObject>",
        "<GameObject>"}) {
        bool rejected=false;
        try { factory.fromString(invalid); } catch (const std::runtime_error&) { rejected=true; }
        check(rejected,"invalid XML or component data rejected");
    }
    double x=body("player").x(),y=body("player").y();
    tick({SDLK_d,SDLK_s});
    check(std::abs(std::hypot(body("player").x()-x,body("player").y()-y)-3)<0.001,
          "diagonal movement normalized to straight movement speed");
    tick();
    x=body("player").x(); tick({},10);
    check(body("player").x()==x,"release stops movement");
    SDL_Event focus{}; focus.type=SDL_WINDOWEVENT; focus.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    tick({SDLK_d}); Input::processEvent(focus);
    check(!Input::isKeyDown(SDLK_d),"focus loss clears held keys");
    use(SDLK_m); check(Engine::minimap,"minimap toggles on");
    tick({SDLK_m},5); check(!Engine::minimap,"held toggle key fires once");
    tick();
    Engine::loadRoom(0); Engine::minimap=false;
    capture(captures,"pump_room");
    check(Engine::find("leak_puddle") && !Engine::find("diversion_puddle"),"initial puddles match XML valves");
    auto* valve=Engine::find("diversion_valve")->get<ValveComponent>();
    valve->toggle(); Engine::flushObjects();
    check(Engine::find("diversion_puddle")!=nullptr,"valve dynamically creates a composed puddle");
    valve->toggle(); Engine::flushObjects();
    check(!Engine::find("diversion_puddle"),"valve dynamically deletes puddle");
    Engine::loadRoom(0);
    tick({SDLK_a},120);
    check(body("player").x()>=40,"player cannot pass through outer wall");
    Engine::loadRoom(0);
    navigate(220,140); use(SDLK_SPACE);
    check(Engine::heldItem=="part","Space picks up the pump part");
    x=body("player").x(); tick({SDLK_d},5);
    check(body("player").x()>x && Engine::heldItem=="part" &&
          std::abs(body("part").centerX()-body("player").centerX())<0.001,"carried item follows a moving player");
    use(SDLK_SPACE); check(Engine::heldItem.empty(),"Space drops the item in free space");
    use(SDLK_SPACE); check(Engine::heldItem=="part","dropped item can be picked up again");
    navigate(340,380);
    x=body("player").x(); tick({SDLK_d,SDLK_e}); tick();
    check(Engine::find("diversion_puddle")!=nullptr && body("player").x()>x,
          "player moves and interacts with a valve in the same frame");
    navigate(420,460);
    double crateX=body("crate").x();
    tick({SDLK_d},60); tick();
    check(body("crate").x()>crateX && body("player").centerX()>520,"walking pushes crate and clears corridor");
    navigate(660,380); use(SDLK_e);
    check(!Engine::find("leak_puddle"),"E closes original leak");
    tick({},1400);
    auto* slime=Engine::find("slime")->get<SlimeBehaviorComponent>();
    check(slime->targetId=="diversion_puddle" && slime->state==CreatureState::Settled,
          "slime navigates through cleared corridor to active puddle");
    navigate(860,200); use(SDLK_e);
    check(Engine::waterRestored && !Engine::find("part") && Engine::heldItem.empty(),"pump repair consumes correct item and restores water");
    capture(captures,"pump_repaired");
    use(SDLK_e);
    check(Engine::currentRoom==1 && Engine::waterRestored,"repaired pump unlocks forge and preserves cooling");
    check(Engine::stationBlocked(body("furnace")),"forge creature initially blocks furnace controls");
    tick({SDLK_d},165); tick();
    navigate(700,460); use(SDLK_SPACE);
    check(Engine::heldItem=="coal","player collects coal in forge");
    navigate(740,300,true); tick({},60);
    check(Engine::find("forge_creature")->get<HeatSeekingComponent>()->targetId=="coal",
          "forge creature detects and follows carried coal");
    navigate(260,280,true); use(SDLK_e);
    check(Engine::find("heater")->get<HeatSourceComponent>()->active,"carried coal lights alternate heater");
    tick({},900);
    auto* creature=Engine::find("forge_creature")->get<HeatSeekingComponent>();
    check(creature->targetId=="heater" && creature->state==CreatureState::Settled,"forge creature settles at active heater");
    Engine::minimap=true;
    capture(captures,"forge_room");
    navigate(860,200);
    check(creature->targetId=="heater" && !Engine::stationBlocked(body("furnace")),"heater keeps creature occupied when player leaves");
    use(SDLK_e);
    check(Engine::won && !Engine::find("coal"),"forge repair consumes coal and passes inspection");
    Engine::minimap=false;
    capture(captures,"inspection_passed");
    use(SDLK_r);
    check(Engine::currentRoom==0 && !Engine::won && !Engine::waterRestored && Engine::heldItem.empty(),"R restarts after winning and resets progress");
    std::cout << "All " << checks << " checks passed.\n";
    return 0;
}
