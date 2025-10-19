#include "QueueManager.h"
#include "FirebaseClient.h"
#include "FirebaseStructureBuilder.h"
#include "FirebasePeopleStructureBuilder.h"
#include "ThroughputTracker.h"
#include "Person.h"
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <limits>
#include <algorithm>
#include <chrono>

// Constants
static const long long ONE_HOUR_MS = 60 * 60 * 1000; // One hour in milliseconds

QueueManager::QueueManager(int maxSize, int numberOfLines, const std::string &strategyPrefix, const std::string &appName)
    : m_maxSize(maxSize), m_numberOfLines(numberOfLines), m_totalPeople(0), m_lines(), m_lineThroughputs(),
      m_firebaseClient(nullptr), m_strategyPrefix(strategyPrefix), m_throughputTrackers(numberOfLines),
      m_totalPeopleEver(0), m_completedPeopleEver(0), m_totalExpectedWaitTime(0.0), m_totalActualWaitTime(0.0), m_lastSelectedLine(-1)
{
    if (m_numberOfLines < 0)
        m_numberOfLines = 0;
    if (m_numberOfLines > MAX_LINES)
        m_numberOfLines = MAX_LINES;                      // enforce historical cap
    m_lines.reserve(m_numberOfLines);                     // reserve capacity to avoid reallocations
    m_lines.assign(m_numberOfLines, std::list<Person>()); // initialize with empty lists

    // Initialize throughput tracking for each line
    m_lineThroughputs.reserve(m_numberOfLines);
    m_lineThroughputs.assign(m_numberOfLines, DEFAULT_THROUGHPUT);

    // Initialize Firebase client with provided app name
    m_firebaseClient = std::make_shared<FirebaseClient>(
        appName,
        "https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app");

    // Initialize Firebase client
    if (!m_firebaseClient->initialize())
    {
        std::cerr << "Failed to initialize Firebase client for " << appName << "!" << std::endl;
        m_firebaseClient = nullptr; // Disable cloud functionality
    }
    else
    {
        std::cout << "Firebase client initialized successfully for " << appName << std::endl;
        // Clear existing data when QueueManager starts
        clearCloudData();
    }
}

bool QueueManager::enqueue(LineSelectionStrategy strategy)
{
    int lineNumber = getNextLineNumber(strategy);
    if (lineNumber == -1)
    {
        return false; // No available lines (all at capacity or no lines exist)
    }

    // Store the selected line for Firebase reporting
    m_lastSelectedLine = lineNumber;

    // Check per-line capacity only
    if (m_maxSize > 0 && static_cast<int>(m_lines[lineNumber - 1].size()) >= m_maxSize)
    {
        return false; // Selected line is at capacity
    }

    // Calculate expected wait time for this person
    double expectedWaitTime = getEstimatedWaitTimeForNewPerson(lineNumber);

    // Create a new person and add to the line
    Person newPerson(expectedWaitTime, lineNumber);
    auto &line = m_lines[lineNumber - 1];
    line.push_back(newPerson);
    m_totalPeople++;

    // Add person to history for offline functionality
    addPersonToHistory(newPerson);

    // Update running statistics
    m_totalPeopleEver++;
    m_totalExpectedWaitTime += expectedWaitTime;

    // If this person is first in line, set their exit timestamp immediately
    if (line.size() == 1)
    {
        line.front().recordExit();
        // Update completion statistics
        m_completedPeopleEver++;
        m_totalActualWaitTime += line.front().getActualWaitTime();
    }

    // Automatically write to Firebase after state change
    writeToFirebase();

    return true;
}

