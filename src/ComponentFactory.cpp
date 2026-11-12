#include "ComponentFactory.h"
#include "BehaviorComponents.h"
#include "BodyComponent.h"
#include "ControllerComponent.h"
#include "SpriteComponent.h"
#include <cmath>
#include <set>
#include <stdexcept>

namespace {
std::string text(const tinyxml2::XMLElement& e, const char* key) {
    const char* value = e.Attribute(key);
    if (!value || !*value) throw std::runtime_error(std::string(e.Name()) + " missing " + key);
    return value;
}
double number(const tinyxml2::XMLElement& e, const char* key, double fallback) {
    double value = fallback;
    if (e.Attribute(key) && e.QueryDoubleAttribute(key,&value) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error(std::string("Invalid number: ") + key);
    if (!std::isfinite(value)) throw std::runtime_error(std::string("Nonfinite number: ") + key);
    return value;
}
bool boolean(const tinyxml2::XMLElement& e, const char* key, bool fallback) {
    bool value = fallback;
    if (e.Attribute(key) && e.QueryBoolAttribute(key,&value) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error(std::string("Invalid boolean: ") + key);
    return value;
}
double positive(const tinyxml2::XMLElement& e, const char* key, double fallback) {
    double value = number(e,key,fallback);
    if (value <= 0) throw std::runtime_error(std::string("Must be positive: ") + key);
    return value;
}
}

ComponentFactory::ComponentFactory() {
    creators["BodyComponent"] = [](GameObject& object, const tinyxml2::XMLElement& e) {
        object.add<BodyComponent>(number(e,"x",0),number(e,"y",0),positive(e,"width",28),
            positive(e,"height",28),number(e,"vx",0),number(e,"vy",0),number(e,"angle",0),
            boolean(e,"solid",false));
    };
    creators["SpriteComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<SpriteComponent>(text(e,"textureKey"));
    };
    creators["ControllerComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<ControllerComponent>(positive(e,"speed",180));
    };
    creators["SlimeBehaviorComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<SlimeBehaviorComponent>(positive(e,"speed",85));
    };
    creators["HeatSeekingComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<HeatSeekingComponent>(positive(e,"speed",105),positive(e,"range",280));
    };
    creators["CarryableComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<CarryableComponent>(text(e,"kind"));
    };
    creators["PushableComponent"] = [](GameObject& o, const tinyxml2::XMLElement&) { o.add<PushableComponent>(); };
    creators["PuddleComponent"] = [](GameObject& o, const tinyxml2::XMLElement&) { o.add<PuddleComponent>(); };
    creators["ValveComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<ValveComponent>(text(e,"puddleId"),number(e,"puddleX",0),number(e,"puddleY",0),boolean(e,"open",false));
    };
    creators["HeatSourceComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<HeatSourceComponent>(boolean(e,"active",false),boolean(e,"heater",true));
    };
    creators["RepairComponent"] = [](GameObject& o, const tinyxml2::XMLElement& e) {
        o.add<RepairComponent>(text(e,"requiredItem"));
    };
}

std::unique_ptr<GameObject> ComponentFactory::create(const tinyxml2::XMLElement& e) const {
    if (std::string(e.Name()) != "GameObject") throw std::runtime_error("Expected GameObject element");
    auto object = std::make_unique<GameObject>(text(e,"id"));
    for (auto* component = e.FirstChildElement(); component; component = component->NextSiblingElement()) {
        auto it = creators.find(component->Name());
        if (it == creators.end()) throw std::runtime_error(std::string("Unknown component: ") + component->Name());
        it->second(*object,*component);
    }
    if (!object->get<BodyComponent>()) throw std::runtime_error(object->id + " needs BodyComponent");
    return object;
}
std::unique_ptr<GameObject> ComponentFactory::fromString(const std::string& xml) const {
    tinyxml2::XMLDocument document;
    if (document.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS || !document.RootElement())
        throw std::runtime_error("Invalid object XML");
    return create(*document.RootElement());
}
std::vector<std::unique_ptr<GameObject>> ComponentFactory::load(const std::string& filename) const {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
        throw std::runtime_error("Cannot load " + filename + ": " + document.ErrorStr());
    auto* scene = document.FirstChildElement("Scene");
    if (!scene) throw std::runtime_error("XML needs a Scene root");
    std::vector<std::unique_ptr<GameObject>> objects;
    std::set<std::string> ids;
    for (auto* e = scene->FirstChildElement(); e; e = e->NextSiblingElement()) {
        auto object = create(*e);
        if (!ids.insert(object->id).second) throw std::runtime_error("Duplicate object id: " + object->id);
        objects.push_back(std::move(object));
    }
    return objects;
}
