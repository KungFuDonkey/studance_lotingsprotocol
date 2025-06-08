#include "DanceClass.h"
#include "Utils.h"
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

std::vector<DanceClass> LoadClasses(bool isTest)
{
    std::vector<DanceClass> classes;

    // possible file names for the dansers file
    std::vector<std::string> danceClassFileNames = {
        "danceclasses.csv"
    };

    // find file
    fs::path danceClassFilePath;
    FindInputFile(danceClassFileNames, danceClassFilePath);

    const int numHeaders = 9;

    std::map<std::string, int> headerMap;
    headerMap.insert({
        {"naam", -1},
        {"maximale ruimte", -1},
        {"minimale ruimte", -1},
        {"extra speel ruimte", -1},
    });

    // open the dance class file
    std::ifstream danceClassFile(danceClassFilePath);

    // Get the header
    std::string line;
    std::getline(danceClassFile, line);

    // Index the header
    int offset = 0;
    int index = 0;
    while (offset < line.length())
    {
        // Get lower version of header
        std::string currentHeader = ParseTillNextComma(line, offset);
        tolower(currentHeader);
        currentHeader.erase(currentHeader.find_last_not_of(" \n\r\t:?") + 1);

        if (headerMap.count(currentHeader))
        {
            headerMap[currentHeader] = index;
        }
        index++;
    }

    std::vector<std::string> failedHeaders;
    for (auto& kv : headerMap)
    {
        if (kv.second == -1)
        {
            failedHeaders.push_back(kv.first);
        }
    }

    if (failedHeaders.size() > 0)
    {
        printf("Failed to find headers in dance class file:\n");
        for (auto& header : failedHeaders)
        {
            printf("    %s\n", header.c_str());
        }
        exit(-1);
    }

    // Create indices array
    const int numIndices = index;
    std::vector<std::string> indices;
    for (int i = 0; i < numIndices; i++)
    {
        indices.emplace_back("");
    }

    // Parse the classes
    while (std::getline(danceClassFile, line))
    {
        DanceClass danceClass = {};

        offset = 0;
        for (int i = 0; i < numIndices; i++)
        {
            indices[i] = ParseTillNextComma(line, offset);
        }

        std::string name = indices[headerMap["naam"]];
        trim(name);
        tolower(name);
        danceClass.name = name;

        std::string maxSize = indices[headerMap["maximale ruimte"]];
        trim(maxSize);
        tolower(maxSize);
        danceClass.maxSize = stoi(maxSize);

        std::string minSize = indices[headerMap["minimale ruimte"]];
        trim(minSize);
        tolower(minSize);
        danceClass.minSize = stoi(minSize);

        std::string additionalSpace = indices[headerMap["extra speel ruimte"]];
        trim(additionalSpace);
        tolower(additionalSpace);
        danceClass.additionalSpace = stoi(additionalSpace);

        classes.push_back(danceClass);
    }

    // close the file
    danceClassFile.close();

    return classes;
}
