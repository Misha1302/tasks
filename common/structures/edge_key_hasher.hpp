//
// Created by micodiy on 7/6/26.
//

#ifndef GRAPH_TASKS_EDGE_KEY_HASHER_HPP
#define GRAPH_TASKS_EDGE_KEY_HASHER_HPP

#include "edge_key.hpp"

#include <cstddef>
#include <functional>

struct EdgeKeyHasher {
    template<typename T, typename... Rest>
    void hash_combine(std::size_t &seed, const T &v, const Rest &... rest) const {
        seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        (hash_combine(seed, rest), ...);
    }

    size_t operator()(const EdgeKey &p) const {
        std::size_t h = 0;
        hash_combine(h, p.src, p.dest);
        return h;
    }
};

#endif //GRAPH_TASKS_EDGE_KEY_HASHER_HPP
