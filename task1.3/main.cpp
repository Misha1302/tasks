#include <algorithm>
#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <ranges>

#include "../common/io/task_repl.hpp"
#include "../common/structures/edge.hpp"
#include "../common/structures/graph.hpp"
#include "../common/io/io.hpp"
#include "../common/types.hpp"

class Dinic {
    struct ResidualArc {
        Edge *edge;
        bool reversed;

        [[nodiscard]] const std::string &to() const {
            return not reversed ? edge->get_dest_vertex_id() : edge->get_src_vertex_id();
        }

        [[nodiscard]] i64 get_capacity(std::unordered_map<Edge *, std::pair<i64, i64> > &res) const {
            return not reversed ? res[edge].first : res[edge].second;
        }

        void push(std::unordered_map<Edge *, std::pair<i64, i64> > &res, i64 x) const {
            if (not reversed)
                res[edge].first -= x, res[edge].second += x;
            else res[edge].first += x, res[edge].second -= x;
        }
    };

    static std::vector<ResidualArc> make_arcs_from(const std::string &u, const Graph &graph) {
        std::vector<ResidualArc> arcs;
        for (const auto e: graph.get_vertex(u).get_edges_out())
            arcs.push_back({e, false});
        for (const auto e: graph.get_vertex(u).get_edges_in())
            arcs.push_back({e, true});
        return arcs;
    }

    static std::unordered_map<std::string, i64> group_vertices_to_levels_bfs(
        const Graph &graph,
        const std::string &start,
        std::unordered_map<Edge *, std::pair<i64, i64> > &edge_forward_and_reversed_weights
    ) {
        std::unordered_map<std::string, i64> levels;

        std::queue<std::string> queue;
        queue.push(start);
        levels[start] = 0;

        while (not queue.empty()) {
            auto u = queue.front();
            queue.pop();

            for (const auto e: make_arcs_from(u, graph)) {
                const auto &v = e.to();
                if (not levels.contains(v) and e.get_capacity(edge_forward_and_reversed_weights) > 0) {
                    queue.push(v);
                    levels[v] = levels[u] + 1;
                }
            }
        }

        return levels;
    }

    static i64 try_dfs(
        const std::string &u, const std::string &end,
        std::unordered_map<std::string, i64> &levels,
        const Graph &graph,
        std::unordered_map<Edge *, std::pair<i64, i64> > &edge_forward_and_reversed_weights,
        std::unordered_map<std::string, i64> &next_arc_index,
        i64 pushed
    ) {
        if (u == end) {
            return pushed;
        }

        const auto arcs = make_arcs_from(u, graph);
        while (next_arc_index[u] < static_cast<i64>(arcs.size())) {
            const auto &e = arcs[next_arc_index[u]];
            const auto &v = e.to();

            i64 c = e.get_capacity(edge_forward_and_reversed_weights);
            if (c <= 0 or not levels.contains(v) or not levels.contains(u) or levels[v] - levels[u] != 1) {
                next_arc_index[u]++;
                continue;
            }

            const auto sent = try_dfs(
                v, end, levels, graph,
                edge_forward_and_reversed_weights,
                next_arc_index,
                std::min(pushed, c)
            );

            if (sent > 0) {
                e.push(edge_forward_and_reversed_weights, sent);
                return sent;
            }

            next_arc_index[u]++;
        }

        return 0;
    }

    static i64 get_blocking_flow_cost(
        const Graph &graph,
        std::unordered_map<std::string, i64> &levels,
        std::unordered_map<Edge *, std::pair<i64, i64> > &edge_forward_and_reversed_weights,
        const std::string &start,
        const std::string &end
    ) {
        i64 sum_max_flow = 0;
        std::unordered_map<std::string, i64> next_arc_index;

        while (true) {
            const i64 value = try_dfs(
                start, end, levels, graph, edge_forward_and_reversed_weights,
                next_arc_index, std::numeric_limits<i64>::max()
            );
            if (value == 0) return sum_max_flow;
            sum_max_flow += value;
        }
    }

public:
    static i64 max_flow(const Graph &graph, const std::string &start, const std::string &end) {
        if (not graph.has_vertex(start) or not graph.has_vertex(end)) return -1;
        if (start == end) return 0;

        std::unordered_map<Edge *, std::pair<i64, i64> > edge_forward_and_reversed_weights;

        for (const auto &val: graph.get_readonly_edges() | std::views::values) {
            edge_forward_and_reversed_weights[val.get()] = {val->get_weight(), static_cast<i64>(0)};
        }

        i64 sum_flow = 0;
        while (true) {
            auto levels = group_vertices_to_levels_bfs(graph, start, edge_forward_and_reversed_weights);
            if (not levels.contains(end)) break;

            sum_flow += get_blocking_flow_cost(
                graph, levels, edge_forward_and_reversed_weights, start, end
            );
        }

        return sum_flow;
    }
};


void solve() {
    TaskRepl task_repl;
    task_repl.register_cmd(
        "MAXFLOW",
        [](Graph &g) {
            const auto start = input<std::string>();
            const auto end = input<std::string>();
            const auto value = Dinic::max_flow(g, start, end);
            std::cout << value << "\n";
        }
    );
    task_repl.start();
}

int main() {
    solve();
}
