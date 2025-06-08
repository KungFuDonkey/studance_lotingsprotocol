#include "Statistics.h"
#include "Utils.h"
#include <fstream>

void PrintAssignmentStats(const Assignment& assignment)
{
    // count of number of first, second and third choices for dancer prio groups
    // and classes
    int buckets[DancerPriorityGroup::Count][4] = {};

    const int numClasses = (int)assignment.size();
    const int numClassBuckets = (int)assignment.size() * 3;
    int* classBuckets = new int[numClassBuckets];
    for (int i = 0; i < numClassBuckets; i++)
    {
        classBuckets[i] = 0;
    }
    int classIndex = 0;
    int unenrolled = 0;
    for (auto& classAssignment : assignment)
    {
        for (auto& dancer : classAssignment.second)
        {
            for (int i = 0; i < 4 && i < dancer.chosenClasses.size(); i++)
            {
                if (dancer.chosenClasses[i] == classAssignment.first.name)
                {
                    buckets[(int)dancer.priorityGroup][i]++;

                    if (classAssignment.first.name != "unenrolled")
                    {
                        classBuckets[classIndex * 3 + i]++;
                    }
                    else
                    {
                        unenrolled++;
                    }
                }
            }
        }
        classIndex++;
    }

    {
        // Create a table for all assignment groups, how many were first second and third choices
        const int longestPriorityGroupName = (int)strlen("NonDancerLastYear") + 1;
        std::string groupName = "| Group Name";
        for (int i = 0; i < longestPriorityGroupName - (int)strlen("| Group Name") + 2; i++)
        {
            groupName += " ";
        }
        groupName += "|";
        std::string headerRow = "";
        for (int i = 0; i < groupName.length(); i++)
        {
            headerRow += "=";
        }
        printf("Assignment Statistics:\n\n");
        printf("%s====================================================================\n", headerRow.c_str());
        printf("%s|    1st choice ||    2nd choice ||    3rd choice ||    unenrolled |\n", groupName.c_str());
        printf("%s====================================================================\n", headerRow.c_str());

        for (int i = 0; i < (int)DancerPriorityGroup::Count - 1; i++)
        {
            DancerPriorityGroup group = (DancerPriorityGroup)i;
            std::string priorityGroupName = DancerPriorityGroupToString(group);
            printf("| %s", priorityGroupName.c_str());

            for (int space = 0; space < longestPriorityGroupName - priorityGroupName.length(); space++)
            {
                printf(" ");
            }
            printf("|");

            int total = buckets[i][0] + buckets[i][1] + buckets[i][2] + buckets[i][3];
            if (total == 0)
            {
                printf("|   0 (  0.00%%) ||   0 (  0.00%%) ||   0 (  0.00%%) ||   0 (  0.00%%) |\n");
            }
            else
            {
                float percentages[4] = {
                    ((float)buckets[i][0] / (float)total) * 100.f,
                    ((float)buckets[i][1] / (float)total) * 100.f,
                    ((float)buckets[i][2] / (float)total) * 100.f,
                    ((float)buckets[i][3] / (float)total) * 100.f,
                };

                int bucketIndex = 0;
                for (int bucket = 0; bucket < 4; bucket++)
                {
                    float p = percentages[bucket];
                    int v = buckets[i][bucket];
                    std::string percentageSpacing = p == 100.0f ? "" : p >= 10.0f ? " " : "  ";
                    std::string numberSpacing = v >= 100 ? "" : v >= 10 ? " " : "  ";
                    printf("| %s%i (%s%.2f%%) |", numberSpacing.c_str(), v, percentageSpacing.c_str(), p);
                    bucketIndex++;
                }
                printf("\n");
            }
        }
        printf("%s====================================================================\n\n\n", headerRow.c_str());
    }

    {
        int longestClassName = 0;
        for (auto& classAssignment : assignment)
        {
            if (classAssignment.first.name.length() > longestClassName)
            {
                longestClassName = (int)classAssignment.first.name.length();
            }
        }
        longestClassName++;

        std::string classNameColumn = "| Class Name";
        for (int i = 0; i < longestClassName - (int)strlen("| Class Name") + 2; i++)
        {
            classNameColumn += " ";
        }
        classNameColumn += "|";
        std::string headerRow = "";
        for (int i = 0; i < classNameColumn.length(); i++)
        {
            headerRow += "=";
        }

        printf("Class Statistics:\n\n");
        printf("%s============================================================\n", headerRow.c_str());
        printf("%s| Total ||    1st choice ||    2nd choice ||    3rd choice |\n", classNameColumn.c_str());
        printf("%s============================================================\n", headerRow.c_str());
        classIndex = 0;
        for (auto& classAssignment : assignment)
        {
            std::string className = classAssignment.first.name;
            if (className == "unenrolled")
            {
                classIndex++;
                continue;
            }

            printf("| %s", className.c_str());

            for (int space = 0; space < longestClassName - className.length(); space++)
            {
                printf(" ");
            }
            printf("|");

            int total = classBuckets[classIndex * 3 + 0] + classBuckets[classIndex * 3 + 1] + classBuckets[classIndex * 3 + 2];
            if (total == 0)
            {
                printf("|     0 ||   0 (  0.00%%) ||   0 (  0.00%%) ||   0 (  0.00%%) |\n");
            }
            else
            {
                std::string totalSpacing = total >= 100 ? "  " : total >= 10 ? "   " : "    ";
                printf("| %s%i |", totalSpacing.c_str(), total);

                float percentages[3] = {
                    ((float)classBuckets[classIndex * 3 + 0] / (float)total) * 100.f,
                    ((float)classBuckets[classIndex * 3 + 1] / (float)total) * 100.f,
                    ((float)classBuckets[classIndex * 3 + 2] / (float)total) * 100.f
                };

                int bucketIndex = 0;
                for (int bucket = 0; bucket < 3; bucket++)
                {
                    float p = percentages[bucket];
                    int v = classBuckets[classIndex * 3 + bucket];
                    std::string percentageSpacing = p == 100.0f ? "" : p >= 10.0f ? " " : "  ";
                    std::string numberSpacing = v >= 100 ? "" : v >= 10 ? " " : "  ";
                    printf("| %s%i (%s%.2f%%) |", numberSpacing.c_str(), v, percentageSpacing.c_str(), p);
                    bucketIndex++;
                }
                printf("\n");
            }
            classIndex++;
        }
        printf("%s============================================================\n\n", headerRow.c_str());

        printf("unenrolled members: %i\n\n", unenrolled);
    }

    delete[] classBuckets;
}