bool QueueManager::dequeue(int lineNumber)
{
    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    // Check if line has people
    if (m_lines[lineNumber - 1].empty())
    {
        return false;
    }

    // Remove the first person from the line (they have already had their exit timestamp set when they became first in line)
    auto &line = m_lines[lineNumber - 1];
    if (!line.empty())
    {
        line.pop_front();
        m_totalPeople--;

        // If there is a new first person, set their exit timestamp now
        if (!line.empty() && !line.front().hasExited())
        {
            line.front().recordExit();

            // Update completion statistics
            m_completedPeopleEver++;
            m_totalActualWaitTime += line.front().getActualWaitTime();
        }
    }

    // Record service completion for throughput tracking
    m_throughputTrackers[lineNumber - 1].recordServiceCompletion();

    // Automatically write to Firebase after state change
    writeToFirebase();

    return true;
}

bool QueueManager::enqueueOnLine(int lineNumber)
{
    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    // Check if the specific line is at capacity
    if (m_maxSize > 0 && static_cast<int>(m_lines[lineNumber - 1].size()) >= m_maxSize)
    {
        return false; // This specific line is at capacity
    }

    // Calculate expected wait time for this person
    double expectedWaitTime = getEstimatedWaitTimeForNewPerson(lineNumber);

    // Create a new person and add to the specified line
    Person newPerson(expectedWaitTime, lineNumber);
    auto &line = m_lines[lineNumber - 1];
    line.push_back(newPerson);
    m_totalPeople++;

    // Add person to history for offline functionality
    addPersonToHistory(newPerson);

    // Update running statistics
    m_totalPeopleEver++;
    m_totalExpectedWaitTime += expectedWaitTime;

    // If this person is first in line, set their exit timestamp immediately
    if (line.size() == 1)
    {
        line.front().recordExit();
        // Update completion statistics
        m_completedPeopleEver++;
        m_totalActualWaitTime += line.front().getActualWaitTime();
    }

    // Automatically write to Firebase after state change
    writeToFirebase();

    return true;
}

int QueueManager::size() const
{
    return m_totalPeople;
}

bool QueueManager::isEmpty() const
{
    return m_totalPeople == 0;
}

int QueueManager::getNextLineNumber(LineSelectionStrategy strategy) const
{
    if (m_numberOfLines == 0)
    {
        return -1;
    }

    // Check if a line is at capacity
    auto isLineAtCapacity = [this](int lineIndex) -> bool
    {
        return m_maxSize > 0 && static_cast<int>(m_lines[lineIndex].size()) >= m_maxSize;
    };

    switch (strategy)
    {
    case LineSelectionStrategy::SHORTEST_WAIT_TIME:
    {
        double minWaitTime = std::numeric_limits<double>::max();
        int bestLine = -1;

        for (int i = 1; i <= m_numberOfLines; i++)
        {
            if (isLineAtCapacity(i - 1))
                continue;
            double waitTime = getEstimatedWaitTimeForNewPerson(i);
            if (waitTime < minWaitTime)
            {
                minWaitTime = waitTime;
                bestLine = i;
            }
        }
        return bestLine;
    }

    case LineSelectionStrategy::FEWEST_PEOPLE:
    {
        int minPeople = std::numeric_limits<int>::max();
        int bestLine = -1;

        for (int i = 0; i < m_numberOfLines; i++)
        {
            if (isLineAtCapacity(i))
                continue;
            int peopleCount = static_cast<int>(m_lines[i].size());
            if (peopleCount < minPeople)
            {
                minPeople = peopleCount;
                bestLine = i + 1;
            }
        }
        return bestLine;
    }

    case LineSelectionStrategy::FARTHEST_FROM_ENTRANCE:
    {
        int bestLine = -1;

        // Find the farthest line that's not at capacity
        for (int i = m_numberOfLines; i >= 1; i--)
        {
            if (!isLineAtCapacity(i - 1))
            {
                bestLine = i;
                break;
            }
        }
        return bestLine;
    }

    default:
        return getNextLineNumber(LineSelectionStrategy::SHORTEST_WAIT_TIME);
    }
}

int QueueManager::getNumberOfLines() const
{
    return m_numberOfLines;
}

