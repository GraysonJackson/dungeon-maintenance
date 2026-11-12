#pragma once
#include "Component.h"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class GameObject final {
public:
    explicit GameObject(std::string id) : id(std::move(id)) {}
    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    template<class T, class... Args>
    void add(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        if (get<T>()) throw std::runtime_error("Duplicate component on " + id);
        components.push_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
    }

    template<class T>
    T* get() const {
        for (const auto& component : components)
            if (auto* result = dynamic_cast<T*>(component.get())) return result;
        return nullptr;
    }

    void initialize() {
        std::stable_sort(components.begin(), components.end(),
            [](const auto& a, const auto& b) { return a->updateOrder() < b->updateOrder(); });
        for (auto& component : components) component->initialize();
    }
    void update() { for (auto& component : components) component->update(); }
    void draw() { for (auto& component : components) component->draw(); }
    void destroy() { alive = false; }

    std::string id;
    bool alive = true;

private:
    std::vector<std::unique_ptr<Component>> components;
};
