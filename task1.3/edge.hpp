//
// Created by micodiy on 7/2/26.
//

#ifndef TASK1_EDGE_H
#define TASK1_EDGE_H

#include <assert.h>
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

    void sub_weight(i64 value) {
        assert(value >= 0);
        weight_ -= value;
    }

    [[nodiscard]] i64 get_weight() const { return weight_; }

    [[nodiscard]] const std::string &get_src_vertex_id() const { return src_vertex_id_; }

    [[nodiscard]] const std::string &get_dest_vertex_id() const { return dest_vertex_id_; }

    bool operator <(const Edge &rhs) const {
        return weight_ < rhs.weight_
               or (
                   weight_ == rhs.weight_
                   and src_vertex_id_ < rhs.src_vertex_id_
               )
               or (
                   weight_ == rhs.weight_
                   and src_vertex_id_ == rhs.src_vertex_id_
                   and dest_vertex_id_ < rhs.dest_vertex_id_
               );
    }
};

#endif //TASK1_EDGE_H
