//
// Created by micodiy on 7/2/26.
//

#ifndef TASK1_VERTEX_H
#define TASK1_VERTEX_H

#include <string>
#include <unordered_set>
#include <utility>


class Vertex {
    std::string name_;
    std::unordered_set<const Edge *> edges_in_;
    std::unordered_set<const Edge *> edges_out_;

public:
    explicit Vertex(std::string name) : name_(std::move(name)) {
    }

    [[nodiscard]] const std::string &get_name() const { return name_; }

    [[nodiscard]] const std::unordered_set<const Edge *> &get_edges_in() const { return edges_in_; }

    [[nodiscard]] const std::unordered_set<const Edge *> &get_edges_out() const { return edges_out_; }


    void add_in_edge(const Edge *edge) {
        edges_in_.insert(edge);
    }

    void add_out_edge(const Edge *edge) {
        edges_out_.insert(edge);
    }

    void remove_in_edge(const Edge *edge) {
        edges_in_.erase(edge);
    }

    void remove_out_edge(const Edge *edge) {
        edges_out_.erase(edge);
    }

    void clear() {
        edges_in_.clear();
        edges_out_.clear();
    }
};

#endif //TASK1_VERTEX_H
