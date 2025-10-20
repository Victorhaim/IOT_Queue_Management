#pragma once

#include <chrono>
#include <vector>

/**
 * ThroughputTracker - Shared class for real-time throughput measurement
 *
 * This class measures actual service performance by tracking:
 * - Service completion events
 * - Time intervals between services
 * - Calculated throughput (people/second)
 *
 * Used by both QueueSimulator and ESP32 for consistent measurements
 */
class ThroughputTracker
{
private:
    std::chrono::steady_clock::time_point sessionStartTime;
    std::chrono::steady_clock::time_point lastServiceTime;
    int serviceCompletionCount;
    double currentThroughput;
    bool hasRecordedService;

    // Configuration
    static constexpr double DEFAULT_THROUGHPUT = 0.1; // people/second

public:
    /**
     * Constructor - Initialize tracking for a new session
     */
    ThroughputTracker();

    /**
     * Record a service completion event
     * Automatically updates throughput calculation
     */
    void recordServiceCompletion();

    /**
     * Get current measured throughput (people per second)
     * Returns default value if insufficient data available
     */
    double getCurrentThroughput() const;

    /**
     * Get number of services completed in this session
     */
    int getServiceCount() const;

    /**
     * Get total session time in seconds
     */
    double getSessionTimeSeconds() const;

    /**
     * Reset tracking (start new measurement session)
     */
    void reset();

    /**
     * Check if we have enough data for reliable throughput calculation
     */
    bool hasReliableData() const;
};