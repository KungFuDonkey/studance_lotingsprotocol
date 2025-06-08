#pragma once
#include <vector>
#include <string>

struct CliArguments
{
    bool isTest;
    bool displayHelp;
    std::vector<std::string> unknownArgs;
    std::vector<std::string> parseFailures;
};

CliArguments InitializeCliArgs(int argc, char* argv[]);

bool ParseSuccess(CliArguments& cliArgs);

void DisplayHelp(CliArguments& cliArgs);