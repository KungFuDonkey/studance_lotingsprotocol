#pragma once
#include <vector>
#include <string>

struct CliArguments
{
    bool asText;
    bool displayHelp;
    bool mcmf;
    bool lottery;
    bool isUpdate;
    int maxUnenroll;
    std::vector<std::string> unknownArgs;
    std::vector<std::string> parseFailures;
};

CliArguments InitializeCliArgs(int argc, char* argv[]);

bool ParseSuccess(CliArguments& cliArgs);

void DisplayHelp(CliArguments& cliArgs);