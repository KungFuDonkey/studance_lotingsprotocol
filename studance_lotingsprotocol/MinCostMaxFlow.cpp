#include "MinCostMaxFlow.h"
#include "Utils.h"
#include <algorithm>
#include <cstring>
#include <queue>
#include <utility>
#include <map>
#include <fstream>
#include <filesystem>
#include <chrono>

#define MAXN 1000

// not 7FFFFFFF to prevent an overflow
#define INF 0x3FFFFFFF
#define INF64 0x3FFFFFFFFFFFFFFFLL

inline void DumpBuffer(const MinCostMaxFlowArgs& args)
{
    auto path = GetOutputFolder() / "dump.bin";

    if (fs::exists(path))
    {
        fs::remove(path);
    }

    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::app);

    int offsets[4] = {
        args.sourceNode,
        args.sinkNode,
        args.numNodes,
        args.bufferDwords
    };

    const char* offsetData = (const char*)&offsets[0];
    output.write(offsetData, 4 * sizeof(int));

    const char* data = (const char*) args.buffer;

    output.write(data, args.bufferDwords * sizeof(args.cost[0]));

    output.close();
}

enum NodeType
{
    Source,
    Sink,
    Dancer,
    Class,
    ClassCost,
    Unknown
};

inline std::string NodeTypeToString(NodeType nodeType)
{
    switch (nodeType)
    {
    case Source: return "Source";
    case Sink: return "Sink";
    case Dancer: return "Dancer";
    case Class: return "Class";
    case ClassCost: return "ClassCost";
    case Unknown: return "Unknown";
    default: return "Unkown";
    }
    return "Unkown";
}

inline NodeType GetNodeType(const MinCostMaxFlowArgs& args, int node)
{
    if (node == 0)
    {
        return Source;
    }
    else if (node >= args.dancerOffset && node < args.classOffset)
    {
        return Dancer;
    }
    else if (node >= args.classOffset && node < args.classCostOffset)
    {
        return Class;
    }
    else if (node >= args.classCostOffset && node < args.sinkNode)
    {
        return ClassCost;
    }
    else if (node == args.sinkNode)
    {
        return Sink;
    }
    else
    {
        return Unknown;
    }
}

inline std::string GetNodeName(const MinCostMaxFlowArgs& args, int node)
{
    NodeType nodeType = GetNodeType(args, node);
    if (nodeType == Source)
    {
        return "Source";
    }
    else if (nodeType == Dancer)
    {
        return std::to_string(args.dancers->operator[](node - args.dancerOffset).relationNumber);
    }
    else if (nodeType == Class)
    {
        return args.classes->operator[](node - args.classOffset).name;
    }
    else if (nodeType == ClassCost)
    {
        int classId = (node - args.classCostOffset) / 3;
        int costId = (node - args.classCostOffset) % 3;
        std::string className = args.classes->operator[](classId).name;
        std::string costName = costId == 0 ? "Min Cost" : costId == 1 ? "Standard Cost" : "Additional Cost";
        return className + " " + costName;
    }
    else if (nodeType == Sink)
    {
        return "Sink";
    }
    else
    {
        return "Out of bounds";
    }
}

std::string PathToString(const MinCostMaxFlowArgs& args, const std::vector<int>& path)
{
    std::string output = "";
    for (auto& node : path)
    {
        NodeType nodeType = GetNodeType(args, node);
        std::string nodeTypeName = NodeTypeToString(nodeType);
        std::string nodeName = GetNodeName(args, node);

        output += std::to_string(node);
        output += " (type: ";
        output += nodeTypeName;
        output += ", name: ";
        output += nodeName;
        output += ")\n";
    }
    return output;
}

inline const Studancer& GetDancerFromNode(const MinCostMaxFlowArgs& args, int node)
{
    NodeType nodeType = GetNodeType(args, node);
    if (nodeType != Dancer)
    {
        printf("ERROR: Bad node type for GetDancerFromNode()");
        exit(-1);
    }
    return args.dancers->operator[](node - args.dancerOffset);
}

