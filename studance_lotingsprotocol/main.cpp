#include "CliArgs.h"
#include "Studancer.h"
#include "DanceClass.h"
#include "MinCostMaxFlow.h"
#include "Lottery.h"
#include "Assignment.h"
#include "Statistics.h"
#include "Export.h"

// Runs Lottery algorithm
void RunLottery(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes, const CliArguments& cliArgs)
{
    printf("*******************************************************************************\n");
    printf("=================== Running Lottery algorithm for assignment ==================\n");
    printf("*******************************************************************************\n\n");

    // Create assignment
    Assignment assignment = Lottery(dancers, classes);

    // Print statistics to the terminal
    PrintAssignmentStats(assignment);

    // Export solution
    ExportAssignment(assignment, "ClassAssignment_Lottery", cliArgs);

    printf("*******************************************************************************\n");
    printf("================== Finished Lottery algorithm for assignment ==================\n");
    printf("*******************************************************************************\n\n");
}

// Runs Min Cost Max Flow algorithm
void RunMCMF(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes, const CliArguments& cliArgs)
{
    printf("*******************************************************************************\n");
    printf("==================== Running MCMF algorithm for assignment ====================\n");
    printf("*******************************************************************************\n\n");

    // Encode the mincost maxflow problem
    MinCostMaxFlowArgs mcmf = EncodeMinCostMaxFlow(dancers, classes);

    // Solve min cost max flow
    auto result = MinCostMaxFlow(mcmf, cliArgs);

    // Retrieve solution from min cost max flow
    Assignment assignment = DecodeMinCostMaxFlow(mcmf);

    // Dump the decision log
    DumpDecisionLog(mcmf);

    // Print statistics to the terminal
    PrintAssignmentStats(assignment);

    // Export solution
    ExportAssignment(assignment, "ClassAssignment_MCMF", cliArgs);

    printf("*******************************************************************************\n");
    printf("=================== Finished MCMF algorithm for assignment ====================\n");
    printf("*******************************************************************************\n\n");
}



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

    if (cliArgs.lottery)
    {
        RunLottery(dancers, classes, cliArgs);
    }

    if (cliArgs.mcmf)
    {
        // Extra spacing for when both programs run
        if (cliArgs.lottery)
        {
            printf("\n\n\n\n");
        }

        RunMCMF(dancers, classes, cliArgs);
    }

    // Wait for input to exit
    system("pause");

    return 0;
}