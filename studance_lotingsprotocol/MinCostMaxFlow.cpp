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

// inline functions for bidirectional accesses
inline int distance(MinCostMaxFlowArgs& args, int u)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in distance");
        DumpBuffer(args);
        exit(-1);
    }
    return args.distance[u];
}
inline void setdistance(MinCostMaxFlowArgs& args, int u, int value)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in setdistance");
        DumpBuffer(args);
        exit(-1);
    }
    args.distance[u] = value;
}
inline int parent(MinCostMaxFlowArgs& args, int u)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in parent");
        DumpBuffer(args);
        exit(-1);
    }
    return args.parent[u];
}
inline void setparent(MinCostMaxFlowArgs& args, int u, int value)
{
    if (u >= args.numNodes)
    {
        printf("Out of range exception in parent");
        DumpBuffer(args);
        exit(-1);
    }
    args.parent[u] = value;
}
inline int flow(MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in flow");
        DumpBuffer(args);
        exit(-1);
    }
    return args.flow[u * args.numNodes + v];
}
inline void setflow(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in setflow");
        DumpBuffer(args);
        exit(-1);
    }
    args.flow[u * args.numNodes + v] = value;
}
inline void addflow(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in addflow");
        DumpBuffer(args);
        exit(-1);
    }
    args.flow[u * args.numNodes + v] += value;
}
inline int cost(MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in cost");
        DumpBuffer(args);
        exit(-1);
    }
    return args.cost[u * args.numNodes + v];
}
inline void setcost(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in setcost");
        DumpBuffer(args);
        exit(-1);
    }
    args.cost[u * args.numNodes + v] = value;
}
inline int capacity(MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in capacity");
        DumpBuffer(args);
        exit(-1);
    }
    return args.capacity[u * args.numNodes + v];
}
inline void setcapacity(MinCostMaxFlowArgs& args, int u, int v, int value)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in setcapacity");
        DumpBuffer(args);
        exit(-1);
    }
    args.capacity[u * args.numNodes + v] = value;
}
inline int canflow(MinCostMaxFlowArgs& args, int u, int v)
{
    if (u >= args.numNodes || v >= args.numNodes)
    {
        printf("Out of range exception in canflow");
        DumpBuffer(args);
        exit(-1);
    }
    return flow(args, u, v) < capacity(args, u, v);
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
    setdistance(args, args.sourceNode, 0);

    bool hadUpdate = false;

    // at most n iterations
    for (int bfIteration = 0; bfIteration < args.numNodes; bfIteration++)
    {
        // Go through all the nodes
        for (int currentNode = 0; currentNode < args.numNodes; currentNode++)
        {
            // Skip node if we don't know how to reach it yet
            const int currentDistance = distance(args, currentNode);
            if (currentDistance == INF)
            {
                continue;
            }

            // Go through all the edges of this node
            for (int neighbour : args.adjecencyList[currentNode])
            {
                // See if we can relax flow if we can go there
                if (canflow(args, currentNode, neighbour))
                {
                    // if the distance is smaller update the distances and the parent
                    const int newDistance = currentDistance + cost(args, currentNode, neighbour);
                    if (newDistance < distance(args, neighbour))
                    {
                        setdistance(args, neighbour, newDistance);
                        setparent(args, neighbour, currentNode);
                        hadUpdate = true;
                    }
                }

                // See if we can relax flow if we can go there via the residual graph
                // (it is then flowing to the current node and we can cancel the flow)
                if (flow(args, neighbour, currentNode) > 0)
                {
                    const int newDistance = currentDistance - cost(args, currentNode, neighbour);
                    if (newDistance < distance(args, neighbour))
                    {
                        setdistance(args, neighbour, newDistance);
                        setparent(args, neighbour, currentNode);
                        hadUpdate = true;
                    }
                }
            }
        }

        if (!hadUpdate)
        {
            // we found the optimal solution so quit, also do not need to check for cycles
            return std::make_pair(distance(args, args.sinkNode), args.sinkNode);
        }
    }

    // Extra iteration to check for negative cycles

    // Go through all the nodes
    for (int currentNode = 0; currentNode < args.numNodes; currentNode++)
    {
        // Skip node if we don't know how to reach it yet
        const int currentDistance = distance(args, currentNode);
        if (currentDistance == INF)
        {
            continue;
        }

        // Go through all the edges of this node
        for (int neighbour : args.adjecencyList[currentNode])
        {
            // find cicles in the normal and residual graph
            bool foundCycle = false;
            if (canflow(args, currentNode, neighbour))
            {
                const int newDistance = currentDistance + cost(args, currentNode, neighbour);
                if (newDistance < distance(args, neighbour))
                {
                    foundCycle = true;
                }
            }

            if (flow(args, neighbour, currentNode) > 0)
            {
                const int newDistance = currentDistance - cost(args, currentNode, neighbour);
                if (newDistance < distance(args, neighbour))
                {
                    foundCycle = true;
                }
            }

            if (foundCycle)
            {
                // We found a negative cycle, now just find the cycle
                std::vector<int> seenNodes;
                seenNodes.push_back(currentNode);
                int currentParent = parent(args, currentNode);

                while (!contains(seenNodes, currentParent))
                {
                    seenNodes.push_back(currentParent);
                    currentNode = currentParent;
                    currentParent = parent(args, currentNode);
                }

                // Negative infinitiy for negative cycle
                return std::make_pair(-INF, currentNode);
            }
        }

    }

    // Return sink node on success
    return std::make_pair(distance(args, args.sinkNode), args.sinkNode);
}

