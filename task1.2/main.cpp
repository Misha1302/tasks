#include <iostream>
#include <set>
#include <string>

#include "../common/graph.hpp"
#include "../common/io.hpp"
#include "../common/types.hpp"


void print_edge_error_if_exists(const std::string &src, const std::string &dest, const Graph::EdgeActionResult result) {
    if (result == Graph::EdgeActionResult::SrcAndDestUnknown)
        std::cout << "Unknown nodes " << src << " " << dest << "\n";
    else if (result == Graph::EdgeActionResult::SrcUnknown)
        std::cout << "Unknown node " << src << "\n";
    else if (result == Graph::EdgeActionResult::DestUnknown)
        std::cout << "Unknown node " << dest << "\n";
}

std::unordered_map<std::string, i64> dejkstra(const Graph &graph, const std::string &start) {
    std::unordered_map<std::string, i64> distances;
    distances[start] = 0;

    std::set<std::pair<i64, std::string> > vertices;
    vertices.insert({0, start});

    while (!vertices.empty()) {
        auto [dist, name] = *--vertices.begin();
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
        } else if (cmd == "DIJKSTRA") {
            auto node_name = input<std::string>();
            auto dejkstra_distances = dejkstra(g, node_name);
            for (const auto &[name, dist]: dejkstra_distances) {
                std::cout << name << " " << dist << "\n";
            }
            std::cout.flush();
        }
    }
}

int main() {
    solve();
}
