#include "MinCostMaxFlow.h"
#include "Utils.h"
#include <algorithm>
#include <cstring>
#include <queue>
#include <utility>
#include <map>
#include <fstream>
#include <filesystem>

#define MAXN 1000

// not 7FFFFFFF to prevent an overflow
#define INF 0x3FFFFFFF

inline void DumpBuffer(MinCostMaxFlowArgs& args)
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

inline NodeType GetNodeType(MinCostMaxFlowArgs& args, int node)
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

inline std::string GetNodeName(MinCostMaxFlowArgs& args, int node)
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

inline const Studancer& GetDancerFromNode(MinCostMaxFlowArgs& args, int node)
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
inline int GetDistance(MinCostMaxFlowArgs& args, int u)
{
    if (u >= args.numNodes)
    {
        printf("ERROR: Out of range exception in GetDistance");
        DumpBuffer(args);
        exit(-1);
    }
    return args.distance[u];
}
inline void SetDistance(MinCostMaxFlowArgs& args, int u, int value)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in SetDistance");
        DumpBuffer(args);
        exit(-1);
    }
    args.distance[u] = value;
}
inline int GetParent(MinCostMaxFlowArgs& args, int u)
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
inline int GetFlow(MinCostMaxFlowArgs& args, int u, int v)
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
inline int GetCost(MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in GetCost");
        DumpBuffer(args);
        exit(-1);
    }
    return args.cost[u * args.numNodes + v];
}
inline void SetCost(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in SetCost");
        DumpBuffer(args);
        exit(-1);
    }
    args.cost[u * args.numNodes + v] = value;
}
inline int GetCapacity(MinCostMaxFlowArgs& args, int u, int v)
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
inline int CanFlow(MinCostMaxFlowArgs& args, int u, int v)
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

std::pair<int, int> BellmanFord(MinCostMaxFlowArgs& args)
{
    // Initialize infinite distances
    InitArray(args.distance, INF, args.numNodes);
    InitArray(args.parent, -1, args.numNodes);

    // set distance to source node to 0
    SetDistance(args, args.sourceNode, 0);

    bool hadUpdate = false;

    // at most n iterations
    for (int bfIteration = 0; bfIteration < args.numNodes; bfIteration++)
    {
        // Go through all the nodes
        for (int currentNode = 0; currentNode < args.numNodes; currentNode++)
        {
            // Skip node if we don't know how to reach it yet
            const int currentDistance = GetDistance(args, currentNode);
            if (currentDistance == INF)
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
                    const int newDistance = currentDistance + GetCost(args, currentNode, neighbour);
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
                    const int newDistance = currentDistance - GetCost(args, currentNode, neighbour);
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
        const int currentDistance = GetDistance(args, currentNode);
        if (currentDistance == INF)
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
                const int newDistance = currentDistance + GetCost(args, currentNode, neighbour);
                if (newDistance < GetDistance(args, neighbour))
                {
                    foundCycle = true;
                }
            }

            if (GetFlow(args, neighbour, currentNode) > 0)
            {
                const int newDistance = currentDistance - GetCost(args, currentNode, neighbour);
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
                return std::make_pair(-INF, currentNode);
            }
        }

    }

    // Return sink node on success
    return std::make_pair(GetDistance(args, args.sinkNode), args.sinkNode);
}

std::pair<int, int> MinCostMaxFlow(MinCostMaxFlowArgs& args, const CliArguments& cliArgs) {

    int minCost = 0, maxFlow = 0;

    // first stores distance, second stores node
    std::pair<int, int> bfOutput = BellmanFord(args);

    printf("Assigned:\n");
    while (bfOutput.first < INF) {

        if (bfOutput.first != -INF && bfOutput.second == args.sinkNode)
        {
            // Update flow for path, small optimization here is that we know the max flow over a path is 1
            // This is because you can only CHOOSE a dance class once, and as we always need to go over a choice
            // to get from the source to the sink, the max flow is always 1

            int currentNode = args.sinkNode;
            while (currentNode != args.sourceNode)
            {
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

            if (cliArgs.isTest)
            {
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

                        int prevNode = 0;

                        while (currentNode != args.sourceNode)
                        {
                            int p = GetParent(args, currentNode);

                            printf("%i (type: %s, name: %s)\n", currentNode, currentNodeTypeName.c_str(), currentNodeName.c_str());
                            prevNode = currentNode;
                            currentNode = p;
                            currentNodeType = GetNodeType(args, currentNode);
                            currentNodeTypeName = NodeTypeToString(currentNodeType);
                            currentNodeName = GetNodeName(args, currentNode);
                        }
                        printf("%i (type: %s, name: %s)\n", currentNode, currentNodeTypeName.c_str(), currentNodeName.c_str());

                        printf("incomming: %i, outgoing: %i", incomming, outgoing);

                        DumpBuffer(args);
                        exit(-1);
                    }
                }
            }


            maxFlow++;
            float percentageAssigned = ((float)maxFlow / (float)args.expectedMaxFlow) * 100.f;
            printf("\r%.2f%%", percentageAssigned);
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

        bfOutput = BellmanFord(args);
    }

    printf("\n");

    return std::make_pair(minCost, maxFlow);
}

