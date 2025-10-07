#pragma once

#include <algorithm>
#include <vector>

/// Shared QueueManager implementation for ESP32 and simulation
/// Manages queue lines using a simple array-based approach suitable for microcontrollers
class QueueManager {
public:
    QueueManager(int maxSize, int numberOfLines);
    ~QueueManager() = default;

    // Core queue operations
    bool enqueue();
    bool dequeue(int lineNumber);
    bool enqueueOnLine(int lineNumber);

    // State queries
    int size() const;
    bool isEmpty() const;
    bool isFull() const;
    int getNextLineNumber() const;
    int getNumberOfLines() const;
    int getLineCount(int lineNumber) const;

    // State modifications
    void setLineCount(int lineNumber, int count);
    void reset();

private:
    static const int MAX_LINES = 10; // Historical cap; still enforced to avoid runaway usage

    int m_maxSize;
    int m_numberOfLines;
    int m_totalPeople;
    std::vector<int> m_lines; // 0-based indexing internally

    // Helper methods
    bool isValidLineNumber(int lineNumber) const;
    void updateTotalPeople();
};
