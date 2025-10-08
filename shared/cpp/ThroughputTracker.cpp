#include "ThroughputTracker.h"
#include <algorithm>

ThroughputTracker::ThroughputTracker()
    : sessionStartTime(std::chrono::steady_clock::now()),
      lastServiceTime(sessionStartTime),
      serviceCompletionCount(0),
      currentThroughput(DEFAULT_THROUGHPUT),
      hasRecordedService(false)
{
}

void ThroughputTracker::recordServiceCompletion()
{
    auto currentTime = std::chrono::steady_clock::now();

    serviceCompletionCount++;
    lastServiceTime = currentTime;
    hasRecordedService = true;

    // Calculate throughput if we have enough data
    if (serviceCompletionCount >= MIN_SERVICES_FOR_CALCULATION)
    {
        auto totalSessionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    currentTime - sessionStartTime)
                                    .count();

        if (totalSessionTime > 0)
        {
            // Calculate people per second: total_services / time_in_seconds
            currentThroughput = static_cast<double>(serviceCompletionCount) / (totalSessionTime / 1000.0);

            // Reasonable bounds: 0.1 to 5.0 people per second
            currentThroughput = std::max(0.1, std::min(5.0, currentThroughput));
        }
    }
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
    currentThroughput = DEFAULT_THROUGHPUT;
    hasRecordedService = false;
}

bool ThroughputTracker::hasReliableData() const
{
    return serviceCompletionCount >= MIN_SERVICES_FOR_CALCULATION;
}