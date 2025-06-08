#include "Studancer.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <random>

std::vector<std::string> DancerFileNames()
{
    return {
        "dansers.csv",
        "dancers.csv",
        "Dansers.csv",
        "Dancers.csv"
    };
}

std::string DancerPriorityGroupToString(DancerPriorityGroup group)
{
    switch (group)
    {
    case Board: return "Board";
    case Damn: return "Damn";
    case ExistingMember: return "ExistingMember";
    case NonDancerLastYear: return "NonDancerLastYear";
    case UnrolledLastYear: return "UnrolledLastYear";
    case NonFemale: return "NonFemale";
    case Female: return "Female";
    case HalfYear: return "HalfYear";
    case GapYear: return "GapYear";
    case HalfGapYear: return "HalfGapYear";
    case NonStudying: return "NonStudying";
    case HalfNonStudying: return "HalfNonStudying";
    default: return "Unkown";
    }
    return "Unkown";
}

std::string inputHeader;
std::string GetDancersInputHeader()
{
    return inputHeader;
}

// If we want to be able to parse the indices at a different location
std::map<std::string, int> inputHeaderMap;
std::map<std::string, int> GetDancersInputHeaderMap()
{
    return inputHeaderMap;
}

// Load all dancers from an input file
std::vector<Studancer> LoadDancers(const std::vector<DanceClass>& classes)
{
    std::vector<Studancer> dancers;

    // Create array of class names for checking chosen and advised classes
    std::vector<std::string> classNames;
    classNames.reserve(classes.size());
    for (auto danceClass : classes)
    {
        classNames.push_back(danceClass.name);
    }

    // possible file names for the dansers file
    std::vector<std::string> dancersFileNames = DancerFileNames();

    // possible file names for the board/damn file
    std::vector<std::string> boardFileNames = {
        "Board.txt"
    };

    // find files
    fs::path dancersFilePath;
    FindInputFile(dancersFileNames, dancersFilePath);
    fs::path boardFilePath;
    FindInputFile(boardFileNames, boardFilePath);

    // open the board file
    std::string line;
    std::ifstream boardFile(boardFilePath);

    std::vector<int> boardMembers;
    std::vector<int> damnMembers;

    // Load numbersx that correspond to board members
    while (std::getline(boardFile, line))
    {
        std::stringstream lineStream(line);
        std::string parseLine;
        while (std::getline(lineStream, parseLine, ','))
        {
            boardMembers.push_back(std::stoi(parseLine));
        }
    }

    // open the dancers file
    std::ifstream dancersFile(dancersFilePath);

    // Get the header
    std::getline(dancersFile, inputHeader);

    // Index the header
    int offset = 0;
    int index = 0;
    while (offset < inputHeader.length())
    {
        // Get lower version of header
        std::string currentHeader = ParseTillNextComma(inputHeader, offset);
        tolower(currentHeader);
        trim(currentHeader);
        // remove symbols
        currentHeader.erase(currentHeader.find_last_not_of(" \n\r\t:?") + 1);

        if (inputHeaderMap.count(currentHeader))
        {
            printf("Found duplicate header in input file: %s", currentHeader.c_str());
            exit(-1);
        }

        inputHeaderMap.emplace(currentHeader, index);
        index++;
    }

    // Required headers for algorihtm to work
    std::vector<std::string> requiredHeaders = {
        "studentstatus",
        "gender",
        "1e keuze",
        "2e keuze",
        "3e keuze",
        "advies",
        "lidmaatschap",
        "relatienummer"
    };
    std::vector<std::string> failedHeaders;
    for (auto& header : requiredHeaders)
    {
        if (!inputHeaderMap.count(header))
        {
            failedHeaders.push_back(header);
        }
    }

    if (failedHeaders.size() > 0)
    {
        printf("Failed to find headers in dancers file:\n");
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

    // Parse the dancers
    int tableIndex = 0;
    while (std::getline(dancersFile, line))
    {
        Studancer dancer = {};

        offset = 0;
        for (int i = 0; i < numIndices; i++)
        {
            indices[i] = ParseTillNextComma(line, offset);
        }

        // TODO wasNonDancingMemberLastYear
        // TODO wasUnrolledLastYear

        // Store the input row for export
        dancer.tableRow = line;

        std::string relationNumber = indices[inputHeaderMap["relatienummer"]];
        trim(relationNumber);
        dancer.relationNumber = std::stoi(relationNumber);

        std::string studentStatus = indices[inputHeaderMap["studentstatus"]];
        trim(studentStatus);
        tolower(studentStatus);
        bool isStudent = studentStatus == "student";
        bool hasGapYear = studentStatus == "tussenjaar";

        std::string wasAMember = indices[inputHeaderMap["ben je al lid"]];
        trim(wasAMember);
        tolower(wasAMember);
        bool isNewMember = wasAMember == "nee";

        std::string gender = indices[inputHeaderMap["gender"]];
        trim(gender);
        tolower(gender);
        bool isNonFemale = gender != "vrouw";

        // Choices need to be cleaned
        std::stringstream firstChoiceUnparsed(indices[inputHeaderMap["1e keuze"]]);
        std::string firstChoice;
        std::getline(firstChoiceUnparsed, firstChoice, '(');
        trim(firstChoice);
        tolower(firstChoice);

        std::stringstream secondChoiceUnparsed(indices[inputHeaderMap["2e keuze"]]);
        std::string secondChoice;
        std::getline(secondChoiceUnparsed, secondChoice, '(');
        trim(secondChoice);
        tolower(secondChoice);

        std::stringstream thirdChoiceUnparsed(indices[inputHeaderMap["3e keuze"]]);
        std::string thirdChoice;
        std::getline(thirdChoiceUnparsed, thirdChoice, '(');
        trim(thirdChoice);
        tolower(thirdChoice);

        dancer.chosenClasses.push_back(firstChoice);

        dancer.chosenClasses.push_back(secondChoice);

        dancer.chosenClasses.push_back(thirdChoice);

        dancer.chosenClasses.push_back("unenrolled");

        // check and sanitize choices
        for (int i = 0; i < dancer.chosenClasses.size(); i++)
        {
            // sanitize empty choices
            if (dancer.chosenClasses[i] == "maak een keuze")
            {
                dancer.chosenClasses[i] = "";
            }

            // If we have the same choice as before, set it to empty
            for (int j = 0; j < i; j++)
            {
                if (dancer.chosenClasses[i] == dancer.chosenClasses[j])
                {
                    dancer.chosenClasses[i] = "";
                }
            }

            // We do not need to check emtpy choices
            if (dancer.chosenClasses[i] == "")
            {
                continue;
            }

            // Check if we have the chosen class in the list. Otherwise we have an input issue
            if (!contains(classNames, dancer.chosenClasses[i]))
            {
                printf("ERROR: Chosen class %s for dancer %s does not exist in the input classes file\n", dancer.chosenClasses[i].c_str(), relationNumber.c_str());
                printf("Aborting...\n");
                exit(-1);
            }
        }

        std::string advice = indices[inputHeaderMap["advies"]];
        trim(advice);
        tolower(advice);
        if (advice != "ik was vorig jaar geen lid" && advice != "maak een keuze" && advice != "nee")
        {
            if (advice == "ja" && dancer.chosenClasses.size() > 0)
            {
                // binary advice
                dancer.advisedClasses.push_back(dancer.chosenClasses[0]);
            }
            else
            {
                // advice list is separated by commas
                std::stringstream advices(advice);
                std::string currentAdvice;
                while (std::getline(advices, currentAdvice, ','))
                {
                    dancer.advisedClasses.push_back(currentAdvice);
                }
            }
        }

        std::string requestedMembership = indices[inputHeaderMap["lidmaatschap"]];
        trim(requestedMembership);
        tolower(requestedMembership);
        bool halfYearMemberShip = requestedMembership == "halfjaarlijkslidmaatschap";

        bool isBoard = contains(boardMembers, dancer.relationNumber);

        // Damn members pick damn as first choice
        bool isDamn = firstChoice == "d.a.m.n.";

        if (isBoard)
        {
            dancer.priorityGroup = Board;
        }
        else if (isDamn)
        {
            dancer.priorityGroup = Damn;
        }
        else if (hasGapYear)
        {
            if (halfYearMemberShip)
            {
                dancer.priorityGroup = HalfGapYear;
            }
            else
            {
                dancer.priorityGroup = GapYear;
            }
        }
        else if (!isStudent)
        {
            if (halfYearMemberShip)
            {
                dancer.priorityGroup = HalfNonStudying;
            }
            else
            {
                dancer.priorityGroup = NonStudying;
            }
        }
        else if (halfYearMemberShip)
        {
            dancer.priorityGroup = HalfYear;
        }
        // TODO was Non dancing member and unrolled
        else if (!isNewMember)
        {
            dancer.priorityGroup = ExistingMember;
        }
        else if (isNonFemale)
        {
            dancer.priorityGroup = NonFemale;
        }
        else
        {
            dancer.priorityGroup = Female;
        }


        dancers.push_back(dancer);

        tableIndex++;
    }

    // close file
    dancersFile.close();

    // Shuffle dancers for random priority assignment
    // This is basically all the randomness necessary for a fair assignment
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(dancers.begin(), dancers.end(), g);

    return dancers;
}


