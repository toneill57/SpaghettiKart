#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

template<typename TObject>
class DataRegistry {
public:
    // Add object by copy or move; overwrites if exists
    void Add(const std::string& resourceName, TObject&& object) {
        mMap[resourceName] = std::make_unique<TObject>(std::forward<TObject>(object));
    }

    void Add(const std::string& resourceName, const TObject& object) {
        mMap[resourceName] = std::make_unique<TObject>(object);
    }

    // Get pointer to object (nullptr if not found)
    TObject* Get(const std::string& resourceName) {
        auto it = mMap.find(resourceName);
        return it != mMap.end() ? it->second.get() : nullptr;
    }

    const TObject* Get(const std::string& resourceName) const {
        auto it = mMap.find(resourceName);
        return it != mMap.end() ? it->second.get() : nullptr;
    }

    bool Remove(const std::string& resourceName) {
        return mMap.erase(resourceName) > 0;
    }

    size_t Size() const {
        return mMap.size();
    }

    void Clear() {
        mMap.clear();
    }

    // MK64 Online FASE 1.4b: read-only visit of every (name, object), for the oracle to name a pointed object by its
    // key. Visiting order is the map's (unordered): callers must not depend on it.
    template <typename F> void ForEach(F&& f) const {
        for (const auto& kv : mMap) {
            f(kv.first, *kv.second);
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<TObject>> mMap;
};
