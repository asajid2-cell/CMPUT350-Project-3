#ifndef REGISTRY_H
#define REGISTRY_H
#include <stdio.h>
#include <stack>
#include <vector>
#include <queue>
#include <cassert>
#include <cstdint>
#include "Component.h"
#include "EntityManager.h"



template <typename T>
class Registry {

    public:
    size_t store(T&& t) {

        if (mAvailableTags.empty()) { // t is gonna sit at the back of the vector
            mComponents.push_back((t)); 

            auto idx = mComponents.size() - 1;
            mTags.push_back(idx);
            mValidTags.push_back(1);
            return idx;

        } else {
            auto newTag = mAvailableTags.back();
            assert(!valid(newTag));
            mAvailableTags.pop_back();

            mValidTags[newTag] = 1;

            (*this)[newTag] = (t);
            return newTag;

        }

    }

        size_t store(const T& t) {

        if (mAvailableTags.empty()) { // t is gonna sit at the back of the vector
            mComponents.push_back((t)); 

            auto idx = mComponents.size() - 1;
            mTags.push_back(idx);
            mValidTags.push_back(1);
            return idx;

        } else {
            auto newTag = mAvailableTags.back();
            assert(!valid(newTag));
            mAvailableTags.pop_back();

            mValidTags[newTag] = 1;

            (*this)[newTag] = (t);
            return newTag;

        }

    }


    inline void remove(size_t t) {
        /*
        "Noooo you cant just inline everything! Your binary size will explode!!!"

        "Haha inline go brrrrr"
        */

        (*this)[t] = T{}; // calls destructor for OG value. assertion implicitly called.
        mValidTags[t] = false;
        mAvailableTags.push_back(t);






    }

    inline decltype(auto) operator[](size_t t) {
        // decl type to also account for rvalues that cannot be bound to lvalues like for a vector of booleans.
        assert(valid(t));
        return mComponents[mTags[t]];
    }

    inline decltype(auto) operator[](size_t t) const {
        return assert(valid(t));
        return mComponents[mTags[t]];
    }

    inline bool valid(size_t t) const {
        return t < size() ? mValidTags[t] : false;
    }

    inline size_t size() const {
        return mComponents.size();
    }
    
    using const_iterator = std::vector<T>::const_iterator;
	const_iterator cbegin() const { return mComponents.begin(); }
	const_iterator cend() const { return mComponents.end(); }
	const_iterator begin() { return mComponents.begin(); }
	const_iterator end() { return mComponents.end(); }


    private:


    ECSEngine::ComponentStorage<T> mC;

    std::vector<T>& mComponents = mC.mStorage;
    std::vector<bool>& mValidTags = mC.mValid; // keeps track of all valid tags
    std::vector<size_t>& mAvailableTags = mC.mFreeList;

    std::vector<size_t> mTags; // maps an ID to its corresponding index in mComponents.

};



#endif  // REGISTRY_H