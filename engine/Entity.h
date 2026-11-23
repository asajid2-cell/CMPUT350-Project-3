#pragma once

#include <string>

namespace ECSEngine {

// Index of an entity in the entity manager's entity array. An id can be reused after the entity is
// removed, so a stored id must be re-checked with EntityManager::IsValid before it is used.
using EntityID = size_t;

// A lightweight handle: an id and a display name. Components live in the EntityManager, not in the
// entity, so an Entity is cheap to store and copy. Only the manager (and the storage that clears
// removed slots) may construct one.
class Entity {
public:
    // The id this handle refers to.
    EntityID GetID() const { return mID; }

private:
    Entity() = default;
    Entity(EntityID eid, const std::string& name) : mID(eid), mType(name) {}

    EntityID mID;       // index into the entity array
    std::string mType;  // display name given at CreateEntity ("player", "star", ...)

    template <typename... T>
    friend class EntityManager;

    // The storage frees a slot by assigning T{}, so it needs the private default constructor.
    template <typename T>
    friend class ComponentStorage;
};

}  // namespace ECSEngine