int QueueManager::getLineCount(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return -1;
    }

    return static_cast<int>(m_lines[lineNumber - 1].size());
}

void QueueManager::setLineCount(int lineNumber, int count)
{
    if (!isValidLineNumber(lineNumber))
    {
        return;
    }

    // Clear the line and recreate with empty Person objects for the count
    m_totalPeople -= static_cast<int>(m_lines[lineNumber - 1].size());
    m_lines[lineNumber - 1].clear();

    // Add dummy persons if count > 0 (for simulation purposes)
    int validCount = (count < 0) ? 0 : count;
    for (int i = 0; i < validCount; i++)
    {
        double expectedWaitTime = getEstimatedWaitTime(lineNumber);
        Person dummyPerson(expectedWaitTime, lineNumber);
        m_lines[lineNumber - 1].push_back(dummyPerson);
    }

    m_totalPeople += validCount;
}

void QueueManager::reset()
{
    for (int i = 0; i < m_numberOfLines; i++)
    {
        m_lines[i].clear();
    }
    m_totalPeople = 0;
}

bool QueueManager::isValidLineNumber(int lineNumber) const
{
    return lineNumber >= 1 && lineNumber <= m_numberOfLines;
}

void QueueManager::updateTotalPeople()
{
    m_totalPeople = 0;
    for (int i = 0; i < m_numberOfLines; i++)
    {
        m_totalPeople += m_lines[i].size();
    }
}

void QueueManager::updateLineThroughput(int lineNumber, double throughputPerSecond)
{
    if (!isValidLineNumber(lineNumber))
    {
        return;
    }

    // Ensure reasonable bounds for throughput
    double boundedThroughput = std::max(0.1, std::min(5.0, throughputPerSecond));
    m_lineThroughputs[lineNumber - 1] = boundedThroughput;
}

double QueueManager::getLineThroughput(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return DEFAULT_THROUGHPUT;
    }

    return m_lineThroughputs[lineNumber - 1];
}

double QueueManager::getEstimatedWaitTime(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return 999.0; // Return high wait time for invalid lines
    }

    int peopleInLine = static_cast<int>(m_lines[lineNumber - 1].size());

    // Use throughput from tracker, fall back to default if no reliable data
    double throughput = m_throughputTrackers[lineNumber - 1].hasReliableData()
                            ? m_throughputTrackers[lineNumber - 1].getCurrentThroughput()
                            : DEFAULT_THROUGHPUT;

    // Estimated wait time = number of people ahead / service rate
    if (peopleInLine == 0)
    {
        return 0.0; // No wait for empty line
    }

    // Simple formula: time = people / throughput
    // Could be enhanced with more sophisticated queueing theory
    return static_cast<double>(peopleInLine) / throughput;
}

double QueueManager::getEstimatedWaitTimeForNewPerson(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return 999.0; // Return high wait time for invalid lines
    }

    int peopleInLine = static_cast<int>(m_lines[lineNumber - 1].size());

    // Use throughput from tracker, fall back to default if no reliable data
    double throughput = m_throughputTrackers[lineNumber - 1].hasReliableData()
                            ? m_throughputTrackers[lineNumber - 1].getCurrentThroughput()
                            : DEFAULT_THROUGHPUT;

    // Expected wait time for new person = time until they become first in line
    // This is the number of people currently in line * average service time
    if (peopleInLine == 0)
    {
        return 0.0; // No wait - they'll be first immediately
    }

    // Time = people ahead of them / service rate
    return static_cast<double>(peopleInLine) / throughput;
}