// inline functions for bidirectional accesses
inline int64_t GetDistance(const MinCostMaxFlowArgs& args, int u)
{
    if (u >= args.numNodes)
    {
        printf("ERROR: Out of range exception in GetDistance");
        DumpBuffer(args);
        exit(-1);
    }
    return args.distance[u];
}
inline void SetDistance(MinCostMaxFlowArgs& args, int u, int64_t value)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in SetDistance");
        DumpBuffer(args);
        exit(-1);
    }
    args.distance[u] = value;
}
inline int GetParent(const MinCostMaxFlowArgs& args, int u)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in GetParent");
        DumpBuffer(args);
        exit(-1);
    }
    return args.parent[u];
}
inline void SetParent(MinCostMaxFlowArgs& args, int u, int value)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in SetParent");
        DumpBuffer(args);
        exit(-1);
    }
    args.parent[u] = value;
}
inline int GetFlow(const MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in GetFlow");
        DumpBuffer(args);
        exit(-1);
    }
    return args.flow[u * args.numNodes + v];
}
inline void SetFlow(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in SetFlow");
        DumpBuffer(args);
        exit(-1);
    }
    args.flow[u * args.numNodes + v] = value;
}
inline void AddFlow(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in AddFlow");
        DumpBuffer(args);
        exit(-1);
    }
    args.flow[u * args.numNodes + v] += value;
}
inline int64_t GetCost(const MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in GetCost");
        DumpBuffer(args);
        exit(-1);
    }
    return args.cost[u * args.numNodes + v];
}
inline void SetCost(MinCostMaxFlowArgs& args, int u, int v, int64_t value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in SetCost");
        DumpBuffer(args);
        exit(-1);
    }
    args.cost[u * args.numNodes + v] = value;
}
inline int GetCapacity(const MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in GetCapacity");
        DumpBuffer(args);
        exit(-1);
    }
    return args.capacity[u * args.numNodes + v];
}
inline void SetCapacity(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in SetCapacity");
        DumpBuffer(args);
        exit(-1);
    }
    args.capacity[u * args.numNodes + v] = value;
}
inline int CanFlow(const MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in CanFlow");
        DumpBuffer(args);
        exit(-1);
    }
    return GetFlow(args, u, v) < GetCapacity(args, u, v);
}
inline void InitArray(int* array, int value, int numElements)
{
    for (int i = 0; i < numElements; i++)
    {
        array[i] = value;
    }
}
inline void InitArray64(int64_t* array, int64_t value, int numElements)
{
    for (int i = 0; i < numElements; i++)
    {
        array[i] = value;
    }
}

std::pair<int64_t, int> BellmanFord(MinCostMaxFlowArgs& args)
{
    // Initialize infinite distances
    InitArray64(args.distance, INF64, args.numNodes);
    InitArray(args.parent, -1, args.numNodes);

    // set distance to source node to 0
    SetDistance(args, args.sourceNode, 0);

    // at most n iterations
    for (int bfIteration = 0; bfIteration < args.numNodes; bfIteration++)
    {
        bool hadUpdate = false;

        // Go through all the nodes
        for (int currentNode = 0; currentNode < args.numNodes; currentNode++)
        {
            // Skip node if we don't know how to reach it yet
            const int64_t currentDistance = GetDistance(args, currentNode);
            if (currentDistance == INF64)
            {
                continue;
            }

            // Go through all the edges of this node
            for (int neighbour : args.adjecencyList[currentNode])
            {
                // See if we can relax flow if we can go there
                if (CanFlow(args, currentNode, neighbour))
                {
                    // if the distance is smaller update the distances and the parent
                    const int64_t newDistance = currentDistance + GetCost(args, currentNode, neighbour);
                    if (newDistance < GetDistance(args, neighbour))
                    {
                        SetDistance(args, neighbour, newDistance);
                        SetParent(args, neighbour, currentNode);
                        hadUpdate = true;
                    }
                }

                // See if we can relax flow if we can go there via the residual graph
                // (it is then flowing to the current node and we can cancel the flow)
                if (GetFlow(args, neighbour, currentNode) > 0)
                {
                    const int64_t newDistance = currentDistance - GetCost(args, neighbour, currentNode);
                    if (newDistance < GetDistance(args, neighbour))
                    {
                        SetDistance(args, neighbour, newDistance);
                        SetParent(args, neighbour, currentNode);
                        hadUpdate = true;
                    }
                }
            }
        }

        if (!hadUpdate)
        {
            // we found the optimal solution so quit, also do not need to check for cycles
            return std::make_pair(GetDistance(args, args.sinkNode), args.sinkNode);
        }
    }

    // Extra iteration to check for negative cycles

    // Go through all the nodes
    for (int currentNode = 0; currentNode < args.numNodes; currentNode++)
    {
        // Skip node if we don't know how to reach it yet
        const int64_t currentDistance = GetDistance(args, currentNode);
        if (currentDistance == INF64)
        {
            continue;
        }

        // Go through all the edges of this node
        for (int neighbour : args.adjecencyList[currentNode])
        {
            // find cicles in the normal and residual graph
            bool foundCycle = false;
            if (CanFlow(args, currentNode, neighbour))
            {
                const int64_t newDistance = currentDistance + GetCost(args, currentNode, neighbour);
                if (newDistance < GetDistance(args, neighbour))
                {
                    foundCycle = true;
                }
            }

            if (GetFlow(args, neighbour, currentNode) > 0)
            {
                const int64_t newDistance = currentDistance - GetCost(args, currentNode, neighbour);
                if (newDistance < GetDistance(args, neighbour))
                {
                    foundCycle = true;
                }
            }

            if (foundCycle)
            {
                // We found a negative cycle, now just find the cycle
                std::vector<int> seenNodes;
                seenNodes.push_back(currentNode);
                int currentParent = GetParent(args, currentNode);

                while (!contains(seenNodes, currentParent))
                {
                    seenNodes.push_back(currentParent);
                    currentNode = currentParent;
                    currentParent = GetParent(args, currentNode);
                }

                // Negative infinitiy for negative cycle
                return std::make_pair(-INF64, currentNode);
            }
        }

    }

    // Return sink node on success
    return std::make_pair(GetDistance(args, args.sinkNode), args.sinkNode);
}

