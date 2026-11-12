#pragma once
#include "GameObject.h"
#include <tinyxml2.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class ComponentFactory {
public:
    ComponentFactory();
    std::unique_ptr<GameObject> create(const tinyxml2::XMLElement& element) const;
    std::unique_ptr<GameObject> fromString(const std::string& xml) const;
    std::vector<std::unique_ptr<GameObject>> load(const std::string& filename) const;
private:
    std::map<std::string, std::function<void(GameObject&, const tinyxml2::XMLElement&)>> creators;
};
