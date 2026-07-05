#include <iostream>
#include <set>
#include <string>
#include <unordered_map>

#include "../common/io/task_repl.hpp"
#include "../common/structures/graph.hpp"
#include "../common/io/io.hpp"
#include "../common/types.hpp"

std::unordered_map<std::string, i64> dejkstra(const Graph &graph, const std::string &start) {
    std::unordered_map<std::string, i64> distances;
    distances[start] = 0;

    std::set<std::pair<i64, std::string> > vertices;
    vertices.insert({0, start});

    while (!vertices.empty()) {
        auto [dist, name] = *vertices.begin();
        vertices.erase(vertices.begin());
        if (dist != distances[name]) continue;

        for (const auto e: graph.get_vertex(name).get_edges_out()) {
            auto new_dist = dist + e->get_weight();
            auto v = e->get_dest_vertex_id();
            if (!distances.contains(v) or new_dist < distances[v]) {
                vertices.insert({new_dist, v});
                distances[v] = new_dist;
            }
        }
    }

    distances.erase(start);
    return distances;
}


void solve() {
    TaskRepl task_repl;
    task_repl.register_cmd("DIJKSTRA", [](Graph &g) {
        const auto node_name = input<std::string>();
        if (not g.has_vertex(node_name)) {
            std::cout << "Unknown node " << node_name << "\n";
            return;
        }

        const auto dejkstra_distances = dejkstra(g, node_name);
        for (const auto &[name, dist]: dejkstra_distances) {
            std::cout << name << " " << dist << "\n";
        }
    });
    task_repl.start();
}

int main() {
    solve();
}
