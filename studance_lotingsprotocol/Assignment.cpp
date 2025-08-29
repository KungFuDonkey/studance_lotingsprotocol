#include "Assignment.h"
#include <algorithm>

bool CompareDancers(const Studancer& dancer1, const Studancer& dancer2)
{
	if (dancer1.priorityGroup == dancer2.priorityGroup)
	{
		//return dancer1.relationNumber < dancer2.relationNumber;
		// Random order should be kept only order based on prio group
		return dancer1.index < dancer2.index;
	}

	return dancer1.priorityGroup < dancer2.priorityGroup;
}

void ResortAssignment(Assignment& assignment)
{
	for (auto& classAssignment : assignment)
	{
		if (classAssignment.second.size() > 0)
		{
			std::sort(classAssignment.second.begin(), classAssignment.second.end(), CompareDancers);
		}
	}
}