MinCostMaxFlowArgs AllocateMinCostMaxFlow(int numNodes)
{
    // space computation
    int spaceRequired = 0;

    // distances space
    int distancesOffset = spaceRequired;
    spaceRequired += numNodes;

    // parent space
    int parentOffset = spaceRequired;
    spaceRequired += numNodes;

    // flow space
    int flowOffset = spaceRequired;
    spaceRequired += numNodes * numNodes;

    // cost space
    int costOffset = spaceRequired;
    spaceRequired += numNodes * numNodes;

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
    args.cost = &buffer[costOffset];
    args.capacity = &buffer[capacityOffset];
    args.distance = &buffer[distancesOffset];
    args.parent = &buffer[parentOffset];
    args.flow = &buffer[flowOffset];
    args.bufferDwords = spaceRequired;
    args.buffer = buffer;

    return args;
}

// This probably needs tuning
const int choiceCost = 3;
const int additionalAdviceCost = -choiceCost;

const int choiceOffset = 12;

const int isBoardOrDamnCost = 0;
const int isExistingMemberCost = isBoardOrDamnCost + choiceOffset;
const int wasNonDancingMemberCost = isExistingMemberCost + choiceOffset;
const int wasUnrolledLastYearCost = wasNonDancingMemberCost + choiceOffset;
const int isNonFemaleCost = wasUnrolledLastYearCost + choiceOffset;
const int isFemaleCost = isNonFemaleCost + choiceOffset;
const int isHalfYearCost = isFemaleCost + choiceOffset;
const int isGapYearCost = isHalfYearCost + choiceOffset;
const int isHalfGapYearCost = isGapYearCost + choiceOffset;
const int isNonStudyingCost = isHalfGapYearCost + choiceOffset;
const int isHalfNonStudyingCost = isNonStudyingCost + choiceOffset;

const int underMinBoundsCost = 0;
const int isWithinClassBoundsCost = 1;
const int useAdditionalSpaceCost = isHalfYearCost;

int GetUnenrollmentCostForDancer(const Studancer& dancer)
{
    return 10000;
}

int GetCostForDancer(const Studancer& dancer)
{
    int priorityGroup = (int)dancer.priorityGroup;

    return priorityGroup * choiceOffset;
}

void MakeEdge(MinCostMaxFlowArgs& args, int u, int v, int c, int cap)
{
    // forward
    args.adjecencyList[u].push_back(v);
    SetCapacity(args, u, v, cap);
    SetCost(args, u, v, c);

    // residual
    args.adjecencyList[v].push_back(u);
}

MinCostMaxFlowArgs EncodeMinCostMaxFlow(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes)
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
        int dancerCost = GetCostForDancer(dancer);
        int unenrolledCost = GetUnenrollmentCostForDancer(dancer);
        // Board members can assign 2 classes
        int numDanceClassesToChoose = dancer.priorityGroup == Board ? 2 : 1;
        args.expectedMaxFlow += numDanceClassesToChoose;

        MakeEdge(args, 0, dancerNodeIndex, dancerCost, numDanceClassesToChoose);

        // encode choices
        for (int j = 0; j < dancer.chosenClasses.size(); j++)
        {
            if (dancer.chosenClasses[j] == "")
            {
                continue;
            }
            const std::string& chosenClass = dancer.chosenClasses[j];
            int classNodeIndex = classMap[chosenClass] + args.classOffset;

            if (chosenClass == "unenrolled")
            {
                // special cost for unenrollmlent
                MakeEdge(args, dancerNodeIndex, classNodeIndex, unenrolledCost, 1);
            }
            else
            {
                bool wasAdvised = contains(dancer.advisedClasses, chosenClass);

                // 3 -> 7 -> 11
                int classCost = j + (j + 1) * choiceCost;
                classCost -= wasAdvised ? additionalAdviceCost : 0;

                // can only choose class once
                MakeEdge(args, dancerNodeIndex, classNodeIndex, classCost, 1);
            }

        }
    }

    // Encode classes (classes to different choice costs)
    for (int i = 0; i < classes.size(); i++)
    {
        const DanceClass& danceClass = classes[i];

        int classNodeIndex = classMap[danceClass.name] + args.classOffset;

        int classNodeCostIndex = i * 3 + args.classCostOffset;

        MakeEdge(args, classNodeIndex, classNodeCostIndex    , underMinBoundsCost     , danceClass.minSize);
        MakeEdge(args, classNodeIndex, classNodeCostIndex + 1, isWithinClassBoundsCost, danceClass.maxSize - danceClass.minSize);
        MakeEdge(args, classNodeIndex, classNodeCostIndex + 2, useAdditionalSpaceCost , danceClass.additionalSpace);

        // We set cost and capacity to 0 as that was already calculated in the last edge
        MakeEdge(args, classNodeCostIndex, args.sinkNode, 0, INF);
        MakeEdge(args, classNodeCostIndex + 1, args.sinkNode, 0, INF);
        MakeEdge(args, classNodeCostIndex + 2, args.sinkNode, 0, INF);
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
            }
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

