#include "ThroughputTracker.h"
#include <algorithm>

ThroughputTracker::ThroughputTracker()
    : sessionStartTime(std::chrono::steady_clock::now()),
      lastServiceTime(sessionStartTime),
      serviceCompletionCount(0),
      currentThroughput(DEFAULT_THROUGHPUT),
      hasRecordedService(false)
{
    recentServices.reserve(MAX_WINDOW_SIZE);
}

void ThroughputTracker::recordServiceCompletion()
{
    auto currentTime = std::chrono::steady_clock::now();

    serviceCompletionCount++;
    lastServiceTime = currentTime;
    hasRecordedService = true;

    // Add to sliding window
    recentServices.push_back(currentTime);
    
    // Remove old services beyond window size
    if (recentServices.size() > MAX_WINDOW_SIZE) {
        recentServices.erase(recentServices.begin());
    }
    
    // Remove services older than time window
    auto cutoffTime = currentTime - std::chrono::seconds(static_cast<long>(WINDOW_TIME_SECONDS));
    recentServices.erase(
        std::remove_if(recentServices.begin(), recentServices.end(),
            [cutoffTime](const std::chrono::steady_clock::time_point& t) {
                return t < cutoffTime;
            }),
        recentServices.end()
    );

    // Calculate throughput using sliding window if we have enough recent data
    if (recentServices.size() >= MIN_SERVICES_FOR_CALCULATION)
    {
        auto windowStart = recentServices.front();
        auto windowEnd = recentServices.back();
        auto windowDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    windowEnd - windowStart).count();

        if (windowDuration > 0)
        {
            // Services per second = (services - 1) / time_span
            // We subtract 1 because N services create N-1 intervals
            double windowThroughput = static_cast<double>(recentServices.size() - 1) / (windowDuration / 1000.0);
            
            // Apply smoothing: blend old and new throughput
            const double SMOOTHING_FACTOR = 0.3; // How much to weight new data
            currentThroughput = (1.0 - SMOOTHING_FACTOR) * currentThroughput + 
                               SMOOTHING_FACTOR * windowThroughput;
        }
    }
    else if (serviceCompletionCount >= MIN_SERVICES_FOR_CALCULATION)
    {
        // Fallback to original calculation for early stages
        auto totalSessionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    currentTime - sessionStartTime).count();

        if (totalSessionTime > 0)
        {
            currentThroughput = static_cast<double>(serviceCompletionCount) / (totalSessionTime / 1000.0);
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
    recentServices.clear();
}

bool ThroughputTracker::hasReliableData() const
{
    // Require either enough recent services OR enough total services
    return recentServices.size() >= MIN_SERVICES_FOR_CALCULATION || 
           serviceCompletionCount >= MIN_SERVICES_FOR_CALCULATION;
}