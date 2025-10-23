#include "FirebasePeopleStructureBuilder.h"
#include <chrono>
#include <iomanip>
#include <numeric>

std::string FirebasePeopleStructureBuilder::generatePersonDataJson(const PersonData& personData)
{
    std::ostringstream json;
    json << "{"
         << "\"personId\":\"" << personData.personId << "\","
         << "\"expectedWaitTime\":" << std::fixed << std::setprecision(2) << personData.expectedWaitTime << ","
         << "\"enteringTimestamp\":" << personData.enteringTimestamp << ","
         << "\"exitingTimestamp\":" << personData.exitingTimestamp << ","
         << "\"lineNumber\":" << personData.lineNumber << ","
         << "\"actualWaitTime\":" << std::fixed << std::setprecision(2) << personData.actualWaitTime << ","
         << "\"hasExited\":" << (personData.exitingTimestamp != 0 ? "true" : "false")
         << "}";
    return json.str();
}

std::string FirebasePeopleStructureBuilder::generatePeopleSummaryJson(const PeopleSummary& summary)
{
    std::ostringstream json;
    json << "{"
         << "\"totalPeople\":" << summary.totalPeople << ","
         << "\"activePeople\":" << summary.activePeople << ","
         << "\"completedPeople\":" << summary.completedPeople << ","
         << "\"historicalAvgExpectedWait\":" << std::fixed << std::setprecision(2) << summary.historicalAvgExpectedWait << ","
         << "\"historicalAvgActualWait\":" << std::fixed << std::setprecision(2) << summary.historicalAvgActualWait << ","
         << "\"lastUpdated\":\"" << summary.lastUpdated << "\""
         << "}";
    return json.str();
}

std::string FirebasePeopleStructureBuilder::getPersonDataPath(const std::string& personId)
{
    return "people/" + personId;
}

std::string FirebasePeopleStructureBuilder::getPeopleSummaryPath()
{
    return "overallStats";
}

FirebasePeopleStructureBuilder::PeopleSummary FirebasePeopleStructureBuilder::createPeopleSummary(const std::vector<Person>& allPeople)
{
    int totalPeople = static_cast<int>(allPeople.size());
    int activePeople = 0;
    int completedPeople = 0;
    double totalExpectedWait = 0.0;
    double totalActualWait = 0.0;

    for (const auto& person : allPeople)
    {
        totalExpectedWait += person.getExpectedWaitTime();
        
        if (person.hasExited())
        {
            completedPeople++;
            totalActualWait += person.getActualWaitTime();
        }
        else
        {
            activePeople++;
        }
    }

    double averageExpectedWait = totalPeople > 0 ? totalExpectedWait / totalPeople : 0.0;
    double averageActualWait = completedPeople > 0 ? totalActualWait / completedPeople : 0.0;

    return PeopleSummary(totalPeople, activePeople, completedPeople, averageExpectedWait, averageActualWait);
}

std::string FirebasePeopleStructureBuilder::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now() + std::chrono::hours(3);
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}