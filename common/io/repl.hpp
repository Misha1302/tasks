//
// Created by micodiy on 7/6/26.
//

#ifndef GRAPH_TASKS_REPL_HPP
#define GRAPH_TASKS_REPL_HPP

#include <functional>
#include <iostream>
#include <map>
#include <string>

#include "../structures/graph.hpp"

class Repl {
    std::map<std::string, std::function<void(Graph& g)>> commands;

  public:
    void register_cmd(const std::string& cmd, const std::function<void(Graph& g)>& func) {
        commands[cmd] = func;
    }

    void start() {
        Graph g;

        std::string cmd;
        while (std::cin >> cmd) {
            if (!commands.contains(cmd)) {
                std::cout << "Unknown command '" << cmd << "'\n";
                continue;
            }

            commands[cmd](g);
        }
    }
};

#endif // GRAPH_TASKS_REPL_HPP