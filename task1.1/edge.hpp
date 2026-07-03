//
// Created by micodiy on 7/2/26.
//

#ifndef TASK1_EDGE_H
#define TASK1_EDGE_H

#include <string>
#include <utility>

#include "types.hpp"

class Edge {
    std::string src_vertex_id_;
    std::string dest_vertex_id_;
    i64 weight_;

public:
    explicit Edge(std::string src_vertex_id, std::string dest_vertex_id, i64 weight)
        : src_vertex_id_(std::move(src_vertex_id)), dest_vertex_id_(std::move(dest_vertex_id)), weight_(weight) {
    }

    [[nodiscard]] i64 get_weight() const { return weight_; }

    [[nodiscard]] const std::string &get_src_vertex_id() const { return src_vertex_id_; }

    [[nodiscard]] const std::string &get_dest_vertex_id() const { return dest_vertex_id_; }
};

#endif //TASK1_EDGE_H
