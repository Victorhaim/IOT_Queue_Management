#pragma once

#include <chrono>
#include <vector>
#include <cmath>

/**
 * Enhanced ThroughputTracker - M/M/1 Queue Theory Implementation
 *
 * This class implements proper M/M/1 queue theory for constant-rate systems:
 * - Tracks service completion events for rate estimation
 * - Applies Little's Law and M/M/1 formulas
 * - Provides stability analysis (ρ < 1)
 * - Optimized for constant service rates (simulation environment)
 *
 * Used by both QueueSimulator and ESP32 for consistent measurements
 */
class ThroughputTracker
{
private:
    // Time tracking
    std::chrono::steady_clock::time_point sessionStartTime;
    std::chrono::steady_clock::time_point lastServiceTime;
    
    // Service tracking
    int serviceCompletionCount;
    double currentThroughput;
    bool hasRecordedService;
    
    // Expected service rate for this line (set during initialization)
    double expectedServiceRate;
    
    // Configuration constants
    static constexpr double DEFAULT_THROUGHPUT = 0.1;        // people/second
    static constexpr int MIN_SERVICES_FOR_RELIABLE_DATA = 5; // Reduced for faster adaptation

public:
    /**
     * Constructor - Initialize tracking for M/M/1 queue
     * @param expectedRate Expected service rate for this line (μ in M/M/1 notation)
     */
    ThroughputTracker(double expectedRate = DEFAULT_THROUGHPUT);

    /**
     * Record a service completion event
     * Updates throughput calculation using cumulative average
     */
    void recordServiceCompletion();

    /**
     * Get current measured throughput (people per second)
     * Blends expected rate with observed rate for stability
     */
    double getCurrentThroughput() const;
    
    /**
     * Get estimated wait time using M/M/1 queue theory
     * @param queueLength Current number of people in queue (n)
     * @param arrivalRate Current arrival rate (λ)
     * @return Expected wait time in seconds using W = n/μ or M/M/1 formulas
     */
    double getEstimatedWaitTime(int queueLength, double arrivalRate = 0.0) const;

    /**
     * Get utilization factor (ρ = λ/μ)
     * Critical for M/M/1 stability analysis
     */
    double getUtilizationFactor(double arrivalRate) const;
    
    /**
     * Check if the system is stable (ρ < 1) for given arrival rate
     */
    bool isSystemStable(double arrivalRate) const;

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

private:
    /**
     * Apply M/M/1 queue theory for wait time calculation
     */
    double applyMM1Theory(double basicWaitTime, int queueLength, double arrivalRate) const;
};