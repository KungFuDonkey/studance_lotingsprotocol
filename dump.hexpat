u32 sourceNode @0x0;
u32 sinkNode @0x4;
u32 numNodes @0x8;
u32 bufferDwords @0xC;

u32 numNodes2 = numNodes * numNodes;

u32 distancesOffset = 0x10;
u32 distances[numNodes] @ distancesOffset;

u32 parentOffset = distancesOffset + numNodes * 4;
u32 parents[numNodes] @ parentOffset;

u32 flowOffset = parentOffset + numNodes * 4;
u32 flow[numNodes2] @ flowOffset;

u32 costOffset = flowOffset + numNodes2 * 4;
u32 costs[numNodes2] @ costOffset;

u32 capacityOffset = costOffset + numNodes2 * 4;
u32 capacities[numNodes2] @ capacityOffset;

