#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include <algorithm>

/// Shared QueueManager implementation for both ESP32 and Flutter (via FFI)
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

    // FFI-friendly functions (C-style interface for Flutter)
    int* getLineCountsArray() const;  // Returns array of line counts
    void updateFromArray(const int* lineCounts, int arraySize);

private:
    static const int MAX_LINES = 10; // Reasonable limit for ESP32 and most use cases

    int m_maxSize;
    int m_numberOfLines;
    int m_totalPeople;
    int m_lines[MAX_LINES]; // Array to store people count for each line (1-based indexing)
    mutable int m_lineCountsBuffer[MAX_LINES]; // Buffer for FFI returns

    // Helper methods
    bool isValidLineNumber(int lineNumber) const;
    void updateTotalPeople();
};

// C-style interface for FFI (Flutter Foreign Function Interface)
extern "C" {
    // Factory functions
    QueueManager* queue_manager_create(int maxSize, int numberOfLines);
    void queue_manager_destroy(QueueManager* qm);

    // Core operations
    bool queue_manager_enqueue(QueueManager* qm);
    bool queue_manager_dequeue(QueueManager* qm, int lineNumber);
    bool queue_manager_enqueue_on_line(QueueManager* qm, int lineNumber);

    // State queries
    int queue_manager_size(QueueManager* qm);
    bool queue_manager_is_empty(QueueManager* qm);
    bool queue_manager_is_full(QueueManager* qm);
    int queue_manager_get_next_line_number(QueueManager* qm);
    int queue_manager_get_number_of_lines(QueueManager* qm);
    int queue_manager_get_line_count(QueueManager* qm, int lineNumber);

    // State modifications
    void queue_manager_set_line_count(QueueManager* qm, int lineNumber, int count);
    void queue_manager_reset(QueueManager* qm);

    // Array-based operations for FFI
    const int* queue_manager_get_line_counts_array(QueueManager* qm);
    void queue_manager_update_from_array(QueueManager* qm, const int* lineCounts, int arraySize);
}

#endif // QUEUE_MANAGER_H