// Cloud integration methods
void QueueManager::clearCloudData()
{
    if (!m_firebaseClient)
    {
        return; // No Firebase client configured
    }

    std::cout << "🧹 Clearing existing cloud data..." << std::endl;

    try
    {
        // Clear all queue lines
        for (int i = 1; i <= m_numberOfLines; ++i)
        {
            std::string queuePath = "simulation" + m_strategyPrefix + "/queues/line" + std::to_string(i);
            if (m_firebaseClient->deleteData(queuePath))
            {
                std::cout << "✅ Successfully cleared existing data for "
                          << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "line " << i << std::endl;
            }
            else
            {
                std::cout << "ℹ️  Note: No existing data found for "
                          << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "line " << i << " or failed to clear" << std::endl;
            }
        }

        // Clear the currentBest aggregated data
        std::string aggPath = "simulation" + m_strategyPrefix + "/currentBest";
        if (m_firebaseClient->deleteData(aggPath))
        {
            std::cout << "✅ Successfully cleared " << aggPath << " data" << std::endl;
        }
        else
        {
            std::cout << "ℹ️  Note: No existing " << aggPath << " data found or failed to clear" << std::endl;
        }

        // Optional: Also clear the entire simulation node to ensure a fresh start
        std::string simulationPath = "simulation" + m_strategyPrefix;
        if (m_firebaseClient->deleteData(simulationPath))
        {
            std::cout << "✅ Successfully cleared all "
                      << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                      << "simulation data" << std::endl;
        }
        else
        {
            std::cout << "ℹ️  Note: No existing "
                      << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                      << "simulation data found or failed to clear all simulation data" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "⚠️  Warning: Error clearing cloud data: " << e.what() << std::endl;
        std::cerr << "Continuing with simulation..." << std::endl;
    }

    std::cout << "🚀 Starting fresh simulation..." << std::endl;
}

bool QueueManager::writeToFirebase()
{
    if (!m_firebaseClient)
    {
        // No Firebase client configured - this is optional functionality
        return false;
    }

    try
    {
        // Collect data for all lines
        std::vector<FirebaseStructureBuilder::LineData> allLinesData;
        int totalPeople = 0;

        for (int line = 1; line <= m_numberOfLines; ++line)
        {
            int currentOccupancy = getLineCount(line);

            // Use throughput from internal trackers
            double throughputFactor = m_throughputTrackers[line - 1].getCurrentThroughput();

            double averageWaitTime = FirebaseStructureBuilder::calculateAverageWaitTime(
                currentOccupancy, throughputFactor);

            totalPeople += currentOccupancy;

            // Create line data object
            FirebaseStructureBuilder::LineData lineData(
                currentOccupancy, throughputFactor, averageWaitTime, line);
            allLinesData.push_back(lineData);

            // Generate JSON and write to Firebase for this specific line
            std::string json = FirebaseStructureBuilder::generateLineDataJson(lineData);
            std::string queuePath = "simulation" + m_strategyPrefix + "/queues/line" + std::to_string(line);

            if (m_firebaseClient->updateData(queuePath, json))
            {
                std::cout << "✅ " << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "Line " << line << " updated - Occupancy: " << currentOccupancy
                          << ", Throughput: " << std::fixed << std::setprecision(3) << throughputFactor
                          << ", Avg Wait: " << std::fixed << std::setprecision(1) << averageWaitTime << "s"
                          << " [" << (m_throughputTrackers[line - 1].hasReliableData() ? "measured" : "default") << "]"
                          << std::endl;
            }
            else
            {
                std::cerr << "❌ Failed to update Firebase for "
                          << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "line " << line << std::endl;
                return false;
            }
        }

        // Calculate recommended line and write aggregated data
        if (!allLinesData.empty())
        {
            FirebaseStructureBuilder::AggregatedData aggData =
                FirebaseStructureBuilder::createAggregatedData(allLinesData.data(), totalPeople, static_cast<int>(allLinesData.size()), m_lastSelectedLine);

            std::string aggJson = FirebaseStructureBuilder::generateAggregatedDataJson(aggData);
            std::string aggPath = "simulation" + m_strategyPrefix + "/currentBest";

            if (m_firebaseClient->updateData(aggPath, aggJson))
            {
                std::cout << "✅ Aggregated " << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "queue object updated (simulation" << m_strategyPrefix << "/currentBest) totalPeople=" << totalPeople
                          << " recommendedLine=" << aggData.recommendedLine
                          << " waitTime=" << std::round(aggData.averageWaitTime) << "s"
                          << " placeInLine=" << aggData.currentOccupancy << std::endl;
            }
            else
            {
                std::cerr << "❌ Failed to update aggregated "
                          << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ")
                          << "queue object" << std::endl;
                return false;
            }
        }

        // Write individual people data to Firebase
        std::vector<Person> allPeople = getAllPeople();

        // Write cumulative people summary (includes all people from entire simulation)
        FirebasePeopleStructureBuilder::PeopleSummary summary = getCumulativePeopleSummary();

        std::string summaryJson = FirebasePeopleStructureBuilder::generatePeopleSummaryJson(summary);
        std::string summaryPath = "simulation" + m_strategyPrefix + "/" +
                                  FirebasePeopleStructureBuilder::getPeopleSummaryPath();

        if (m_firebaseClient->updateData(summaryPath, summaryJson))
        {
            std::cout << "✅ People summary updated: " << summary.totalPeople
                      << " total, " << summary.activePeople << " active, "
                      << summary.completedPeople << " completed" << std::endl;
        }
        else
        {
            std::cerr << "❌ Failed to update people summary" << std::endl;
        }

        // Write individual people data (limit to recent people to avoid overwhelming Firebase)
        int peopleWritten = 0;
        const int MAX_PEOPLE_TO_WRITE = 50; // Limit to avoid Firebase quota issues

        for (const auto &person : allPeople)
        {
            if (peopleWritten >= MAX_PEOPLE_TO_WRITE)
                break;

            FirebasePeopleStructureBuilder::PersonData personData(person);
            std::string personJson = FirebasePeopleStructureBuilder::generatePersonDataJson(personData);
            std::string personPath = "simulation" + m_strategyPrefix + "/" +
                                     FirebasePeopleStructureBuilder::getPersonDataPath(person.getId());

            if (m_firebaseClient->updateData(personPath, personJson))
            {
                peopleWritten++;
            }
        }

        if (peopleWritten > 0)
        {
            std::cout << "✅ Updated " << peopleWritten << " individual people records" << std::endl;
        }

        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error writing to Firebase: " << e.what() << std::endl;
        return false;
    }
}

std::vector<Person> QueueManager::getAllPeople() const
{
    std::vector<Person> allPeople;

    for (const auto &line : m_lines)
    {
        for (const auto &person : line)
        {
            allPeople.push_back(person);
        }
    }

    return allPeople;
}

std::vector<Person> QueueManager::getPeopleInLine(int lineNumber) const
{
    std::vector<Person> people;

    if (!isValidLineNumber(lineNumber))
    {
        return people; // Return empty vector for invalid line
    }

    const auto &line = m_lines[lineNumber - 1];
    for (const auto &person : line)
    {
        people.push_back(person);
    }

    return people;
}

FirebasePeopleStructureBuilder::PeopleSummary QueueManager::getCumulativePeopleSummary() const
{
    int activePeople = m_totalPeople; // Current people in queue
    int completedPeople = m_completedPeopleEver;
    int totalPeople = m_totalPeopleEver;

    double averageExpectedWait = totalPeople > 0 ? m_totalExpectedWaitTime / totalPeople : 0.0;
    double averageActualWait = completedPeople > 0 ? m_totalActualWaitTime / completedPeople : 0.0;

    return FirebasePeopleStructureBuilder::PeopleSummary(
        totalPeople, activePeople, completedPeople, averageExpectedWait, averageActualWait);
}

// History management methods for offline functionality
void QueueManager::addPersonToHistory(const Person& person)
{
    // Clean old entries before adding new one
    cleanOldHistoryEntries();
    
    // Add the new person to history
    m_lastHourHistory.push_back(person);
}

void QueueManager::cleanOldHistoryEntries()
{
    long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    long long oneHourAgo = currentTime - ONE_HOUR_MS;
    
    // Remove entries older than one hour
    m_lastHourHistory.erase(
        std::remove_if(m_lastHourHistory.begin(), m_lastHourHistory.end(),
            [oneHourAgo](const Person& person) {
                return person.getEnteringTimestamp() < oneHourAgo;
            }),
        m_lastHourHistory.end()
    );
}

std::vector<Person> QueueManager::getPeopleFromLastHour() const
{
    // Return a copy of the last hour history
    return m_lastHourHistory;
}

bool QueueManager::writeHistoryToFirebase()
{
    if (!m_firebaseClient)
    {
        std::cerr << "❌ No Firebase client configured for history upload" << std::endl;
        return false;
    }

    try
    {
        std::cout << "📤 Uploading " << m_lastHourHistory.size() << " people from last hour to cloud..." << std::endl;
        
        int successCount = 0;
        int totalCount = static_cast<int>(m_lastHourHistory.size());
        
        // Upload each person from the history
        for (const auto& person : m_lastHourHistory)
        {
            FirebasePeopleStructureBuilder::PersonData personData(person);
            std::string personJson = FirebasePeopleStructureBuilder::generatePersonDataJson(personData);
            std::string personPath = "simulation" + m_strategyPrefix + "/" +
                                   FirebasePeopleStructureBuilder::getPersonDataPath(person.getId());

            if (m_firebaseClient->updateData(personPath, personJson))
            {
                successCount++;
            }
            else
            {
                std::cerr << "❌ Failed to upload person " << person.getId() << " to Firebase" << std::endl;
            }
        }

        // Update summary with historical data
        FirebasePeopleStructureBuilder::PeopleSummary summary = getCumulativePeopleSummary();
        std::string summaryJson = FirebasePeopleStructureBuilder::generatePeopleSummaryJson(summary);
        std::string summaryPath = "simulation" + m_strategyPrefix + "/" +
                                FirebasePeopleStructureBuilder::getPeopleSummaryPath();

        bool summarySuccess = m_firebaseClient->updateData(summaryPath, summaryJson);
        
        if (summarySuccess)
        {
            std::cout << "✅ Successfully uploaded " << successCount << "/" << totalCount 
                      << " people and updated summary to cloud" << std::endl;
        }
        else
        {
            std::cout << "⚠️  Uploaded " << successCount << "/" << totalCount 
                      << " people but failed to update summary" << std::endl;
        }

        return (successCount == totalCount) && summarySuccess;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Error uploading history to Firebase: " << e.what() << std::endl;
        return false;
    }
}

bool QueueManager::updateAllAndCleanHistory()
{
    std::cout << "🔄 Starting offline data synchronization..." << std::endl;
    
    // Clean old entries first
    cleanOldHistoryEntries();
    
    if (m_lastHourHistory.empty())
    {
        std::cout << "ℹ️  No historical data from the last hour to upload" << std::endl;
        return true; // Nothing to do, but not an error
    }
    
    // Upload all historical data to Firebase
    bool historyUploadSuccess = writeHistoryToFirebase();
    
    if (historyUploadSuccess)
    {
        // Clear the history after successful upload
        size_t clearedCount = m_lastHourHistory.size();
        m_lastHourHistory.clear();
        
        std::cout << "✅ Successfully synchronized and cleared " << clearedCount 
                  << " historical entries" << std::endl;
        
        // Also update current state to Firebase
        bool currentStateSuccess = writeToFirebase();
        
        if (currentStateSuccess)
        {
            std::cout << "✅ Current queue state also synchronized to cloud" << std::endl;
            return true;
        }
        else
        {
            std::cout << "⚠️  Historical data uploaded but current state sync failed" << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "❌ Failed to upload historical data - keeping local history for retry" << std::endl;
        return false;
    }
}