std::pair<int64_t, int> MinCostMaxFlow(MinCostMaxFlowArgs& args, const CliArguments& cliArgs) {

    int64_t minCost = 0;
    int maxFlow = 0;

    // first stores distance, second stores node
    std::pair<int64_t, int> bfOutput = BellmanFord(args);

    std::chrono::system_clock::time_point start = {};

    printf("Assigned:\n");
    while (bfOutput.first < INF64) {

        Decision decision = {};

        if (bfOutput.first != -INF64 && bfOutput.second == args.sinkNode)
        {
            // Update flow for path, small optimization here is that we know the max flow over a path is 1
            // This is because you can only CHOOSE a dance class once, and as we always need to go over a choice
            // to get from the source to the sink, the max flow is always 1
            decision.type = AssignDancer;

            int64_t initialCost = minCost;

            int currentNode = args.sinkNode;
            while (currentNode != args.sourceNode)
            {
                decision.changedNodes.push_back(currentNode);
                int p = GetParent(args, currentNode);

                // Update flow in both the normal and residual graph
                if (GetCapacity(args, p, currentNode) > 0)
                {
                    // normal graph
                    AddFlow(args, p, currentNode, 1);
                    minCost += GetCost(args, p, currentNode);
                }
                else
                {
                    // residual graph
                    AddFlow(args, currentNode, p, -1);
                    minCost -= GetCost(args, p, currentNode);
                }

                currentNode = p;
            }
            decision.changedNodes.push_back(args.sourceNode);

            for (int node = 1; node < args.sinkNode; node++)
            {
                int incomming = 0;
                int outgoing = 0;

                for (int neighbour : args.adjecencyList[node])
                {
                    incomming += GetFlow(args, neighbour, node);
                    outgoing += GetFlow(args, node, neighbour);

                    if (GetFlow(args, neighbour, node) < 0)
                    {
                        NodeType nodeType = GetNodeType(args, node);
                        std::string nodeTypeName = NodeTypeToString(nodeType);
                        std::string nodeName = GetNodeName(args, node);
                        printf("\nFailed flow conservation: Negative incomming for node %i with NodeType %s and Name %s\n", node, nodeTypeName.c_str(), nodeName.c_str());
                        DumpBuffer(args);
                        exit(-1);
                    }

                    if (GetFlow(args, node, neighbour) < 0)
                    {
                        NodeType nodeType = GetNodeType(args, node);
                        std::string nodeTypeName = NodeTypeToString(nodeType);
                        std::string nodeName = GetNodeName(args, node);
                        printf("\nFailed flow conservation: Negative outgoing for node %i with NodeType %s and Name %s\n", node, nodeTypeName.c_str(), nodeName.c_str());
                        DumpBuffer(args);
                        exit(-1);
                    }
                }

                if (incomming != outgoing)
                {
                    NodeType nodeType = GetNodeType(args, node);
                    std::string nodeTypeName = NodeTypeToString(nodeType);
                    std::string nodeName = GetNodeName(args, node);

                    printf("\nFailed flow conservation for node %i with NodeType %s and Name %s after updating path:\n", node, nodeTypeName.c_str(), nodeName.c_str());

                    currentNode = args.sinkNode;
                    NodeType currentNodeType = GetNodeType(args, currentNode);
                    std::string currentNodeTypeName = NodeTypeToString(currentNodeType);
                    std::string currentNodeName = GetNodeName(args, currentNode);

                    // reverse the path
                    std::vector<int> path;
                    for (int i = (int)decision.changedNodes.size() - 1; i > 0; i--)
                    {
                        path.push_back(decision.changedNodes[i]);
                    }
                    std::string pathString = PathToString(args, path);
                    printf("%s\n", pathString.c_str());

                    printf("incomming: %i, outgoing: %i", incomming, outgoing);

                    DumpBuffer(args);
                    exit(-1);
                }
            }

            decision.flowChange += 1;
            decision.costChange += (minCost - initialCost);
            maxFlow++;

            // update terminal every so often
            auto duration = std::chrono::system_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() > 200)
            {
                float percentageAssigned = ((float)maxFlow / (float)args.expectedMaxFlow) * 100.f;
                printf("\r%.2f%%", percentageAssigned);
                start = std::chrono::system_clock::now();
            }
        }
        else
        {
            // TODO negative cycles,
            int currentNode = bfOutput.second;
            std::vector<int> seenNodes;
            seenNodes.push_back(currentNode);
            int currentParent = GetParent(args, currentNode);

            while (!contains(seenNodes, currentParent))
            {
                seenNodes.push_back(currentParent);
                currentNode = currentParent;
                currentParent = GetParent(args, currentNode);
            }

            printf("\n");
            printf("Found Cycle: %i", seenNodes[0]);
            for (int i = 1; i < seenNodes.size(); i++)
            {
                printf(", %i", seenNodes[1]);
            }
            printf("\n");

            DumpBuffer(args);
            exit(-1);
        }

        args.decisions.push_back(decision);

        bfOutput = BellmanFord(args);
    }

    // final update for terminal
    printf("\r%.2f%%", 100.0f);
    // Create spacing for the rest of the program
    printf("\n\n");
    //printf("%llu", minCost);
    //printf("\n\n");

    return std::make_pair(minCost, maxFlow);
}

