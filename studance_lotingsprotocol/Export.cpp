#include "Export.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <map>

void ExportAssignmentAsTxt(const Assignment& assignment, const std::string& outputName)
{
    std::string outputFileName = outputName + ".txt";
    auto outputPath = GetOutputFolder() / outputFileName;
    std::ofstream outputFile(outputPath);

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

    printf("Exported to: %ls\n\n", outputPath.c_str());
}

void ExportAssignmentAsCsv(const Assignment& assignment, const std::string& outputName)
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

    std::string outputFileName = outputName + ".csv";
    auto outputPath = GetOutputFolder() / outputFileName;
    std::ofstream outputFile(outputPath);

    for (auto& classAssignment : assignment)
    {
        std::string name = classAssignment.first.name;
        name[0] = toupper(name[0]);
        outputFile << preHeader;
        outputFile << name << ",";
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

    outputFile.close();

    printf("Exported to: %ls\n\n", outputPath.c_str());
}

void ExportAssignment(const Assignment& assignment, const std::string& outputName, const CliArguments& cliArgs)
{
    if (cliArgs.asText)
    {
        ExportAssignmentAsTxt(assignment, outputName);
    }
    else
    {
        ExportAssignmentAsCsv(assignment, outputName);
    }
}

Assignment LoadExportAssignment(const std::string& fileName, const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes)
{
    Assignment result;

    std::string line;

    std::vector<std::string> fileNames = {
        fileName
    };

    std::map<int, Studancer> dancerMap;
    std::map<std::string, int> classMap;

    for (auto& dancer : dancers)
    {
        dancerMap.emplace(std::make_pair(dancer.relationNumber, dancer));
    }

    int classIndex = 0;
    for (auto& danceClass : classes)
    {
        result.push_back(std::make_pair(danceClass, std::vector<Studancer>()));
        classMap.emplace(std::make_pair(danceClass.name, classIndex));
        classIndex++;
    }

    fs::path assignmentPath;
    FindOutputFile(fileNames, assignmentPath);

    std::ifstream assignmentCsv(assignmentPath);

    while (std::getline(assignmentCsv, line))
    {
        int offset = 0;
        std::string currentInput = ParseTillNextComma(line, offset);

        tolower(currentInput);
        trim(currentInput);

        bool isHeaderLine = currentInput == "relatienummer";

        if (isHeaderLine)
        {
            std::string className = ParseTillNextComma(line, offset);
            int index = classMap[className];

            while (std::getline(assignmentCsv, line))
            {
                int tmpOffset = 0;
                std::string relationNumberString = ParseTillNextComma(line, tmpOffset);
                if (relationNumberString == "")
                {
                    break;
                }
                int relationNumber = std::stoi(relationNumberString);

                Studancer dancer = dancerMap[relationNumber];

                result[index].second.push_back(dancer);
            }
        }
    }

    return result;
}
