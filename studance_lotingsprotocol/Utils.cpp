#include "Utils.h"

// trim from start (in place)
void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
}

// trim from end (in place)
void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// trim from both ends (in place)
void trim(std::string& s) {
    rtrim(s);
    ltrim(s);
}

// transform string to lowercase
void tolower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
}

// Finds file according to a list
void FindInputFile(std::vector<std::string> fileNames, std::filesystem::path& filePath)
{
    bool found = false;
    fs::path inputDir = GetInputFolder();
    for (auto& fileName : fileNames)
    {
        auto path = inputDir / fileName.c_str();
        if (fs::exists(path))
        {
            filePath = path;
            found = true;
            break;
        }
    }

    if (!found)
    {
        printf("Failed to find %s file, searches included:\n", fileNames[0].c_str());
        for (auto& fileName : fileNames)
        {
            fs::path path = inputDir / fileName.c_str();
            printf("    %ls\n", path.c_str());
        }
        exit(-1);
    }
}

fs::path FindFolderInParentDirs(const std::string& folder)
{
    fs::path currentPath = fs::current_path();

    while (!fs::exists(currentPath / folder) && currentPath != currentPath.parent_path())
    {
        currentPath = currentPath.parent_path();
    }

    if (currentPath == currentPath.parent_path())
    {
        printf("Could not find %s folder, please make a folder called %s at: %ls", folder.c_str(), folder.c_str(), fs::current_path().c_str());
        exit(-1);
    }

    return currentPath / folder;
}


fs::path inputFolder = "";
fs::path GetInputFolder()
{
    if (inputFolder != "")
    {
        return inputFolder;
    }

    inputFolder = FindFolderInParentDirs("input");

    return inputFolder;
}

fs::path outputFolder = "";
fs::path GetOutputFolder()
{
    if (outputFolder != "")
    {
        return outputFolder;
    }

    outputFolder = FindFolderInParentDirs("output");

    return outputFolder;
}

// Parses untill the next comma in a CSV file
std::string ParseTillNextComma(const std::string& inputString, int& offset)
{
    std::string accumulator;

    char c = inputString[offset++];
    char parsingToken = ',';

    // Could be a "some string with a , in between" or a standard comma separation
    if (c == '\"')
    {
        parsingToken = '\"';
        c = inputString[offset++];
    }

    // Parse till next
    while (c != parsingToken && offset < inputString.length())
    {
        accumulator += c;
        c = inputString[offset++];
    }

    if (offset == inputString.length())
    {
        accumulator += c;
    }

    if (parsingToken == '\"')
    {
        offset++;
    }

    return accumulator;
}
