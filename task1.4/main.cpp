#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../common/io/io.hpp"
#include "../common/io/task_repl.hpp"
#include "../common/structures/edge.hpp"
#include "../common/structures/graph.hpp"
#include "../common/types.hpp"

class Tarjan {
    static void pro_dfs(const Graph& graph,
                        const std::string& u,
                        std::unordered_map<std::string, i64>& tin,
                        std::unordered_map<std::string, i64>& tmin,
                        std::vector<std::string>& cur_component_stack,
                        std::unordered_set<std::string>& cur_component_set,
                        std::vector<std::vector<std::string>>& components,
                        i64& t) {
        cur_component_stack.push_back(u);
        cur_component_set.insert(u);

        tin[u] = t++;
        tmin[u] = tin[u];

        for (auto& e : graph.get_vertex(u).get_edges_out()) {
            const auto& v = e->get_dest_vertex_id();
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
                if (u == k) {
                    break;
                }
            }
        }
    }

  public:
    static std::vector<std::vector<std::string>> get_components(const Graph& graph, const std::string& u) {
        std::unordered_map<std::string, i64> tin, tmin;
        std::vector<std::string> cur_component_stack;
        std::unordered_set<std::string> cur_component_set;
        std::vector<std::vector<std::string>> components;
        i64 t = 0;

        pro_dfs(graph, u, tin, tmin, cur_component_stack, cur_component_set, components, t);

        return components;
    }
};

void solve() {
    TaskRepl task_repl;
    task_repl.register_cmd("TARJAN", [](const Graph& g) {
        const auto u = input<std::string>();
        if (not g.has_vertex(u)) {
            std::cout << "Unknown node " << u << "\n";
            return;
        }

        const auto value = Tarjan::get_components(g, u);
        for (const auto& vec : value) {
            if (vec.size() <= 1) {
                continue;
            }

            for (const auto& v : vec) {
                std::cout << v << " ";
            }
            std::cout << "\n";
        }
    });
    task_repl.start();
}

int main() {
    solve();
}
