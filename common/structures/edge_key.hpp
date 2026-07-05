//
// Created by micodiy on 7/6/26.
//

#ifndef GRAPH_TASKS_EDGEKEY_HPP
#define GRAPH_TASKS_EDGEKEY_HPP

#include <string>

struct EdgeKey {
    std::string src, dest;

    bool operator <(const EdgeKey &rhs) const {
        return src < rhs.src or (src == rhs.src and dest < rhs.dest);
    }

    bool operator ==(const EdgeKey &rhs) const {
        return src == rhs.src and dest == rhs.dest;
    }
};

#endif //GRAPH_TASKS_EDGEKEY_HPP
