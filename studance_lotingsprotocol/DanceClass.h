#pragma once
#include <string>
#include <vector>

struct DanceClass {
    std::string name;
    int maxSize;
    int minSize;
    int additionalSpace;
};

std::vector<DanceClass> LoadClasses();