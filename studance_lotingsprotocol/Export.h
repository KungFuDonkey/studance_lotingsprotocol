#pragma once
#include "Assignment.h"
#include "MinCostMaxFlow.h"

void ExportAssignment(const Assignment& assignment, const std::string& outputName, const CliArguments& cliArgs);

Assignment LoadExportAssignment(const std::string& fileName, const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes);
