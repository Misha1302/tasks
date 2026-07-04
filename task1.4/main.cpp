#include <algorithm>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "edge.hpp"
#include "graph.hpp"
#include "io.hpp"
#include "types.hpp"


void print_edge_error_if_exists(const std::string &src, const std::string &dest, const Graph::EdgeActionResult result) {
    if (result == Graph::EdgeActionResult::SrcAndDestUnknown)
        std::cout << "Unknown nodes " << src << " " << dest << "\n";
    else if (result == Graph::EdgeActionResult::SrcUnknown)
        std::cout << "Unknown node " << src << "\n";
    else if (result == Graph::EdgeActionResult::DestUnknown)
        std::cout << "Unknown node " << dest << "\n";
}

class Tarjan {
    void pro_dfs(
        const Graph &graph,
        const std::string &u,
        std::unordered_map<std::string, i64> &tin,
        std::unordered_map<std::string, i64> &tmin,
        std::vector<std::string> &cur_component_stack,
        std::unordered_set<std::string> &cur_component_set,
        std::vector<std::vector<std::string> > &components,
        i64 &t
    ) {
        cur_component_stack.push_back(u);
        cur_component_set.insert(u);

        tin[u] = t++;
        tmin[u] = tin[u];

        for (auto &e: graph.get_vertex(u).get_edges_out()) {
            auto v = e->get_dest_vertex_id();
            if (not tin.contains(v)) {
                pro_dfs(graph, v, tin, tmin, cur_component_stack, cur_component_set, components, t);
                tmin[u] = std::min(tmin[u], tmin[v]);
            } else if (cur_component_set.contains(v)) {
                tmin[u] = std::min(tmin[u], tin[v]);
            }
        }

        if (tin[u] == tmin[u]) {
            components.emplace_back();
            while (true) {
                auto k = cur_component_stack.back();
                cur_component_set.erase(cur_component_stack.back());
                cur_component_stack.pop_back();
                components.back().push_back(k);
                if (u == k) break;
            }
        }
    }

public:
    std::vector<std::vector<std::string> > get_components(const Graph &graph, const std::string &u) {
        std::unordered_map<std::string, i64> tin, tmin;
        std::vector<std::string> cur_component_stack;
        std::unordered_set<std::string> cur_component_set;
        std::vector<std::vector<std::string> > components;
        i64 t = 0;

        pro_dfs(graph, u, tin, tmin, cur_component_stack, cur_component_set, components, t);

        return components;
    }
};

void solve() {
    Graph g;
    std::string cmd;
    while (std::cin >> cmd) {
        if (cmd == "NODE") {
            auto node_name = input<std::string>();
            g.add_vertex(node_name);
        } else if (cmd == "EDGE") {
            auto src = input<std::string>(), dest = input<std::string>();
            auto weight = input<i64>();

            const auto result = g.add_edge(src, dest, weight);
            print_edge_error_if_exists(src, dest, result);
        } else if (cmd == "REMOVE") {
            auto obj_to_delete = input<std::string>();
            if (obj_to_delete == "NODE") {
                auto node_name = input<std::string>();
                auto result = g.remove_vertex(node_name);
                if (result == Graph::VertexActionResult::UnknownVertex)
                    std::cout << "Unknown node " << node_name << "\n";
            } else if (obj_to_delete == "EDGE") {
                auto src = input<std::string>(), dest = input<std::string>();
                auto result = g.remove_edge(src, dest);
                print_edge_error_if_exists(src, dest, result);
            }
        } else if (cmd == "TARJAN") {
            auto u = input<std::string>();
            auto value = Tarjan{}.get_components(g, u);
            for (const auto &vec: value) {
                for (const auto &v: vec)
                    std::cout << v << " ";
                std::cout << "\n";
            }
        }
    }
}

int main() {
    solve();
}
