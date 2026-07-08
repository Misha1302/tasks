#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common/io/io.hpp"
#include "../common/io/task_repl.hpp"
#include "../common/structures/graph.hpp"
#include "../common/types.hpp"

enum class ProDfsColors : u8 { NotVisited, InVisiting, Visited };

using ColorsMap = std::unordered_map<std::string, ProDfsColors>;
using LoopsVec = std::vector<std::pair<std::string, std::string>>;

void get_post_order_internal(
    const Graph& graph, const std::string& u, ColorsMap& colors, std::vector<std::string>& output, LoopsVec& loops) {
    colors[u] = ProDfsColors::InVisiting;

    for (const auto e : graph.get_vertex(u).get_edges_out()) {
        const auto& v = e->get_dest_vertex_id();

        if (colors[v] == ProDfsColors::NotVisited) {
            get_post_order_internal(graph, v, colors, output, loops);
        } else if (colors[v] == ProDfsColors::InVisiting) {
            loops.emplace_back(u, v);
        }
    }

    colors[u] = ProDfsColors::Visited;
    output.push_back(u);
}

void print_reverse_post_order(const Graph& graph, const std::string& start) {
    if (not graph.has_vertex(start)) {
        std::cout << "Unknown node " << start << "\n";
        return;
    }

    ColorsMap colors;
    std::vector<std::string> output;
    LoopsVec loops;
    get_post_order_internal(graph, start, colors, output, loops);

    for (const auto& [u, v] : loops) {
        std::cout << "Found loop " << u << "->" << v << "\n";
    }
    for (i64 i = static_cast<i64>(output.size()) - 1; i >= 0; i--) {
        std::cout << output[i] << " ";
    }
    std::cout << "\n";
}

void solve() {
    TaskRepl task_repl;
    task_repl.register_cmd("RPO_NUMBERING", [](const Graph& g) { print_reverse_post_order(g, input<std::string>()); });

    task_repl.start();
}

int main() {
    solve();
}