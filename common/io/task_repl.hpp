//
// Created by micodiy on 7/6/26.
//

#ifndef GRAPH_TASKS_TASK_REPL_HPP
#define GRAPH_TASKS_TASK_REPL_HPP

#include <iostream>
#include <string>

#include "io.hpp"
#include "repl.hpp"
#include "structures/graph.hpp"
#include "types.hpp"

class TaskRepl : public Repl {
    static void print_edge_error_if_exists(const std::string& src,
                                           const std::string& dest,
                                           const GraphEdgeActionResult result) {
        if (result == GraphEdgeActionResult::SrcAndDestUnknown) {
            std::cout << "Unknown nodes " << src << " " << dest << "\n";
        } else if (result == GraphEdgeActionResult::SrcUnknown) {
            std::cout << "Unknown node " << src << "\n";
        } else if (result == GraphEdgeActionResult::DestUnknown) {
            std::cout << "Unknown node " << dest << "\n";
        } else if (result == GraphEdgeActionResult::EdgeAlreadyAdded) {
            std::cout << "Edge already was added " << src << " " << dest << "\n";
        }
    }

  public:
    TaskRepl() {
        register_cmd("NODE", [](Graph& g) {
            const auto node_name = input<std::string>();
            g.add_vertex(node_name);
        });

        register_cmd("EDGE", [](Graph& g) {
            const auto src = input<std::string>();
            const auto dest = input<std::string>();
            const auto weight = input<i64>();

            const auto result = g.add_edge(src, dest, weight);
            print_edge_error_if_exists(src, dest, result);
        });

        register_cmd("REMOVE", [](Graph& g) {
            const auto obj_to_delete = input<std::string>();
            if (obj_to_delete == "NODE") {
                const auto node_name = input<std::string>();
                const auto result = g.remove_vertex(node_name);

                if (result == GraphVertexActionResult::UnknownVertex) {
                    std::cout << "Unknown node " << node_name << "\n";
                }
            } else if (obj_to_delete == "EDGE") {
                const auto src = input<std::string>();
                const auto dest = input<std::string>();
                const auto result = g.remove_edge(src, dest);

                print_edge_error_if_exists(src, dest, result);
            }
        });
    }
};

#endif // GRAPH_TASKS_TASK_REPL_HPP