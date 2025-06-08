#include "Lottery.h"
#include "Utils.h"
#include <random>

typedef std::map<std::string, std::pair<DanceClass, std::vector<Studancer>>> LotteryAssignment;

void AssignPriorityGroup(std::vector<Studancer>& group, LotteryAssignment& assignment)
{
    for (auto& dancer : group)
    {
        for (auto& chosenClass : dancer.chosenClasses)
        {
            if (chosenClass == "")
            {
                continue;
            }

            auto& classAssignment = assignment[chosenClass];

            // Check if there is space in this class
            if (classAssignment.second.size() < classAssignment.first.maxSize)
            {
                // There is space, so assign the dancer
                classAssignment.second.push_back(dancer);
                break;
            }
        }
    }
}

Assignment Lottery(const std::vector<Studancer>& dancers, const std::vector<DanceClass>& classes)
{
    std::vector<Studancer> priorityBuckets[DancerPriorityGroup::Count];

    std::vector<Studancer> followingAdvice;

    // NOTE: dancers are already suffled in LoadDancers()
    for (auto& dancer : dancers)
    {
        if (dancer.priorityGroup == ExistingMember &&
            dancer.chosenClasses.size() > 0 &&
            dancer.chosenClasses[0] != "" &&
            contains(dancer.advisedClasses, dancer.chosenClasses[0]))
        {
            followingAdvice.push_back(dancer);
        }
        else
        {
            priorityBuckets[dancer.priorityGroup].push_back(dancer);
        }
    }

    // for easier finding of classes
    LotteryAssignment assignment;
    for (int i = 0; i < classes.size(); i++)
    {
        assignment.emplace(std::make_pair(classes[i].name, std::make_pair(classes[i], std::vector<Studancer>())));
    }

    // Assign board and damn
    for (int p = 0; p <= DancerPriorityGroup::Damn; p++)
    {
        AssignPriorityGroup(priorityBuckets[p], assignment);
    }

    // Special case: Existing members following advice
    {
        bool reshuffle = false;
        for (auto& dancer : followingAdvice)
        {
            std::string chosenClass = dancer.chosenClasses[0];
            if (chosenClass == "")
            {
                continue;
            }

            auto& classAssignment = assignment[chosenClass];

            // Check if there is space in this class
            if (classAssignment.second.size() < classAssignment.first.maxSize)
            {
                // There is space, so assign the dancer
                classAssignment.second.push_back(dancer);
            }
            else
            {
                // Disable this dancer from trying to pick this class again
                dancer.chosenClasses[0] = "";

                // Reinsert this dancer
                priorityBuckets[DancerPriorityGroup::ExistingMember].push_back(dancer);

                reshuffle = true;
            }
        }

        if (reshuffle)
        {
            // Reshuffle the ExistingMember prio group again, because otherwise the 'following advice' people will always
            // be at the back of the buffer.
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(priorityBuckets[DancerPriorityGroup::ExistingMember].begin(), priorityBuckets[DancerPriorityGroup::ExistingMember].end(), g);
        }
    }

    // Assign other priority groups
    for (int p = DancerPriorityGroup::ExistingMember; p <= DancerPriorityGroup::Count; p++)
    {
        AssignPriorityGroup(priorityBuckets[p], assignment);
    }

    // Create final assignment type
    Assignment finalAssignment;
    for (auto& danceClass : classes)
    {
        finalAssignment.push_back(assignment[danceClass.name]);
    }

    printf("Assigned everyone\n");


    return finalAssignment;
}
