#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>
#include "Component.h"
#include "Entity.h"
#include "pack.h"
#include <functional>
#include <utility>

namespace ECSEngine {

// Owns the entities and every component type in the game. Each component type has its own dense
// array with its own free list (ComponentStorage); each entity keeps one ticket per component type
// in mEntityToComponent, so reading entity i's component is two lookups with no search. The entity
// list can have gaps after removals, so every pass must check IsValid on the ids it holds.
template <typename... Components>
class EntityManager {
public:
    // Ticket value meaning "this entity has no component of this type".
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // Creates an entity with this display name and returns its id. The id is an index into the
    // entity vector and is reused after RemoveEntity, so an id that was kept must be re-checked with
    // IsValid before it is trusted.
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        
        const auto& entity_idx = mEntitiesIdx.store({});
        mEntitiesIdx[entity_idx].mID = entity_idx; // set entity ID
        mEntitiesIdx[entity_idx].mType = name; // set type

        std::function<void(std::vector<size_t>&)> modifyVector;
        if (entity_idx < mEntityToComponent.at(0).size()) modifyVector = [&entity_idx](std::vector<size_t>& vec) -> void {vec[entity_idx] = kNullComponent;};
        else modifyVector = [&entity_idx](std::vector<size_t>& vec) -> void {vec.push_back(kNullComponent);};

        for (auto& vec : mEntityToComponent) modifyVector(vec);
        return entity_idx;
        
    }

    // Clears every component the entity owns, then frees its row so the slot can be handed out
    // again. The entity index itself stays in the vector -- callers must test IsValid before using
    // an id they kept, because a freed slot can come back as a different entity.
    void RemoveEntity(EntityID entity) {
        assert(mEntitiesIdx.valid(entity));

        RemoveAllComponents(entity, std::index_sequence_for<Components...>{});
        mEntitiesIdx.remove(entity);
    }

    // True when this id still refers to a live entity. Anything iterating the entity vector has to
    // ask this per slot, because removals leave dead gaps behind.
    bool IsValid(EntityID entity) const { return mEntitiesIdx.valid(entity); }

    // Gives this entity a copy of component. The component is stored in its type's dense array, so
    // adding one can reallocate that array: do not hold a reference from GetComponent across this
    // call, fetch it again afterwards.
    template <typename ComponentType>
    void AddComponent(EntityID id, ComponentType& component) {
        assert(mEntitiesIdx.valid(id));
        static_assert(contains<ComponentType>);
        constexpr auto index = idx<ComponentType>;

        const auto ticket = std::get<index>(mRegistries).store(component);
        SetComponentIdx<ComponentType>(id, ticket);    
        return;
    }

    // Removes one component type from an entity that has it. The entity itself stays alive.
    template <typename T>
    void RemoveComponent(EntityID entity) {
        assert(mEntitiesIdx.valid(entity)); // assert valid entity
        static_assert(contains<T>); 
        constexpr auto index = idx<T>;

        assert(HasComponent<T>(entity));

        const auto ticket = std::get<index>(mRegistries).remove(GetComponentIdx<T>(entity));
        GetComponentIdx<T>(entity) = kNullComponent;

    }

    // True when this live entity owns a component of type T.
    template <typename T>
    bool HasComponent(EntityID entity) const {
        return mEntityToComponent[idx<T>][entity] != kNullComponent;
    }

    // The entity's component of type T. The reference points into that type's dense array; it stays
    // valid until the next AddComponent/RemoveComponent/RemoveEntity on any entity, because those
    // can move the array. Re-fetch it after any structural change.
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        
        return std::get<idx<T>>(mRegistries)[GetComponentIdx<T>(entity)];
    }

    // The whole dense array for one component type, so a system can walk one storage instead of
    // round-tripping every entity through HasComponent. The reference lives as long as the manager.
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        static_assert(contains<T>);
        return std::get<idx<T>>(mRegistries);
    }

    using tEntity = std::vector<Entity>;

    // Iterates every slot, live or not. Systems check IsValid on each id before touching it, and
    // must not add or remove entities while an iterator is being held.
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntitiesIdx.cbegin(); }
    const_iterator cend() const { return mEntitiesIdx.cend(); }
    const_iterator begin() { return mEntitiesIdx.begin(); }
    const_iterator end() { return mEntitiesIdx.end(); }

    private:
    std::tuple<ComponentStorage<Components>...> mRegistries;
    std::array<std::vector<size_t>, Pack<Components...>::size> mEntityToComponent;
    ComponentStorage<Entity> mEntitiesIdx; 
    struct Pack<Components...> mPack;

    template <typename Component>
    static constexpr bool contains = Pack<Components...>::template contains<Component>;

    template <typename Component>
    static constexpr size_t idx = Pack<Components...>::template index<Component>;

    template<size_t t>
    using type = Pack<Components...>::template type<t>;

    template<typename T>
    inline void SetComponentIdx(const EntityID& e, const size_t& idx) {
        GetComponentIdx<T>(e) = idx;
    }

    template<typename T>
    inline size_t& GetComponentIdx(const EntityID& e) {
        static_assert(contains<T>);
        auto& index = mEntityToComponent[idx<T>][e];
        return index;

    }

    // Frees one component slot for this entity, if the entity has it. Used by RemoveEntity so the
    // sparse set does not leak the slot when the entity goes away.
    template <size_t I>
    inline void RemoveComponentAt(EntityID entity) {
        auto& slot = mEntityToComponent[I][entity];
        if (slot == kNullComponent) return;

        std::get<I>(mRegistries).remove(slot);
        slot = kNullComponent;
    }

    template <size_t... Is>
    inline void RemoveAllComponents(EntityID entity, std::index_sequence<Is...>) {
        (RemoveComponentAt<Is>(entity), ...);
    }


};

}  // namespace ECSEngine
