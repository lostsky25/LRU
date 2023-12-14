#pragma once

#include <list>
#include <unordered_map>
#include <vector>
#include <functional>

namespace caches {
    template <class T, class KeyT = int>
    struct cache_t {
    public:
        cache_t(size_t sz) : m_sz(sz) {}


        bool full() const { 
            return (m_cache.size() == m_sz);
        }

        // template <class Func>
        bool lookup_update(KeyT key, std::function<T(KeyT)> func) {
            auto el = m_hash.find(key);
            if (!m_hash.contains(key)) {
                if (full()) {
                    m_hash.erase(m_cache.back().first);
                    m_cache.pop_back();
                }
                m_cache.emplace_front(key, func(key));
                m_hash.emplace(key, m_cache.begin());
                // return false;
            }

            // if (el != m_cache.begin())
            //     m_cache.splice(m_cache.begin(), m_cache, el, std::next(el));
            return true;
        }

    // private:
        size_t m_sz;
        
        std::list<std::pair<KeyT, T>> m_cache;

        using ListIt = typename std::list<std::pair<KeyT, T>>::iterator;
        std::unordered_map<KeyT, ListIt> m_hash;
    };
}