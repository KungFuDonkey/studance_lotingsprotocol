#include "Export.h"
#include "Utils.h"
#include <fstream>

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
    ExportAsCsv(assignment);
}
