//
// Created by micodiy on 7/2/26.
//

#ifndef TASK1_GRAPH_H
#define TASK1_GRAPH_H

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "edge.hpp"
#include "edge_key.hpp"
#include "edge_key_hasher.hpp"
#include "graph_action_results.hpp"
#include "vertex.hpp"

class Graph {
    using VerticesPair = std::pair<std::reference_wrapper<Vertex>, std::reference_wrapper<Vertex>>;

    std::unordered_map<std::string, std::unique_ptr<Vertex>> vertices_;
    std::unordered_map<EdgeKey, std::unique_ptr<Edge>, EdgeKeyHasher> edges_;

    std::optional<std::reference_wrapper<Vertex>> try_get_vertex_mut(const std::string& name) {
        const auto it = vertices_.find(name);
        if (it == vertices_.end()) {
            return std::nullopt;
        }
        return *it->second;
    }

    std::expected<VerticesPair, GraphEdgeActionResult> try_get_edge_vertices(const std::string& src,
                                                                             const std::string& dest) {
        const auto a = try_get_vertex_mut(src);
        const auto b = try_get_vertex_mut(dest);

        if (not a.has_value() and not b.has_value()) {
            return std::unexpected(GraphEdgeActionResult::SrcAndDestUnknown);
        }
        if (not a.has_value()) {
            return std::unexpected(GraphEdgeActionResult::SrcUnknown);
        }
        if (not b.has_value()) {
            return std::unexpected(GraphEdgeActionResult::DestUnknown);
        }

        return VerticesPair{a->get(), b->get()};
    }

  public:
    GraphVertexActionResult add_vertex(const std::string& name) {
        if (vertices_.contains(name)) {
            return GraphVertexActionResult::VertexAlreadyExists;
        }

        vertices_[name] = std::make_unique<Vertex>(name);
        return GraphVertexActionResult::Success;
    }

    GraphEdgeActionResult add_edge(const std::string& src, const std::string& dest, i64 weight) {
        const auto res = try_get_edge_vertices(src, dest);
        if (not res.has_value()) {
            return res.error();
        }

        const auto [u, v] = res.value();
        const auto key = EdgeKey{.src = src, .dest = dest};

        if (edges_.contains(key)) {
            return GraphEdgeActionResult::EdgeAlreadyAdded;
        }

        auto edge = std::make_unique<Edge>(src, dest, weight);

        u.get().add_out_edge(edge.get());
        v.get().add_in_edge(edge.get());

        edges_[key] = std::move(edge);

        return GraphEdgeActionResult::Success;
    }

    GraphEdgeActionResult remove_edge(const std::string& src, const std::string& dest) {
        const auto res = try_get_edge_vertices(src, dest);
        if (not res.has_value()) {
            return res.error();
        }
        const auto [A, B] = res.value();

        const auto key = EdgeKey{.src = src, .dest = dest};
        const auto it = edges_.find(key);
        if (it == edges_.end()) {
            return GraphEdgeActionResult::EdgeUnknown;
        }

        A.get().remove_out_edge(it->second.get());
        B.get().remove_in_edge(it->second.get());
        edges_.erase(key);

        return GraphEdgeActionResult::Success;
    }

    GraphVertexActionResult remove_vertex(const std::string& name) {
        const auto u = try_get_vertex_mut(name);
        if (not u.has_value()) {
            return GraphVertexActionResult::UnknownVertex;
        }

        std::unordered_set<EdgeKey, EdgeKeyHasher> edges_to_remove;
        for (auto& e : vertices_[name].get()->get_edges_in()) {
            edges_to_remove.emplace(e->get_src_vertex_id(), e->get_dest_vertex_id());
        }
        for (auto& e : vertices_[name].get()->get_edges_out()) {
            edges_to_remove.emplace(e->get_src_vertex_id(), e->get_dest_vertex_id());
        }
        for (const auto& [src, dest] : edges_to_remove) {
            remove_edge(src, dest);
        }

        vertices_.erase(name);

        return GraphVertexActionResult::Success;
    }

    const Vertex& get_vertex(const std::string& name) const {
        return *vertices_.at(name).get();
    }

    bool has_vertex(const std::string& name) const {
        return vertices_.contains(name);
    }

    const std::unordered_map<EdgeKey, std::unique_ptr<Edge>, EdgeKeyHasher>& get_readonly_edges() const {
        return edges_;
    }

    const std::unordered_map<std::string, std::unique_ptr<Vertex>>& get_readonly_vertices() const {
        return vertices_;
    }
};

#endif // TASK1_GRAPH_H