std::pair<int, int> MinCostMaxFlow(MinCostMaxFlowArgs& args, const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes, const CliArguments& cliArgs) {

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
                int p = parent(args, currentNode);

                // Update flow in both the normal and residual graph
                if (capacity(args, p, currentNode) > 0)
                {
                    // normal graph
                    addflow(args, p, currentNode, 1);
                    minCost += cost(args, p, currentNode);
                }
                else
                {
                    // residual graph
                    addflow(args, currentNode, p, -1);
                    minCost -= cost(args, p, currentNode);
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
                        incomming += flow(args, neighbour, node);
                        outgoing += flow(args, node, neighbour);

                        if (flow(args, neighbour, node) < 0)
                        {
                            printf("\nFailed flow conservation: Negative incomming in node %i\n", node);
                            DumpBuffer(args);
                            exit(-1);
                        }

                        if (flow(args, node, neighbour) < 0)
                        {
                            printf("\nFailed flow conservation: Negative outgoing in node %i\n", node);
                            DumpBuffer(args);
                            exit(-1);
                        }
                    }

                    if (incomming != outgoing)
                    {
                        printf("\nFailed flow conservation in node %i after updating path:\n", node);

                        currentNode = args.sinkNode;
                        while (currentNode != args.sourceNode)
                        {
                            int p = parent(args, currentNode);
                            printf("%i ", currentNode);
                            currentNode = p;
                        }
                        printf("%i\n", currentNode);


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
            int currentParent = parent(args, currentNode);

            while (!contains(seenNodes, currentParent))
            {
                seenNodes.push_back(currentParent);
                currentNode = currentParent;
                currentParent = parent(args, currentNode);
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
    setcapacity(args, u, v, cap);
    setcost(args, u, v, c);

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
    numNodes += (int)classes.size() + 2;
    int chosenUnenrollmentNode = numNodes - 2;
    int unenrollmentNode = numNodes - 1;

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
    int dancerOffset = 1;
    int classOffset = dancerOffset + (int)dancers.size();
    int classCostsOffset = classOffset + (int)classes.size() + 2;
    int sinkNode = (int)numNodes - 1;
    args.expectedMaxFlow = 0;

    // Encode dancers (edges of sink to dancers, and dancers to classes)
    for (int i = 0; i < dancers.size(); i++)
    {
        // Encode source -> dancer
        const Studancer& dancer = dancers[i];

        // node for this dancer
        int dancerNodeIndex = dancerOffset + i;

        if (dancer.priorityGroup == NonDancingMember)
        {
            // no cost for non dancing members
            MakeEdge(args, 0, dancerNodeIndex, 0, 1);
            MakeEdge(args, dancerNodeIndex, chosenUnenrollmentNode, 0, 1);
            args.expectedMaxFlow += 1;
        }
        else
        {
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
                if (dancer.chosenClasses[j] == "" || dancer.chosenClasses[j] == "maak een keuze")
                {
                    continue;
                }
                const std::string& chosenClass = dancer.chosenClasses[j];
                bool wasAdvised = contains(dancer.advisedClasses, chosenClass);

                // 3 -> 7 -> 11
                int classCost = j + (j + 1) * choiceCost;
                classCost -= wasAdvised ? additionalAdviceCost : 0;

                int classNodeIndex = classMap[chosenClass] + classOffset;

                // can only choose class once
                MakeEdge(args, dancerNodeIndex, classNodeIndex, classCost, 1);
            }

            MakeEdge(args, dancerNodeIndex, unenrollmentNode, unenrolledCost, 1);
        }
    }

    // Encode classes (classes to different choice costs)
    for (int i = 0; i < classes.size(); i++)
    {
        const DanceClass& danceClass = classes[i];

        int classNodeIndex = classMap[danceClass.name] + classOffset;

        int classNodeCostIndex = i * 3 + classCostsOffset;

        MakeEdge(args, classNodeIndex, classNodeCostIndex    , underMinBoundsCost     , danceClass.minSize);
        MakeEdge(args, classNodeIndex, classNodeCostIndex + 1, isWithinClassBoundsCost, danceClass.maxSize - danceClass.minSize);
        MakeEdge(args, classNodeIndex, classNodeCostIndex + 2, useAdditionalSpaceCost , danceClass.additionalSpace);

        // We set cost and capacity to 0 as that was already calculated in the last edge
        MakeEdge(args, classNodeCostIndex, sinkNode, 0, INF);
        MakeEdge(args, classNodeCostIndex + 1, sinkNode, 0, INF);
        MakeEdge(args, classNodeCostIndex + 2, sinkNode, 0, INF);
    }

    MakeEdge(args, chosenUnenrollmentNode, args.sinkNode, 0, INF);
    MakeEdge(args, unenrollmentNode, args.sinkNode, 0, INF);

    return args;
}

Assignment DecodeMinCostMaxFlow(MinCostMaxFlowArgs& args, const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes)
{
    Assignment assignment;

    int dancerOffset = 1;
    int classOffset = dancerOffset + (int)dancers.size();
    int classCostsOffset = classOffset + (int)classes.size() + 2;

    for (int i = 0; i < classes.size(); i++)
    {
        DanceClass danceClass = classes[i];
        std::vector<Studancer> assignedDancers;

        int classNodeIndex = i + classOffset;

        for (int neighbour : args.adjecencyList[classNodeIndex])
        {
            // If there is flow from a dancer to this class, this class was chosen
            if (neighbour >= dancerOffset && neighbour < classOffset && !canflow(args, neighbour, classNodeIndex))
            {
                int dancerIndex = neighbour - dancerOffset;
                assignedDancers.push_back(dancers[dancerIndex]);
            }
        }

        assignment.push_back(std::make_pair(danceClass, assignedDancers));
    }

    int chosenUnenrollmentNode = classCostsOffset - 2;
    DanceClass nonDancingMembersClass = {};
    nonDancingMembersClass.name = "niet-dansende leden";
    std::vector<Studancer> nonDancingMembers;

    for (int neighbour : args.adjecencyList[chosenUnenrollmentNode])
    {
        // If there is flow from a dancer to this class, this class was chosen
        if (neighbour >= dancerOffset && neighbour < classOffset && !canflow(args, neighbour, chosenUnenrollmentNode))
        {
            int dancerIndex = neighbour - dancerOffset;
            nonDancingMembers.push_back(dancers[dancerIndex]);
        }
    }

    assignment.push_back(std::make_pair(nonDancingMembersClass, nonDancingMembers));

    int unenrolledNode = classCostsOffset - 1;
    DanceClass unenrolledClass = {};
    unenrolledClass.name = "uitgeloot";
    std::vector<Studancer> unenrolledMembers;

    for (int neighbour : args.adjecencyList[unenrolledNode])
    {
        // If there is flow from a dancer to this class, this class was chosen
        if (neighbour >= dancerOffset && neighbour < classOffset && !canflow(args, neighbour, unenrolledNode))
        {
            int dancerIndex = neighbour - dancerOffset;
            unenrolledMembers.push_back(dancers[dancerIndex]);
        }
    }

    assignment.push_back(std::make_pair(unenrolledClass, unenrolledMembers));

    return assignment;
}

