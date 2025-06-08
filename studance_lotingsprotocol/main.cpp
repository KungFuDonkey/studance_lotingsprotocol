#include "CliArgs.h"
#include "Studancer.h"
#include "DanceClass.h"
#include "MinCostMaxFlow.h"
#include "Assignment.h"
#include "Statistics.h"
#include "Export.h"

// -----------------------------------------------
int main(int argc, char* argv[])
{
    // Get cli arguments
    CliArguments cliArgs = InitializeCliArgs(argc, argv);

    // Check if we should display help
    if (!ParseSuccess(cliArgs) || cliArgs.displayHelp)
    {
        DisplayHelp(cliArgs);
        return ParseSuccess(cliArgs) ? 0 : -1;
    }

    // Load classes
    std::vector<DanceClass> classes = LoadClasses();

    // Load all dancers
    std::vector<Studancer> dancers = LoadDancers(classes);

    // Encode the mincost maxflow problem
    MinCostMaxFlowArgs mcmf = EncodeMinCostMaxFlow(dancers, classes);

    // Solve min cost max flow
    auto result = MinCostMaxFlow(mcmf, cliArgs);

    // Retrieve solution from min cost max flow
    Assignment assignment = DecodeMinCostMaxFlow(mcmf);

    // Print statistics to the terminal
    PrintAssignmentStats(assignment);

    // Export solution
    ExportAssignment(assignment);

    // Wait for input to exit
    system("pause");

    return 0;
}
