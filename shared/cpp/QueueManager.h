#pragma once

#include <algorithm>
#include <vector>
#include <memory>
#include "FirebaseClient.h"
#include "ThroughputTracker.h"

/// Line selection strategies for queue management
enum class LineSelectionStrategy
{
    SHORTEST_WAIT_TIME,  // Current implementation: consider both queue length and throughput
    FEWEST_PEOPLE,       // Simply choose line with fewest people
    FARTHEST_FROM_ENTRANCE // Choose line where last person is farthest from entrance
};

/// Shared QueueManager implementation for ESP32 and simulation
/// Manages queue lines using smart line selection based on wait times
/// Considers both queue length and service throughput for optimal routing
class QueueManager
{
public:
    QueueManager(int maxSize, int numberOfLines);
    ~QueueManager() = default;

    // Core queue operations
    bool enqueue(LineSelectionStrategy strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME);
    bool dequeue(int lineNumber);
    bool enqueueOnLine(int lineNumber);

    // State queries
    int size() const;
    bool isEmpty() const;
    bool isFull() const;
    int getNextLineNumber(LineSelectionStrategy strategy = LineSelectionStrategy::SHORTEST_WAIT_TIME) const;
    int getNumberOfLines() const;
    int getLineCount(int lineNumber) const;

    // Throughput management
    void updateLineThroughput(int lineNumber, double throughputPerSecond);
    double getLineThroughput(int lineNumber) const;
    double getEstimatedWaitTime(int lineNumber) const;

    // Cloud integration (optional)
    void setFirebaseClient(std::shared_ptr<FirebaseClient> client);
    void setStrategyPrefix(const std::string& prefix);
    bool writeToFirebase();
    void setThroughputTrackers(std::vector<ThroughputTracker>* trackers);

    // State modifications
    void setLineCount(int lineNumber, int count);
    void reset();

private:
    static const int MAX_LINES = 10; // Historical cap; still enforced to avoid runaway usage
    static constexpr double DEFAULT_THROUGHPUT = 0.5; // Default people/second when no data available

    int m_maxSize;
    int m_numberOfLines;
    int m_totalPeople;
    std::vector<int> m_lines; // 0-based indexing internally
    std::vector<double> m_lineThroughputs; // Throughput per line (people/second)

    // Optional Firebase integration
    std::shared_ptr<FirebaseClient> m_firebaseClient;
    std::string m_strategyPrefix; // e.g., "", "_shortest", "_farthest"
    std::vector<ThroughputTracker>* m_throughputTrackers; // External tracker reference

    // Helper methods
    bool isValidLineNumber(int lineNumber) const;
    void updateTotalPeople();
};