MinCostMaxFlowArgs AllocateMinCostMaxFlow(int numNodes)
{
    // space computation
    int spaceRequired = 0;

    // distances space
    int distancesOffset = spaceRequired;
    spaceRequired += numNodes * 2; // 64 bit

    // parent space
    int parentOffset = spaceRequired;
    spaceRequired += numNodes;

    // flow space
    int flowOffset = spaceRequired;
    spaceRequired += numNodes * numNodes;

    // cost space
    int costOffset = spaceRequired;
    spaceRequired += numNodes * numNodes * 2; // 64 bit

    // capacity space
    int capacityOffset = spaceRequired;
    spaceRequired += numNodes * numNodes;

    // we have a single array for all data of the network
    int* buffer = new int[spaceRequired];
    InitArray(buffer, 0, spaceRequired);

    MinCostMaxFlowArgs args = {};
    args.sourceNode = 0;
    args.sinkNode = numNodes - 1;
    args.numNodes = numNodes;
    args.adjecencyList = new std::vector<int>[numNodes];
    args.cost = (int64_t*)&buffer[costOffset];
    args.capacity = &buffer[capacityOffset];
    args.distance = (int64_t*)&buffer[distancesOffset];
    args.parent = &buffer[parentOffset];
    args.flow = &buffer[flowOffset];
    args.bufferDwords = spaceRequired;
    args.buffer = buffer;

    return args;
}

const int underMinBoundsCost = 0;
const int isWithinClassBoundsCost = 1;
const int useAdditionalSpaceCost = 2;

