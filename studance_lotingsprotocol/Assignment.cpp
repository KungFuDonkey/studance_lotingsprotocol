#include "Assignment.h"
#include "Utils.h"
#include <fstream>

void PrintAssignmentStats(const Assignment& assignment)
{
    printf("Assignment Statistics:\n");
    // count of number of first, second and third choices
    int buckets[DancerPriorityGroup::Count][3] = {};

    int unenrolledMembers = 0;

    for (auto& classAssignment : assignment)
    {
        if (classAssignment.first.name == "uitgeloot")
        {
            unenrolledMembers = (int)classAssignment.second.size();
        }
        else
        {
            for (auto& dancer : classAssignment.second)
            {
                for (int i = 0; i < 3 && i < dancer.chosenClasses.size(); i++)
                {
                    if (dancer.chosenClasses[i] == classAssignment.first.name)
                    {
                        buckets[(int)dancer.priorityGroup][i]++;
                    }
                }
            }
        }
    }

    const int longestClassName = (int)strlen("NonDancerLastYear");

    for (int i = 0; i < (int)DancerPriorityGroup::Count - 1; i++)
    {
        DancerPriorityGroup group = (DancerPriorityGroup)i;
        std::string priorityGroupName = DancerPriorityGroupToString(group);
        printf("%s", priorityGroupName.c_str());

        for (int space = 0; space < longestClassName - priorityGroupName.length(); space++)
        {
            printf(" ");
        }

        printf(": ");

        int total = buckets[i][0] + buckets[i][1] + buckets[i][2];
        if (total == 0)
        {
            printf("|   0.00%% ||   0.00%% ||   0.00%% |\n");
        }
        else
        {
            float percentages[3] = {
                ((float)buckets[i][0] / (float)total) * 100.f,
                ((float)buckets[i][1] / (float)total) * 100.f,
                ((float)buckets[i][2] / (float)total) * 100.f
            };

            for (float p : percentages)
            {
                int spacing = p == 100.0f ? 1 : p >= 10.0f ? 2 : 3;
                printf("|");
                for (int space = 0; space < spacing; space++)
                {
                    printf(" ");
                }
                printf("%.2f%%", p);

                printf(" |");
            }
            printf("\n");
        }
    }
}

void ExportAsTxt(const Assignment& assignment)
{
    std::ofstream outputFile(GetOutputFolder() / "out.txt");

    for (auto& classAssignment : assignment)
    {
        outputFile << classAssignment.first.name << ":\n";
        for (auto& dancer : classAssignment.second)
        {
            outputFile << dancer.tableRow << "\n";
        }
        outputFile << "\n\n\n\n";
    }

    outputFile.close();
}

void ExportAsCsv(const Assignment& assignment)
{
    // We need to inject a comma for the dance class
    std::string header = GetDancersInputHeader();

    int offset = 0;
    ParseTillNextComma(header, offset);

    std::string preHeader = header.substr(0, offset);
    std::string postHeader = header.substr(offset, header.length());

    int commas = 1;
    while (offset < header.length())
    {
        ParseTillNextComma(header, offset);
        commas += 1;
    }

    std::string emptyRow = "";
    for (int i = 0; i < commas; i++)
    {
        emptyRow += ",";
    }

    std::string preTotal = ",Totaal:,";
    std::string postTotal = "";
    for (int i = 0; i < commas - 2; i++)
    {
        postTotal += ",";
    }

    auto outputPath = GetOutputFolder() / "out.csv";
    std::ofstream outputFile(outputPath);

    for (auto& classAssignment : assignment)
    {
        outputFile << preHeader;
        outputFile << classAssignment.first.name << ",";
        outputFile << postHeader << "\n";

        for (auto& dancer : classAssignment.second)
        {
            offset = 0;
            ParseTillNextComma(dancer.tableRow, offset);

            std::string preDancer = dancer.tableRow.substr(0, offset);
            std::string postDancer = dancer.tableRow.substr(offset, dancer.tableRow.length());

            // We need to inject a comma for the dance class
            outputFile << preDancer << "," << postDancer << "\n";
        }

        outputFile << preTotal;
        outputFile << classAssignment.second.size();
        outputFile << postTotal << "\n";

        outputFile << emptyRow << "\n";
        outputFile << emptyRow << "\n";
        outputFile << emptyRow << "\n";
        outputFile << emptyRow << "\n";
    }

    printf("Exported to: %ls\n", outputPath.c_str());
}

void ExportAssignment(const Assignment& assignment)
{
    PrintAssignmentStats(assignment);
    ExportAsCsv(assignment);
}
