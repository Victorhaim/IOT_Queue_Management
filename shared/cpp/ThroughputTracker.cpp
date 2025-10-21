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

    // Calculate throughput using total session time and service count
    auto totalSessionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                currentTime - sessionStartTime)
                                .count();

    if (totalSessionTime > 0)
    {
        currentThroughput = static_cast<double>(serviceCompletionCount) / (totalSessionTime / 1000.0);
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
    // Return true if we have at least the minimum required service completions for this line
    return serviceCompletionCount >= MIN_SERVICES_FOR_RELIABLE_DATA;
}