int64_t GetChoiceCostForDancer(const Studancer& dancer, const std::string& chosenClass, int choiceNumber)
{
    if (chosenClass == "unenrolled")
    {
        choiceNumber = 3;
    }
    // Layout is:
    //    [0-2]: 1st - 3rd choice
    //    [3]  : unrolled cost
    // If someone decides not to enroll for 3 classes, the choice to unroll them is made earlier
    int64_t boardAndDamnCost[4] = {
        0,
        0,
        300000000,
        600000000
    };

    int64_t hbboardCost[4] = {
        0,
        200000000,
        300000000,
        600000000
    };

    // if four ExistingMembers can go from unenrolled to 1st choice to stop an advised choice it will happen
    // calculation:
    //      (9500 - 3000) + 4 * (9000 - 11000) < 0
    //      (9500 - 3000) + 3 * (9000 - 11000) > 0
    int64_t existingMemberFollowsAdviceCost = 0;
    int64_t existingCost[4] = {
         900000,
         950000,
        1000000,
        1100000
    };


    // NonDancing and Unrolled are basically grouped in the same math equations
    // The NonDancing have slightly higher priority than Unrolled

    // if two NonDancing/Unrolled members can go from 3rd to first choice due to a single ExistingMember
    // if four NonDancing/Unrolled members can go from 2nd to first choice due to a single ExistingMember
    //   we should pick that solution
    int64_t nonDancingLastYearCost[4] = {
        1010000,
        1027500,
        1045000,
        1090000
    };
    int64_t unrolledLastYearCost[4] = {
        1020000,
        1035000,
        1050000,
        1080000
    };

    // Female and NonFemale are basically grouped in the same math equations
    // The NonFemale have slightly higher priority than Unrolled

    //if two NonFemale/Female members can go from 3rd to first choice due to a single NonDancingMembers
    //if four NonFemale/Female members can go from 2nd to first choice due to a single NonDancingMembers
    //    we should pick that solution
    int64_t nonFemaleCost[4] = {
        1010000,
        1027500,
        1045000,
        1090000
    };
    int64_t femaleCost[4] = {
        1020000,
        1035000,
        1050000,
        1080000
    };

    // if two HalfYear members can go from 3rd to first choice due to a single NonFemale
    // if four HalfYear members can go from 2nd to first choice due to a single NonFemale
    //    we should pick that solution
    int64_t halfYearCost[4] = {
        1062100,
        1064050,
        1066000,
        1069900
    };

    // if two GapYear/HalfGapYear members can go from 3rd to first choice due to a single HalfYear
    // if four GapYear/HalfGapYear members can go from 2nd to first choice due to a single HalfYear
    //    we should pick that solution
    int64_t gapYearCost[4] = {
        1066300,
        1067000,
        1067700,
        1069100
    };
    int64_t halfGapYearCost[4] = {
        1066600,
        1067200,
        1067800,
        1069000
    };

    // if two NonStudying/HalfNonStudying members can go from 3rd to first choice due to a single GapYear
    // if four NonStudying/HalfNonStudying can go from 2nd to first choice due to a single GapYear
    //    we should pick that solution
    int64_t nonStudyingCost[4] = {
        1066300,
        1067000,
        1067700,
        1069100
    };
    int64_t halfNonStudyingCost[4] = {
        1066600,
        1067200,
        1067800,
        1069000
    };

#if 1
    int64_t baseCost = 300000;
    int64_t maxCost = 300000000;

    int64_t start[3]
    {
        0,0,0
    };
    int64_t increment[3]
    {
        0,0,0
    };

    increment[0] = 8988;
    start[0] = baseCost + increment[0] * 3 + 1;

    for (int i = 0; i < 2; i++)
    {
        increment[i + 1] = increment[i] / 3 + 1;
        start[i + 1] = start[i] + 2 * increment[i] + 10; // added padding
    }

    int64_t groupCost[3][4] = {};

    for (int i = 0; i < 3; i++)
    {
        groupCost[i][0] = start[i];
        for (int j = 1; j < 3; j++)
        {
            groupCost[i][j] = groupCost[i][j - 1] + increment[i];
        }
    }

    int64_t unenrollBase = groupCost[0][2];
    groupCost[0][3] = unenrollBase * 1.5f;
    groupCost[1][3] = unenrollBase * 1.4f;
    groupCost[2][3] = unenrollBase * 1.3f;

    for (int i = 0; i < 4; i++)
    {
        existingCost[i] = groupCost[0][i];
    }

    for (int i = 0; i < 4; i++)
    {
        nonDancingLastYearCost[i] = groupCost[1][i];
    }

    for (int i = 0; i < 4; i++)
    {
        unrolledLastYearCost[i] = groupCost[1][i] + 1;
    }

    for (int i = 0; i < 4; i++)
    {
        nonFemaleCost[i] = groupCost[1][i] + 2;
    }

    for (int i = 0; i < 4; i++)
    {
        femaleCost[i] = groupCost[1][i] + 3;
    }

    for (int i = 0; i < 4; i++)
    {
        halfYearCost[i] = groupCost[2][i];
    }

    for (int i = 0; i < 4; i++)
    {
        gapYearCost[i] = groupCost[2][i] + 1;
    }

    for (int i = 0; i < 4; i++)
    {
        halfGapYearCost[i] = groupCost[2][i] + 2;
    }

    for (int i = 0; i < 4; i++)
    {
        nonStudyingCost[i] = groupCost[2][i] + 3;
    }

    for (int i = 0; i < 4; i++)
    {
        halfNonStudyingCost[i] = groupCost[2][i] + 4;
    }
#endif

    // flat cost of +1 such that it is better to not unenroll someone when the choice is between
    // moving someone to 2nd or 3rd choice or someone to unenrolled
    int isUnrolledClass = chosenClass == "unenrolled" ? 1 : 0;

    if (choiceNumber > 3)
    {
        choiceNumber = 3;
        printf("Warning: Dancer %i has more choices than allowed", dancer.relationNumber);
    }

    if (dancer.priorityGroup == HBBoard)
    {
        return hbboardCost[choiceNumber] + isUnrolledClass;
    }

    // Smallest cost for board and damn
    if (dancer.priorityGroup == KBBoard || dancer.priorityGroup == Damn)
    {
        return boardAndDamnCost[choiceNumber] + isUnrolledClass;
    }

    // For exising members that follow advice (First class only (choiceNumber == 0)) give next smallest cost
    if ((dancer.priorityGroup == ExistingMember) && choiceNumber == 0 && contains(dancer.advisedClasses, chosenClass))
    {
        return existingMemberFollowsAdviceCost + isUnrolledClass;
    }

    if (dancer.priorityGroup == ExistingMember)
    {
        return existingCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == NonDancerLastYear)
    {
        return nonDancingLastYearCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == UnrolledLastYear)
    {
        return unrolledLastYearCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == NonFemale)
    {
        return nonFemaleCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == Female)
    {
        return femaleCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == HalfYear)
    {
        return halfYearCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == GapYear)
    {
        return gapYearCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == HalfGapYear)
    {
        return halfGapYearCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == NonStudying)
    {
        return nonStudyingCost[choiceNumber] + isUnrolledClass;
    }

    if (dancer.priorityGroup == HalfNonStudying)
    {
        return halfNonStudyingCost[choiceNumber] + isUnrolledClass;
    }

    printf("Warning: Dancer %i was not categorized correctly setting half non studying cost", dancer.relationNumber);

    return halfNonStudyingCost[choiceNumber] + isUnrolledClass;
}

