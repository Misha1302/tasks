//
// Created by micodiy on 7/2/26.
//

#ifndef TASK1_GRAPH_H
#define TASK1_GRAPH_H

#include <expected>
#include <memory>
#include <unordered_map>
#include <vector>

#include "edge.hpp"
#include "vertex.hpp"


class Graph {
    struct EdgeKey {
        std::string src, dest;

        struct Hasher {
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

        bool operator <(const EdgeKey &rhs) const {
            return src < rhs.src or (src == rhs.src and dest < rhs.dest);
        }

        bool operator ==(const EdgeKey &rhs) const {
            return src == rhs.src and dest == rhs.dest;
        }
    };

    std::unordered_map<std::string, std::unique_ptr<Vertex> > vertices_;
    std::unordered_map<EdgeKey, std::unique_ptr<Edge>, EdgeKey::Hasher> edges_;


    std::optional<std::reference_wrapper<Vertex> > try_get_vertex_mut(const std::string &name) {
        const auto it = vertices_.find(name);
        if (it == vertices_.end()) return std::nullopt;
        return *it->second;
    }

public:
    enum class EdgeActionResult : u64 {
        Success = 1,
        SrcUnknown = 2,
        DestUnknown = 4,
        SrcAndDestUnknown = SrcUnknown | DestUnknown,
        EdgeUnknown = 8
    };

    enum class VertexActionResult : u64 {
        Success = 1,
        UnknownVertex = 2
    };

private:
    using VerticesPair = std::pair<
        std::reference_wrapper<Vertex>,
        std::reference_wrapper<Vertex>
    >;

    std::expected<VerticesPair, EdgeActionResult> get_edge_vertices(const std::string &src, const std::string &dest) {
        const auto A = try_get_vertex_mut(src);
        const auto B = try_get_vertex_mut(dest);

        if (not A.has_value() and not B.has_value()) return std::unexpected(EdgeActionResult::SrcAndDestUnknown);
        if (not A.has_value()) return std::unexpected(EdgeActionResult::SrcUnknown);
        if (not B.has_value()) return std::unexpected(EdgeActionResult::DestUnknown);

        return VerticesPair{A->get(), B->get()};
    }

public:
    void add_vertex(const std::string &name) {
        vertices_[name] = std::make_unique<Vertex>(name);
    }

    EdgeActionResult add_edge(const std::string &src, const std::string &dest, i64 weight) {
        const auto res = get_edge_vertices(src, dest);
        if (not res.has_value()) return res.error();
        const auto [u, v] = res.value();

        auto edge = std::make_unique<Edge>(src, dest, weight);

        u.get().add_out_edge(edge.get());
        v.get().add_in_edge(edge.get());

        const auto key = EdgeKey{edge->get_src_vertex_id(), edge->get_dest_vertex_id()};
        edges_[key] = std::move(edge);

        return EdgeActionResult::Success;
    }


    EdgeActionResult remove_edge(const std::string &src, const std::string &dest) {
        const auto res = get_edge_vertices(src, dest);
        if (not res.has_value()) return res.error();
        const auto [A, B] = res.value();

        const auto key = EdgeKey{src, dest};
        const auto it = edges_.find(key);
        if (it == edges_.end()) return EdgeActionResult::EdgeUnknown;

        A.get().remove_out_edge(it->second.get());
        B.get().remove_in_edge(it->second.get());
        edges_.erase(key);

        return EdgeActionResult::Success;
    }


    VertexActionResult remove_vertex(const std::string &name) {
        const auto u = try_get_vertex_mut(name);
        if (not u.has_value()) return VertexActionResult::UnknownVertex;

        std::unordered_set<EdgeKey, EdgeKey::Hasher> edges_to_remove;
        for (auto &e: vertices_[name].get()->get_edges_in())
            edges_to_remove.emplace(e->get_src_vertex_id(), e->get_dest_vertex_id());
        for (auto &e: vertices_[name].get()->get_edges_out())
            edges_to_remove.emplace(e->get_src_vertex_id(), e->get_dest_vertex_id());
        for (const auto &[src, dest]: edges_to_remove)
            remove_edge(src, dest);

        vertices_.erase(name);

        return VertexActionResult::Success;
    }

    Vertex &get_vertex(const std::string &name) const {
        return *vertices_.at(name).get();
    }

    bool has_vertex(const std::string &name) const {
        return vertices_.contains(name);
    }

    const std::unordered_map<EdgeKey, std::unique_ptr<Edge>, EdgeKey::Hasher> &get_readonly_edges() const {
        return edges_;
    }

    const std::unordered_map<std::string, std::unique_ptr<Vertex> > &get_readonly_vertices() const {
        return vertices_;
    }
};

#endif //TASK1_GRAPH_H
