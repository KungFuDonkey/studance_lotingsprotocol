#pragma once
#include <string>
#include <vector>
#include <filesystem>

#define fs std::filesystem

// trim from start (in place)
void ltrim(std::string& s);

// trim from end (in place)
void rtrim(std::string& s);

// trim from both ends (in place)
void trim(std::string& s);

// convert string to lower chars
void tolower(std::string& s);

// finds a file with multiple filenames
void FindInputFile(std::vector<std::string> fileNames, fs::path& filePath);
void FindOutputFile(std::vector<std::string> fileNames, fs::path& filePath);

// Returns true when a vector contains a value
template<typename T>
bool contains(std::vector<T> vec, const T& value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

fs::path GetInputFolder();

fs::path GetOutputFolder();

// Parses until the next comma and leaves the offset at the start of the next string in a csv line
std::string ParseTillNextComma(const std::string& inputString, int& offset);