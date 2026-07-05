//
// Created by micodiy on 7/6/26.
//

#ifndef GRAPH_TASKS_GRAPH_ACTION_RESULTS_HPP
#define GRAPH_TASKS_GRAPH_ACTION_RESULTS_HPP

#include "types.hpp"

enum class GraphEdgeActionResult : u64 {
    Success = 1,
    SrcUnknown = 2,
    DestUnknown = 4,
    SrcAndDestUnknown = SrcUnknown | DestUnknown,
    EdgeUnknown = 8,
    EdgeAlreadyAdded = 16
};

enum class GraphVertexActionResult : u64 {
    Success = 1,
    UnknownVertex = 2
};

#endif //GRAPH_TASKS_GRAPH_ACTION_RESULTS_HPP
