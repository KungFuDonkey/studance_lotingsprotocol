#include "CliArgs.h"
#include <string>


CliArguments InitializeCliArgs(int argc, char* argv[])
{
    std::vector<std::string> args;
    // first one is the application
    for (int i = 1; i < argc; i++)
    {
        args.emplace_back(argv[i]);
    }

    CliArguments cliArgs;
    cliArgs.asText = false;
    cliArgs.displayHelp = false;
    cliArgs.lottery = false;
    cliArgs.mcmf = false;
    cliArgs.unknownArgs = std::vector<std::string>();
    cliArgs.parseFailures = std::vector<std::string>();

    // Return default if there are no args
    if (argc == 0)
    {
        return cliArgs;
    }

    for (auto& arg : args)
    {
        if (arg == "--help" || arg == "-h")
        {
            cliArgs.displayHelp = true;
        }
        else if (arg == "--mcmf" || arg == "-m")
        {
            cliArgs.mcmf = true;
        }
        else if (arg == "--lottery" || arg == "-l")
        {
            cliArgs.lottery = true;
        }
        else if (arg == "--txt" || arg == "-t")
        {
            cliArgs.asText = true;
        }
    }

    // auto enable mcmf
    if (cliArgs.mcmf == cliArgs.lottery && cliArgs.mcmf == false)
    {
        cliArgs.mcmf = true;
        cliArgs.lottery = false;
    }

    return cliArgs;
}

bool ParseSuccess(CliArguments& cliArgs)
{
    return cliArgs.unknownArgs.size() == 0 && cliArgs.parseFailures.size() == 0;
}

void DisplayHelp(CliArguments& cliArgs)
{
    // if help was not requested, but this is called then the user did something wrong
    // So display the unknown args
    if (cliArgs.unknownArgs.size() > 0 || cliArgs.parseFailures.size() > 0)
    {
        printf("There was an issue parsing the input arguments:\n");
        for (auto& arg : cliArgs.unknownArgs)
        {
            printf("  Unknown Argument: %s\n", arg.c_str());
        }

        for (auto& parseFailure : cliArgs.parseFailures)
        {
            printf("  Parse Failure: %s\n", parseFailure.c_str());
        }
    }

    printf("Usage: studance_lotingsprotocol.exe [-h|--help] [-t|--txt] [-m|--mcmf|-l|--lottery]\n");
    printf("Command explanation:\n");
    printf("  [-h|--help]    : Display this help dialog\n");
    printf("  [-t|--txt]     : Ouptut as text file instead of csv\n");
    printf("  [-m|--mcmf]    : Display this help dialog\n");
    printf("  [-l|--lottery] : Use test data instead of the input data\n");
}
