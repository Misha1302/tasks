#include <iostream>
#include <string>

#include "../common/graph.hpp"
#include "../common/io.hpp"
#include "../common/types.hpp"

void get_post_order_internal(
    Graph &graph, const std::string &u, std::unordered_map<std::string, i64> &colors,
    std::vector<std::string> &output, std::vector<std::pair<std::string, std::string> > &loops
) {
    colors[u] = 1;

    for (const auto e: graph.get_vertex(u).get_edges_out()) {
        const auto &v = e->get_dest_vertex_id();

        if (colors[v] == 0)
            get_post_order_internal(graph, v, colors, output, loops);
        else if (colors[v] == 1)
            loops.emplace_back(u, v);
    }

    colors[u] = 2;
    output.push_back(u);
}

void print_reverse_post_order(Graph &graph, const std::string &start) {
    if (not graph.has_vertex(start)) {
        std::cout << "Unknown node " << start << "\n";
        return;
    }

    std::unordered_map<std::string, i64> colors;
    std::vector<std::string> output;
    std::vector<std::pair<std::string, std::string> > loops;
    get_post_order_internal(graph, start, colors, output, loops);

    for (const auto &[u, v]: loops)
        std::cout << "Found loop " << u << "->" << v << "\n";
    for (i64 i = static_cast<i64>(output.size()) - 1; i >= 0; i--)
        std::cout << output[i] << " ";
    std::cout << "\n";
}

void print_edge_error_if_exists(const std::string &src, const std::string &dest, const Graph::EdgeActionResult result) {
    if (result == Graph::EdgeActionResult::SrcAndDestUnknown)
        std::cout << "Unknown nodes " << src << " " << dest << "\n";
    else if (result == Graph::EdgeActionResult::SrcUnknown)
        std::cout << "Unknown node " << src << "\n";
    else if (result == Graph::EdgeActionResult::DestUnknown)
        std::cout << "Unknown node " << dest << "\n";
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
        } else if (cmd == "RPO_NUMBERING") {
            print_reverse_post_order(g, input<std::string>());
        }
    }
}

int main() {
    solve();
}
