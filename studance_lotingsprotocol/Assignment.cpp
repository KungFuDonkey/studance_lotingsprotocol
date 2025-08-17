#include "Assignment.h"
#include <algorithm>

bool CompareDancers(const Studancer& dancer1, const Studancer& dancer2)
{
	if (dancer1.priorityGroup == dancer2.priorityGroup)
	{
		//return dancer1.relationNumber < dancer2.relationNumber;
		// Random order should be kept only order based on prio group
		return true;
	}

	return dancer1.priorityGroup < dancer2.priorityGroup;
}

void ResortAssignment(Assignment& assignment)
{
	for (auto& classAssignment : assignment)
	{
		std::sort(classAssignment.second.begin(), classAssignment.second.end(), CompareDancers);
	}
}