#include "QueueManager.h"
#include "FirebaseClient.h"
#include "FirebaseStructureBuilder.h"
#include "ThroughputTracker.h"
#include <cstring>
#include <iostream>
#include <iomanip>

// Implementation updated to use std::vector for line storage instead of fixed-size C array.

QueueManager::QueueManager(int maxSize, int numberOfLines, const std::string& strategyPrefix, const std::string& appName)
    : m_maxSize(maxSize), m_numberOfLines(numberOfLines), m_totalPeople(0), m_lines(), m_lineThroughputs(),
      m_firebaseClient(nullptr), m_strategyPrefix(strategyPrefix), m_throughputTrackers(nullptr)
{
    if (m_numberOfLines < 0)
        m_numberOfLines = 0;
    if (m_numberOfLines > MAX_LINES)
        m_numberOfLines = MAX_LINES;    // enforce historical cap
    m_lines.reserve(m_numberOfLines);   // reserve capacity to avoid reallocations
    m_lines.assign(m_numberOfLines, 0); // initialize with zeros
    
    // Initialize throughput tracking for each line
    m_lineThroughputs.reserve(m_numberOfLines);
    m_lineThroughputs.assign(m_numberOfLines, DEFAULT_THROUGHPUT);

    // Initialize Firebase client with provided app name
    m_firebaseClient = std::make_shared<FirebaseClient>(
        appName,
        "https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app"
    );

    // Initialize Firebase client
    if (!m_firebaseClient->initialize())
    {
        std::cerr << "Failed to initialize Firebase client for " << appName << "!" << std::endl;
        m_firebaseClient = nullptr; // Disable cloud functionality
    }
    else
    {
        std::cout << "Firebase client initialized successfully for " << appName << std::endl;
    }
}

bool QueueManager::enqueue(LineSelectionStrategy strategy)
{
    if (isFull())
    {
        return false;
    }

    int lineNumber = getNextLineNumber(strategy);
    if (lineNumber == -1)
    {
        return false;
    }

    // External API uses 1-based line numbers; internal storage is 0-based
    m_lines[lineNumber - 1]++;
    m_totalPeople++;
    return true;
}

bool QueueManager::dequeue(int lineNumber)
{
    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    // Check if line has people
    if (m_lines[lineNumber - 1] == 0)
    {
        return false;
    }

    m_lines[lineNumber - 1]--;
    m_totalPeople--;
    return true;
}

