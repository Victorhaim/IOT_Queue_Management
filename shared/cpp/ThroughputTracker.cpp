#include "ThroughputTracker.h"
#include <algorithm>

ThroughputTracker::ThroughputTracker(double expectedRate)
    : sessionStartTime(std::chrono::steady_clock::now()),
      lastServiceTime(sessionStartTime),
      serviceCompletionCount(0),
      currentThroughput(expectedRate),
      hasRecordedService(false),
      expectedServiceRate(expectedRate)
{
}

void ThroughputTracker::recordServiceCompletion()
{
    auto currentTime = std::chrono::steady_clock::now();

    serviceCompletionCount++;
    lastServiceTime = currentTime;
    hasRecordedService = true;

    // Calculate throughput using total session time and service count
    auto totalSessionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                currentTime - sessionStartTime)
                                .count();

    if (totalSessionTime > 0)
    {
        double observedThroughput = static_cast<double>(serviceCompletionCount) / (totalSessionTime / 1000.0);

        if (hasReliableData())
        {
            // Use observed rate when we have enough data
            currentThroughput = observedThroughput;
        }
        else
        {
            // Blend expected rate with observed rate for early measurements
            double blendFactor = static_cast<double>(serviceCompletionCount) / MIN_SERVICES_FOR_RELIABLE_DATA;
            currentThroughput = expectedServiceRate * (1.0 - blendFactor) + observedThroughput * blendFactor;
        }
    }
}

double ThroughputTracker::getEstimatedWaitTime(int queueLength, double arrivalRate) const
{
    if (queueLength == 0)
        return 0.0;

    // Basic M/M/1 calculation: Average wait time = n/μ (for people currently in queue)
    double basicWaitTime = static_cast<double>(queueLength) / currentThroughput;

    // Apply M/M/1 theory if arrival rate is provided and system is not saturated
    if (arrivalRate > 0.0 && isSystemStable(arrivalRate))
    {
        return applyMM1Theory(basicWaitTime, queueLength, arrivalRate);
    }

    return basicWaitTime;
}

double ThroughputTracker::applyMM1Theory(double basicWaitTime, int queueLength, double arrivalRate) const
{
    // M/M/1 Queue Theory:
    // ρ = λ/μ (utilization factor)
    // W_q = ρ/(μ-λ) = λ/(μ(μ-λ)) (average wait time in queue)
    // W_s = 1/μ (average service time)
    // W = W_q + W_s = 1/(μ-λ) (average time in system)

    double rho = arrivalRate / currentThroughput;

    if (rho >= 0.95) // Safety margin to prevent numerical issues
    {
        return basicWaitTime; // Fallback to basic calculation
    }

    // For M/M/1: Expected wait time in queue = ρ/(μ-λ) * (1/μ)
    double avgServiceTime = 1.0 / currentThroughput;
    double avgWaitTimeInQueue = (rho / (1.0 - rho)) * avgServiceTime;

    // Scale by current queue length vs theoretical average queue length
    double theoreticalAvgQueueLength = rho * rho / (1.0 - rho); // E[Nq] in M/M/1

    if (theoreticalAvgQueueLength > 0)
    {
        double scaleFactor = static_cast<double>(queueLength) / theoreticalAvgQueueLength;
        return avgWaitTimeInQueue * scaleFactor;
    }

    return basicWaitTime;
}

double ThroughputTracker::getUtilizationFactor(double arrivalRate) const
{
    if (currentThroughput <= 0.0)
        return 1.0; // Assume saturated if no throughput
    return arrivalRate / currentThroughput;
}

double ThroughputTracker::getCurrentThroughput() const
{
    return currentThroughput;
}

int ThroughputTracker::getServiceCount() const
{
    return serviceCompletionCount;
}

double ThroughputTracker::getSessionTimeSeconds() const
{
    auto currentTime = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                         currentTime - sessionStartTime)
                         .count();
    return totalTime / 1000.0;
}

void ThroughputTracker::reset()
{
    sessionStartTime = std::chrono::steady_clock::now();
    lastServiceTime = sessionStartTime;
    serviceCompletionCount = 0;
    currentThroughput = expectedServiceRate;
    hasRecordedService = false;
}

bool ThroughputTracker::hasReliableData() const
{
    // Return true if we have at least the minimum required service completions for this line
    return serviceCompletionCount >= MIN_SERVICES_FOR_RELIABLE_DATA;
}

bool ThroughputTracker::isSystemStable(double arrivalRate) const
{
    return getUtilizationFactor(arrivalRate) < 1.0;
}