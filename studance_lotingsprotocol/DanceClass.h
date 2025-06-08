#pragma once
#include <string>
#include <vector>
#include "Studancer.h"

struct DanceClass {
    std::string name;
    int maxSize;
    int minSize;
    int additionalSpace;
};

std::vector<DanceClass> LoadClasses(bool isTest);