const int choiceOffset = 12;
int64_t GetCostForDancer(const Studancer& dancer)
{
    int priorityGroup = (int)dancer.priorityGroup;
    return priorityGroup * choiceOffset;
}

void MakeEdge(MinCostMaxFlowArgs& args, int u, int v, int64_t c, int cap)
{
    // forward
    args.adjecencyList[u].push_back(v);
    SetCapacity(args, u, v, cap);
    SetCost(args, u, v, c);

    // residual
    args.adjecencyList[v].push_back(u);
}

MinCostMaxFlowArgs EncodeMinCostMaxFlow(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes, const CliArguments& cliArgs)
{
    // 1 for source
    int numNodes = 1;

    // first layer, all dancers
    numNodes += (int)dancers.size();

    // second layer, all classes + 1 chosen unenrollment class + 1 unenrolled class
    numNodes += (int)classes.size();

    // third layer, classes no cost, classes bit of cost, classes extra space cost
    numNodes += (int)classes.size();
    numNodes += (int)classes.size();
    numNodes += (int)classes.size();

    // sink node
    numNodes += 1;

    // Initialize network
    MinCostMaxFlowArgs args = AllocateMinCostMaxFlow(numNodes);

    // for easier finding of classes
    std::map<std::string, int> classMap;
    for (int i = 0; i < classes.size(); i++)
    {
        classMap.emplace(std::make_pair(classes[i].name, i));
    }

    // 0 is sourceNode
    args.sourceNode = 0;
    args.dancerOffset = 1;
    args.classOffset = args.dancerOffset + (int)dancers.size();
    args.classCostOffset = args.classOffset + (int)classes.size();
    args.sinkNode = (int)numNodes - 1;
    args.expectedMaxFlow = 0;
    args.dancers = &dancers;
    args.classes = &classes;

    // Encode dancers (edges of sink to dancers, and dancers to classes)
    for (int i = 0; i < dancers.size(); i++)
    {
        // Encode source -> dancer
        const Studancer& dancer = dancers[i];

        // node for this dancer
        int dancerNodeIndex = args.dancerOffset + i;

        // Different types of dancers have different types of cost
        int64_t dancerCost = GetCostForDancer(dancer);
        // Board members can assign 2 classes
        int numDanceClassesToChoose = dancer.priorityGroup == KBBoard || dancer.priorityGroup == Damn ? 2 : 1;
        args.expectedMaxFlow += numDanceClassesToChoose;

        MakeEdge(args, 0, dancerNodeIndex, dancerCost, numDanceClassesToChoose);

        int choiceNumber = 0;
        // encode choices
        for (int j = 0; j < dancer.chosenClasses.size(); j++)
        {
            if (dancer.chosenClasses[j] == "")
            {
                continue;
            }
            const std::string& chosenClass = dancer.chosenClasses[j];
            int classNodeIndex = classMap[chosenClass] + args.classOffset;

            // Note: Also handles unenrolled
            int64_t classCost = GetChoiceCostForDancer(dancer, chosenClass, choiceNumber);

            // can only choose class once
            MakeEdge(args, dancerNodeIndex, classNodeIndex, classCost, 1);

            choiceNumber++;
        }
    }

    // Encode classes (classes to different choice costs)
    for (int i = 0; i < classes.size(); i++)
    {
        const DanceClass& danceClass = classes[i];

        int classNodeIndex = classMap[danceClass.name] + args.classOffset;

        int classNodeCostIndex = i * 3 + args.classCostOffset;

        if (danceClass.name == "niet-dansend lid" || danceClass.name == "unenrolled")
        {
            if (danceClass.name == "unenrolled" && cliArgs.maxUnenroll != 0xFFFFFFFFU)
            {
                MakeEdge(args, classNodeIndex, classNodeCostIndex, underMinBoundsCost, cliArgs.maxUnenroll);
            }
            else
            {
                MakeEdge(args, classNodeIndex, classNodeCostIndex, underMinBoundsCost, danceClass.maxSize);
            }

            MakeEdge(args, classNodeCostIndex, args.sinkNode, 0, INF);
        }
        else
        {
            MakeEdge(args, classNodeIndex, classNodeCostIndex, underMinBoundsCost, danceClass.minSize);
            MakeEdge(args, classNodeIndex, classNodeCostIndex + 1, isWithinClassBoundsCost, danceClass.maxSize - danceClass.minSize);
            MakeEdge(args, classNodeIndex, classNodeCostIndex + 2, useAdditionalSpaceCost, danceClass.additionalSpace);

            // We set cost and capacity to 0 as that was already calculated in the last edge
            MakeEdge(args, classNodeCostIndex, args.sinkNode, 0, INF);
            MakeEdge(args, classNodeCostIndex + 1, args.sinkNode, 0, INF);
            MakeEdge(args, classNodeCostIndex + 2, args.sinkNode, 0, INF);
        }
    }

    // Check the encoding
    // Source node only links to dansers
    for (int i = 0; i < args.numNodes; i++)
    {
        NodeType type = GetNodeType(args, i);

        if (type == Dancer)
        {
            if (GetCapacity(args, args.sourceNode, i) == 0)
            {
                printf("ERROR: source node not connected to dancer %s\n", GetNodeName(args, i).c_str());
                exit(-1);
            }
        }
        else
        {
            if (GetCapacity(args, args.sourceNode, i) != 0)
            {
                printf("ERROR: source node was connected to a non dancer node named %s\n", GetNodeName(args, i).c_str());
                exit(-1);
            }
        }
    }

    // Dancer nodes only have link to chosen classes
    for (int i = 0; i < dancers.size(); i++)
    {
        int dancerIndex = args.dancerOffset + i;
        const Studancer& dancer = GetDancerFromNode(args, dancerIndex);
        bool foundUnenrolled = false;
        // Check all connections
        for (int j = 0; j < args.numNodes; j++)
        {

            if (GetCapacity(args, dancerIndex, j) > 0)
            {
                // if there is a link it MUST be a class and in the chosen list of the dancer
                NodeType nodeType = GetNodeType(args, j);
                std::string nodeName = GetNodeName(args, j).c_str();
                if (nodeType != Class)
                {
                    printf("ERROR: dancer node %s was connected to a non class node named %s\n", GetNodeName(args, i).c_str(), nodeName.c_str());
                    exit(-1);
                }
                if (!contains(dancer.chosenClasses, nodeName))
                {
                    printf("ERROR: dancer %s did not choose node %s but it has been connected\n", GetNodeName(args, i).c_str(), nodeName.c_str());
                    exit(-1);
                }
                foundUnenrolled = foundUnenrolled || (nodeName == "unenrolled");
            }
        }
        if (!foundUnenrolled)
        {
            printf("ERROR: dancer node %s was not connected to the unenrolled class\n", GetNodeName(args, i).c_str());
            exit(-1);
        }

        // Check chosen connections specifically
        for (auto& chosenClass : dancer.chosenClasses)
        {
            if (chosenClass == "")
            {
                continue;
            }
            int classIndex = args.classOffset + classMap[chosenClass];
            std::string className = GetNodeName(args, classIndex).c_str();

            if (GetCapacity(args, dancerIndex, classIndex) == 0)
            {
                printf("ERROR: dancer %s was not connected to class %s while it was chosen by the dancer\n", GetNodeName(args, i).c_str(), className.c_str());
                exit(-1);
            }
        }
    }

    // Class nodes only have connections to class cost nodes
    for (int i = 0; i < classes.size(); i++)
    {
        int classIndex = args.classOffset + i;

        for (int j = 0; j < args.numNodes; j++)
        {
            if (GetCapacity(args, classIndex, j) > 0)
            {
                // if there is a link it MUST be a class cost node of this class
                NodeType nodeType = GetNodeType(args, j);
                std::string nodeName = GetNodeName(args, j).c_str();
                if (nodeType != ClassCost)
                {
                    printf("ERROR: class node %s was connected to a non class cost node named %s\n", GetNodeName(args, i).c_str(), nodeName.c_str());
                    exit(-1);
                }

                // Check if the connection was made to the correct node
                int classCostIndex = j - args.classCostOffset;
                int checkClassIndex = classCostIndex / 3;
                if (i != checkClassIndex)
                {
                    printf("ERROR: class node %s was connected to a class cost node named %s but the index does not match\n", GetNodeName(args, i).c_str(), nodeName.c_str());
                    exit(-1);
                }
            }
        }
    }

    // Class cost nodes only have connections to the sink node
    for (int i = 0; i < classes.size() * 3; i++)
    {
        int classCostIndex = args.classCostOffset + i;

        for (int j = 0; j < args.numNodes; j++)
        {
            if (GetCapacity(args, classCostIndex, j) > 0)
            {
                // if there is a link it MUST be the sink node
                NodeType nodeType = GetNodeType(args, j);
                std::string nodeName = GetNodeName(args, j).c_str();
                if (nodeType != Sink)
                {
                    printf("ERROR: class cost node %s was connected to a node named %s\n", GetNodeName(args, i).c_str(), nodeName.c_str());
                    exit(-1);
                }
            }
        }
    }

    return args;
}

