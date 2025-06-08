#pragma once
#include <vector>
#include "Studancer.h"
#include "DanceClass.h"
#include "Assignment.h"
#include "CliArgs.h"

struct MinCostMaxFlowArgs
{
    int sourceNode;                 // source and sink node indices
    int sinkNode;

    int dancerOffset;               // Offsets for the different types of nodes
    int classOffset;
    int classCostOffset;

    int numNodes;                   // number of nodes
    std::vector<int>* adjecencyList; // Array of vectors containing the adjecency list, size = numNodes
    int* cost;                       // Array of costs, size = numNodes * numNodes
    int* capacity;                   // Array of capacities, size = numNodes * numNodes

    // Shortest path calculation data
    int* distance;                   // distances for each node, size = numNodes
    int* parent;                     // for reconstructing the path, size = numNodes

    // mcmf
    int* flow; // final flow, size = numNodes * numNodes
    int expectedMaxFlow;

    // total allocated space
    int* buffer;
    int bufferDwords;

    // Data of dancers
    const std::vector<Studancer>* dancers;
    const std::vector<DanceClass>* classes;
};

std::pair<int, int> MinCostMaxFlow(MinCostMaxFlowArgs& args, const CliArguments& cliArgs);

MinCostMaxFlowArgs EncodeMinCostMaxFlow(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes);

Assignment DecodeMinCostMaxFlow(MinCostMaxFlowArgs& args);
