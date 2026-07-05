#include <iostream>
#include <string>
#include <unordered_map>

#include "../common/io/task_repl.hpp"
#include "../common/structures/graph.hpp"
#include "../common/io/io.hpp"
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

void solve() {
    TaskRepl task_repl;
    task_repl.register_cmd("RPO_NUMBERING", [](Graph &g) {
        print_reverse_post_order(g, input<std::string>());
    });

    task_repl.start();
}

int main() {
    solve();
}