Assignment DecodeMinCostMaxFlow(MinCostMaxFlowArgs& args)
{
    Assignment assignment;

    const std::vector<Studancer>& dancers = *args.dancers;
    const std::vector<DanceClass>& classes = *args.classes;

    int dancerOffset = 1;
    int classOffset = dancerOffset + (int)dancers.size();
    int classCostsOffset = classOffset + (int)classes.size();

    for (int i = 0; i < classes.size(); i++)
    {
        DanceClass danceClass = classes[i];
        std::vector<Studancer> assignedDancers;

        int classNodeIndex = i + classOffset;

        for (int neighbour : args.adjecencyList[classNodeIndex])
        {
            // If there is flow from a dancer to this class, this class was chosen
            if (neighbour >= dancerOffset && neighbour < classOffset && !CanFlow(args, neighbour, classNodeIndex))
            {
                int dancerIndex = neighbour - dancerOffset;
                assignedDancers.push_back(dancers[dancerIndex]);
            }
        }

        assignment.push_back(std::make_pair(danceClass, assignedDancers));
    }

    return assignment;
}

void DumpDecisionLog(const MinCostMaxFlowArgs& args)
{
    auto outputPath = GetOutputFolder() / "DecisionLog_MCMF.txt";
    std::ofstream outputFile(outputPath);

    int decisionNumber = 1;
    for (auto& decision : args.decisions)
    {
        outputFile << "Decision ";
        outputFile << decisionNumber;
        outputFile << " (flow change: ";
        outputFile << decision.flowChange;
        outputFile << ", cost change: ";
        outputFile << decision.costChange;
        outputFile << ")\n";

        // reverse the path
        std::vector<int> path;
        for (int i = (int)decision.changedNodes.size() - 1; i > 0; i--)
        {
            path.push_back(decision.changedNodes[i]);
        }

        if (decision.type == AssignDancer)
        {
            const Studancer& dancer = GetDancerFromNode(args, path[1]);
            std::string className = GetNodeName(args, path[2]);

            outputFile << "Assigned dancer ";
            outputFile << dancer.relationNumber;
            outputFile << " with priority group ";
            outputFile << DancerPriorityGroupToString(dancer.priorityGroup);
            outputFile << " to ";
            outputFile << className;

            // if the path is longer than 4 someone else was moved
            if (path.size() > 4)
            {
                int offset = 3;

                outputFile << " by updating the assignment:\n";

                for (int index = 2; index < path.size() - 2; index++)
                {
                    int currentNode = path[index];
                    int nextNode = path[index + 1];

                    NodeType currentNodeType = GetNodeType(args, currentNode);
                    NodeType nextNodeType = GetNodeType(args, nextNode);

                    if (currentNodeType == Dancer && nextNodeType == Class)
                    {
                        const Studancer& updatedDancer = GetDancerFromNode(args, currentNode);
                        std::string updatedClass = GetNodeName(args, nextNode);
                        outputFile << "Assigned dancer ";
                        outputFile << updatedDancer.relationNumber;
                        outputFile << " with priority group ";
                        outputFile << DancerPriorityGroupToString(updatedDancer.priorityGroup);
                        outputFile << " to ";
                        outputFile << updatedClass;
                        outputFile << "\n";
                    }
                    else if (currentNodeType == Class && nextNodeType == Dancer)
                    {
                        const Studancer& updatedDancer = GetDancerFromNode(args, nextNode);
                        std::string updatedClass = GetNodeName(args, currentNode);
                        outputFile << "Unassigned dancer ";
                        outputFile << updatedDancer.relationNumber;
                        outputFile << " with priority group ";
                        outputFile << DancerPriorityGroupToString(updatedDancer.priorityGroup);
                        outputFile << " from ";
                        outputFile << updatedClass;
                        outputFile << "\n";
                    }
                    else if (currentNodeType == Dancer && nextNodeType == Source)
                    {
                        const Studancer& updatedDancer = GetDancerFromNode(args, currentNode);
                        outputFile << "Dancer ";
                        outputFile << updatedDancer.relationNumber;
                        outputFile << " with priority group ";
                        outputFile << DancerPriorityGroupToString(updatedDancer.priorityGroup);
                        outputFile << " was completely unassigned";
                        outputFile << "\n";
                    }
                }

                outputFile << "\n";
            }
            else
            {
                outputFile << "\n\n";
            }
        }
        else if (decision.type == CycleCancel)
        {
            std::string pathString = PathToString(args, path);

            outputFile << "Cancelled cycle over path:\n";
            outputFile << pathString;
            outputFile << "\n\n";
        }

        decisionNumber++;
    }

    outputFile.close();
}

