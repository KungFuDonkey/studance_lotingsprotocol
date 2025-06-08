#pragma once
#include <vector>
#include <string>
#include <map>
#include "DanceClass.h"

enum DancerPriorityGroup
{
    Board,
    Damn,
    ExistingMember,
    NonDancerLastYear,
    UnrolledLastYear,
    NonFemale,
    Female,
    HalfYear,
    GapYear,
    HalfGapYear,
    NonStudying,
    HalfNonStudying,
    Count
};

std::string DancerPriorityGroupToString(DancerPriorityGroup group);

// Describes a single person, and all attributes required for the lottery of assigning the person
// within a dance group in Studance
struct Studancer
{
    DancerPriorityGroup priorityGroup;
    std::vector<std::string> advisedClasses;
    std::vector<std::string> chosenClasses;
    int relationNumber;
    std::string tableRow;
};

std::string GetDancersInputHeader();

std::map<std::string, int> GetDancersInputHeaderMap();

std::vector<Studancer> LoadDancers(const std::vector<DanceClass>& classes);