bool QueueManager::enqueueOnLine(int lineNumber)
{
    if (isFull())
    {
        return false;
    }

    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    m_lines[lineNumber - 1]++;
    m_totalPeople++;
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

bool QueueManager::isFull() const
{
    if (m_maxSize == 0)
    {
        return false; // 0 means unlimited
    }
    return m_totalPeople >= m_maxSize;
}

int QueueManager::getNextLineNumber(LineSelectionStrategy strategy) const
{
    if (m_numberOfLines == 0)
    {
        return -1;
    }

    switch (strategy)
    {
    case LineSelectionStrategy::SHORTEST_WAIT_TIME:
        {
            // Current implementation: find line with shortest estimated wait time
            double minWaitTime = getEstimatedWaitTime(1);
            int bestLine = 1; // return value stays 1-based
            
            for (int i = 2; i <= m_numberOfLines; i++)
            {
                double waitTime = getEstimatedWaitTime(i);
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
            // Find line with fewest people
            int minPeople = m_lines[0];
            int bestLine = 1; // 1-based
            
            for (int i = 1; i < m_numberOfLines; i++)
            {
                if (m_lines[i] < minPeople)
                {
                    minPeople = m_lines[i];
                    bestLine = i + 1; // Convert to 1-based
                }
            }
            return bestLine;
        }
        
    case LineSelectionStrategy::FARTHEST_FROM_ENTRANCE:
        {
            // Find line where last person is farthest from entrance
            // Assume line numbers increase with distance from entrance
            // Choose the highest numbered line that has people, or highest if all empty
            int bestLine = m_numberOfLines; // Start with farthest line
            
            // If we want the line where the LAST person is farthest, 
            // we should prefer lines with people that are farther from entrance
            for (int i = m_numberOfLines; i >= 1; i--)
            {
                if (m_lines[i - 1] > 0) // This line has people
                {
                    bestLine = i;
                    break;
                }
            }
            
            // If no lines have people, choose the farthest line
            return bestLine;
        }
        
    default:
        // Fallback to shortest wait time
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

    return m_lines[lineNumber - 1];
}

void QueueManager::setLineCount(int lineNumber, int count)
{
    if (!isValidLineNumber(lineNumber))
    {
        return;
    }

    // Update total people count
    m_totalPeople -= m_lines[lineNumber - 1];
    m_lines[lineNumber - 1] = (count < 0) ? 0 : count;
    m_totalPeople += m_lines[lineNumber - 1];
}

void QueueManager::reset()
{
    for (int i = 0; i < m_numberOfLines; i++)
    {
        m_lines[i] = 0;
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
        m_totalPeople += m_lines[i];
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
    
    int peopleInLine = m_lines[lineNumber - 1];
    double throughput = m_lineThroughputs[lineNumber - 1];
    
    // Estimated wait time = number of people ahead / service rate
    // Add small penalty for being in line vs. empty line
    if (peopleInLine == 0)
    {
        return 0.0; // No wait for empty line
    }
    
    return static_cast<double>(peopleInLine) / throughput;
}

// Cloud integration methods
void QueueManager::setThroughputTrackers(std::vector<ThroughputTracker>* trackers)
{
    m_throughputTrackers = trackers;
}

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
            std::string queuePath = "queues" + m_strategyPrefix + "/line" + std::to_string(i);
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
        std::string aggPath = "currentBest" + m_strategyPrefix;
        if (m_firebaseClient->deleteData(aggPath))
        {
            std::cout << "✅ Successfully cleared " << aggPath << " data" << std::endl;
        }
        else
        {
            std::cout << "ℹ️  Note: No existing " << aggPath << " data found or failed to clear" << std::endl;
        }

        // Optional: Also clear the entire queues node to ensure a fresh start
        std::string queuesPath = "queues" + m_strategyPrefix;
        if (m_firebaseClient->deleteData(queuesPath))
        {
            std::cout << "✅ Successfully cleared all " 
                      << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ") 
                      << "queue data" << std::endl;
        }
        else
        {
            std::cout << "ℹ️  Note: No existing " 
                      << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ") 
                      << "queue data found or failed to clear all queues" << std::endl;
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
            
            // Use throughput from trackers if available, otherwise use internal throughput
            double throughputFactor = getLineThroughput(line);
            if (m_throughputTrackers && (line - 1) < static_cast<int>(m_throughputTrackers->size())) 
            {
                throughputFactor = (*m_throughputTrackers)[line - 1].getCurrentThroughput();
            }
            
            double averageWaitTime = FirebaseStructureBuilder::calculateAverageWaitTime(
                currentOccupancy, throughputFactor);

            totalPeople += currentOccupancy;

            // Create line data object
            FirebaseStructureBuilder::LineData lineData(
                currentOccupancy, throughputFactor, averageWaitTime, line);
            allLinesData.push_back(lineData);

            // Generate JSON and write to Firebase for this specific line
            std::string json = FirebaseStructureBuilder::generateLineDataJson(lineData);
            std::string queuePath = "queues" + m_strategyPrefix + "/line" + std::to_string(line);

            if (m_firebaseClient->updateData(queuePath, json))
            {
                std::cout << "✅ " << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ") 
                          << "Line " << line << " updated - Occupancy: " << currentOccupancy
                          << ", Throughput: " << std::fixed << std::setprecision(3) << throughputFactor
                          << ", Avg Wait: " << std::fixed << std::setprecision(1) << averageWaitTime << "s";
                
                if (m_throughputTrackers && (line - 1) < static_cast<int>(m_throughputTrackers->size())) 
                {
                    std::cout << " [" << ((*m_throughputTrackers)[line - 1].hasReliableData() ? "measured" : "default") << "]";
                }
                std::cout << std::endl;
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
                FirebaseStructureBuilder::createAggregatedData(allLinesData.data(), totalPeople, static_cast<int>(allLinesData.size()));

            std::string aggJson = FirebaseStructureBuilder::generateAggregatedDataJson(aggData);
            std::string aggPath = "currentBest" + m_strategyPrefix;

            if (m_firebaseClient->updateData(aggPath, aggJson))
            {
                std::cout << "✅ Aggregated " << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ") 
                          << "queue object updated (currentBest" << m_strategyPrefix << ") totalPeople=" << totalPeople
                          << " recommendedLine=" << aggData.recommendedLine
                          << " waitTime=" << std::round(aggData.averageWaitTime) << "s"
                          << " placeInLine=" << aggData.currentOccupancy << std::endl;
                return true;
            }
            else
            {
                std::cerr << "❌ Failed to update aggregated " 
                          << (m_strategyPrefix.empty() ? "" : m_strategyPrefix.substr(1) + " ") 
                          << "queue object" << std::endl;
                return false;
            }
        }
        
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error writing to Firebase: " << e.what() << std::endl;
        return false;
    }
}