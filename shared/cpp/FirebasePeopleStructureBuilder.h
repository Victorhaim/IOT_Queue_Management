#ifndef FIREBASE_PEOPLE_STRUCTURE_BUILDER_H
#define FIREBASE_PEOPLE_STRUCTURE_BUILDER_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "Person.h"

/**
 * FirebasePeopleStructureBuilder - Creates Firebase data structures for individual people tracking
 *
 * This class creates the Firebase structure for storing individual people data:
 * - people/[personId] (individual person data with timing information)
 * - summary (aggregated statistics about all people)
 */
class FirebasePeopleStructureBuilder
{
public:
    struct PersonData
    {
        std::string personId;
        double expectedWaitTime;
        long long enteringTimestamp;
        long long exitingTimestamp;
        int lineNumber;
        double actualWaitTime;

        PersonData(const Person& person)
            : personId(person.getId())
            , expectedWaitTime(person.getExpectedWaitTime())
            , enteringTimestamp(person.getEnteringTimestamp())
            , exitingTimestamp(person.getExitingTimestamp())
            , lineNumber(person.getLineNumber())
            , actualWaitTime(person.getActualWaitTime())
        {}
    };

    struct PeopleSummary
    {
        int totalPeople;
        int activePeople;          // People still in queue
        int completedPeople;       // People who have exited
        double averageExpectedWait;
        double averageActualWait;  // Only for completed people
        std::string lastUpdated;

        PeopleSummary(int total, int active, int completed, double avgExpected, double avgActual)
            : totalPeople(total), activePeople(active), completedPeople(completed)
            , averageExpectedWait(avgExpected), averageActualWait(avgActual)
            , lastUpdated(getCurrentTimestamp()) {}
    };

    /**
     * Generate JSON for individual person data
     * Structure: { personId, expectedWaitTime, enteringTimestamp, exitingTimestamp, lineNumber, actualWaitTime, hasExited }
     */
    static std::string generatePersonDataJson(const PersonData& personData);

    /**
     * Generate JSON for people summary data
     * Structure: { totalPeople, activePeople, completedPeople, averageExpectedWait, averageActualWait, lastUpdated }
     */
    static std::string generatePeopleSummaryJson(const PeopleSummary& summary);

    /**
     * Get the Firebase path for a specific person
     * Returns: "people/[personId]"
     */
    static std::string getPersonDataPath(const std::string& personId);

    /**
     * Get the Firebase path for people summary data
     * Returns: "people_summary"
     */
    static std::string getPeopleSummaryPath();

    /**
     * Create PeopleSummary from a vector of people
     */
    static PeopleSummary createPeopleSummary(const std::vector<Person>& allPeople);

    /**
     * Get current timestamp as std::string
     */
    static std::string getCurrentTimestamp();
};

#endif // FIREBASE_PEOPLE_STRUCTURE_BUILDER_H