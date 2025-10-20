#pragma once

#include <algorithm>
#include <vector>
#include <memory>
#include <list>
#include "FirebaseClient.h"
#include "FirebasePeopleStructureBuilder.h"
#include "ThroughputTracker.h"
#include "Person.h"

/// Line selection strategies for queue management
enum class LineSelectionStrategy
{
    SHORTEST_WAIT_TIME,    ///< Selects line with shortest estimated wait time (considers queue length + throughput)
    FEWEST_PEOPLE,         ///< Simply chooses line with fewest people (ignores throughput differences)
    FARTHEST_FROM_ENTRANCE ///< Chooses line where last person is farthest from entrance (assumes higher line numbers = farther)
};

/**
 * @brief Shared QueueManager implementation for ESP32 and simulation environments
 *
 * Manages multiple queue lines using intelligent line selection algorithms based on wait times.
 * Considers both queue length and service throughput for optimal customer routing.
 * Includes optional Firebase cloud integration for real-time data sharing.
 *
 * Features:
 * - Multiple line selection strategies (shortest wait, fewest people, farthest from entrance)
 * - Real-time throughput tracking and wait time estimation
 * - Firebase cloud integration with automatic data synchronization
 * - Thread-safe design suitable for embedded and simulation environments
 *
 * @note Uses 1-based line numbering for external API, 0-based internally
 * @note Firebase integration is optional and fails gracefully if unavailable
 */
class QueueManager
{
public:
    /**
     * @brief Constructs a QueueManager with optional Firebase integration
     * @param maxSize Maximum people per individual line (0 = unlimited)
     * @param numberOfLines Number of queue lines to manage (1-10)
     * @param strategyPrefix Firebase path prefix for data organization (e.g., "_shortest", "_farthest")
     * @param appName Firebase application name for cloud integration
     */
    QueueManager(int maxSize, int numberOfLines, const std::string &strategyPrefix = "", const std::string &appName = "iot-queue-management");
    ~QueueManager() = default;

    // Core queue operations
    /**
     * @brief Adds a person to the optimal line based on the given strategy
     * @param strategy Line selection algorithm to use (SHORTEST_WAIT_TIME, FEWEST_PEOPLE, FARTHEST_FROM_ENTRANCE)
     * @return true if person was successfully added, false if queue is full
     */
    bool enqueue(LineSelectionStrategy strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME);

    /**
     * @brief Removes a person from the specified line (service completion)
     * @param lineNumber Line number to remove person from (1-based indexing)
     * @return true if person was successfully removed, false if line is empty or invalid
     */
    bool dequeue(int lineNumber);

    /**
     * @brief Adds a person directly to a specific line, bypassing strategy selection
     * @param lineNumber Target line number (1-based indexing)
     * @return true if person was successfully added, false if queue is full or line is invalid
     */
    bool enqueueOnLine(int lineNumber);

    // State queries
    /**
     * @brief Gets the total number of people across all lines
     * @return Total people count in the entire queue system
     */
    int size() const;

    /**
     * @brief Checks if all lines are empty
     * @return true if no people are in any line, false otherwise
     */
    bool isEmpty() const;

    /**
     * @brief Determines the optimal line number based on the given strategy
     * @param strategy Line selection algorithm to use
     * @return Recommended line number (1-based), or -1 if no lines available
     */
    int getNextLineNumber(LineSelectionStrategy strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME) const;

    /**
     * @brief Gets the number of people in a specific line
     * @param lineNumber Line to query (1-based indexing)
     * @return Number of people in the line, or -1 if line number is invalid
     */
    int getLineCount(int lineNumber) const;

    /**
     * @brief Gets all people currently in the queue system
     * @return Vector containing all people across all lines
     */
    std::vector<Person> getAllPeople() const;

    /**
     * @brief Gets all people in a specific line
     * @param lineNumber Line to query (1-based indexing)
     * @return Vector containing all people in the specified line
     */
    std::vector<Person> getPeopleInLine(int lineNumber) const;

    /**
     * @brief Gets cumulative statistics for all people throughout the simulation
     * @return PeopleSummary with cumulative statistics
     */
    FirebasePeopleStructureBuilder::PeopleSummary getCumulativePeopleSummary() const;

    /**
     * @brief Updates cloud with all people data from the last hour and cleans local history
     * This is designed to be called when WiFi reconnects after being offline
     * @return true if successfully updated cloud and cleaned data, false otherwise
     */
    bool updateAllAndCleanHistory();

    /**
     * @brief Gets all people who entered the queues in the last hour
     * @return Vector containing all people from the last hour with their complete information
     */
    std::vector<Person> getPeopleFromLastHour() const;

    /**
     * @brief Calculates estimated wait time for a specific line
     * @param lineNumber Line to analyze (1-based indexing)
     * @return Estimated wait time in seconds, based on queue length and throughput
     */
    double getEstimatedWaitTime(int lineNumber) const;

    /**
     * @brief Calculates estimated wait time for a new person entering a specific line
     * This calculates time until becoming first in line (not until leaving)
     * @param lineNumber Line to analyze (1-based indexing)
     * @return Estimated wait time in seconds until becoming first in line
     */
    double getEstimatedWaitTimeForNewPerson(int lineNumber) const;

private:
    static const int MAX_LINES = 10;                  // Historical cap; still enforced to avoid runaway usage
    static constexpr double DEFAULT_THROUGHPUT = 0.5; // Default people/second when no data available

    int m_maxSize;
    int m_numberOfLines;
    int m_totalPeople;
    std::vector<std::list<Person>> m_lines; // Each line contains a list of Person objects

    // Running statistics for all people throughout simulation
    int m_totalPeopleEver;          // Total people who have ever entered
    int m_completedPeopleEver;      // Total people who have completed (exited)
    double m_totalExpectedWaitTime; // Sum of all expected wait times
    double m_totalActualWaitTime;   // Sum of all actual wait times for completed people
    int m_lastSelectedLine;         // Last line selected by enqueue strategy

    // Optional Firebase integration
    std::shared_ptr<FirebaseClient> m_firebaseClient;
    std::string m_strategyPrefix;                        // e.g., "", "_shortest", "_farthest"
    std::vector<ThroughputTracker> m_throughputTrackers; // Throughput tracking for each line

    // History tracking for offline functionality
    std::vector<Person> m_lastHourHistory; // Queue of all people who entered in the last hour

    // Helper methods
    bool isValidLineNumber(int lineNumber) const;
    void updateTotalPeople();
    int getNumberOfLines() const;
    bool writeToFirebase();
    void clearCloudData();
    void setLineCount(int lineNumber, int count);
    void reset();
    
    // History management helpers
    void addPersonToHistory(const Person& person);
    void cleanOldHistoryEntries();
    bool writeHistoryToFirebase();
};
