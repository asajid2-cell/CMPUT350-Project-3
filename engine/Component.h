#pragma once

#include <cassert>
#include <vector>

namespace ECSEngine {

// A dense array of one component type with a free list over removed slots.
//
// Components are stored back to back so walking one type is a single linear pass. A slot is
// addressed by a "ticket" (an index into mStorage); removing a slot clears the object, marks the
// slot invalid and pushes the ticket on the free list so the next store reuses it.
//
// Lifetime: references into the storage point into mStorage. store() can grow that vector, so a
// reference is valid only until the next store(); the ticket is what stays valid.
template <typename T>
class ComponentStorage {
public:
    // Stores t and returns its ticket. A freed slot is reused before the array grows.
    size_t store(T&& t) {
        if (mFreeList.empty()) {
            mStorage.push_back(std::move(t));
            const auto idx = mStorage.size() - 1;
            mValid.push_back(1);
            return idx;
        }
        const auto newTag = mFreeList.back();
        assert(!valid(newTag));
        mFreeList.pop_back();
        mValid[newTag] = 1;
        (*this)[newTag] = (t);
        return newTag;
    }

    // Copying overload; see the move version above.
    size_t store(const T& t) {
        if (mFreeList.empty()) {
            mStorage.push_back(t);
            const auto idx = mStorage.size() - 1;
            mValid.push_back(1);
            return idx;
        }
        const auto newTag = mFreeList.back();
        assert(!valid(newTag));
        mFreeList.pop_back();
        mValid[newTag] = 1;
        (*this)[newTag] = (t);
        return newTag;
    }

    // Destroys the value in this slot (by assigning a default T) and frees the ticket.
    inline void remove(size_t t) {
        (*this)[t] = T{};  // asserts that the slot is valid
        mValid[t] = false;
        mFreeList.push_back(t);
    }

    // The value at this ticket. Asserts in debug if the ticket is not live.
    inline decltype(auto) operator[](size_t t) {
        assert(valid(t));
        return mStorage[t];
    }

    // Read-only access; same contract as the non-const overload.
    inline decltype(auto) operator[](size_t t) const {
        assert(valid(t));
        return mStorage[t];
    }

    // True when this ticket refers to a live slot. Tickets past the end are never valid.
    inline bool valid(size_t t) const { return t < size() ? mValid[t] : false; }

    inline size_t size() const { return mValid.size(); }

    using const_iterator = std::vector<T>::const_iterator;
    const_iterator cbegin() const { return mStorage.cbegin(); }
    const_iterator cend() const { return mStorage.cend(); }
    const_iterator begin() { return mStorage.begin(); }
    const_iterator end() { return mStorage.end(); }

    std::vector<T> mStorage;       // dense storage; a ticket indexes this
    std::vector<bool> mValid;      // mValid[t] is true while ticket t is live
    std::vector<size_t> mFreeList; // tickets freed by remove(), reused by store()
};

}  // namespace ECSEngine
