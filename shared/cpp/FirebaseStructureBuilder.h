#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

/**
 * FirebaseStructureBuilder - Unified class for creating consistent Firebase data structures
 *
 * This class ensures that both the simulation and ESP32 create the same Firebase structure:
 * - queues/line1, queues/line2, etc. (current conditions if you join this line now)
 * - recommendedChoice (which line the algorithm suggests with its current wait time)
 *
 * Works with both standard C++ and ESP32 environments
 */
class FirebaseStructureBuilder
{
public:
    struct LineData
    {
        int queueLength;
        double serviceRatePeoplePerSec;
        double estimatedWaitForNewPerson;
        int lineNumber;

        LineData(int occupancy, double throughput, double waitTime, int number)
            : queueLength(occupancy), serviceRatePeoplePerSec(throughput),
              estimatedWaitForNewPerson(waitTime), lineNumber(number) {}
    };

    struct AggregatedData
    {
        int totalPeopleInAllQueues;
        int numberOfLines;
        int recommendedLine;
        double recommendedLineEstWaitTime;
        int recommendedLineQueueLength;

        AggregatedData(int total, int numLines, int recommended, double waitTime = 0.0, int occupancy = 0)
            : totalPeopleInAllQueues(total), numberOfLines(numLines), recommendedLine(recommended),
              recommendedLineEstWaitTime(waitTime), recommendedLineQueueLength(occupancy) {}
    };

    /**
     * Generate JSON for queue line data (what you'd see if you joined this line right now)
     * Structure: { queueLength, serviceRatePeoplePerSec, estimatedWaitForNewPerson, lastUpdated, lineNumber }
     */
    static std::string generateLineDataJson(const LineData &lineData);

    /**
     * Generate JSON for recommended choice (which line the algorithm suggests)
     * Structure: { totalPeopleInAllQueues, numberOfLines, recommendedLine, recommendedLineEstWaitTime, recommendedLineQueueLength }
     */
    static std::string generateAggregatedDataJson(const AggregatedData &aggData);

    /**
     * Get the Firebase path for a specific line
     * Returns: "queues/line{lineNumber}"
     */
    static std::string getLineDataPath(int lineNumber);

    /**
     * Get the Firebase path for aggregated data
     * Returns: "recommendedChoice"
     */
    static std::string getAggregatedDataPath();

    /**
     * Create AggregatedData with recommended line and duplicated wait time/occupancy info
     * Uses the provided recommendedLine (from actual strategy selection)
     * Works with both C++ vectors (use .data()) and C-style arrays
     * @param recommendedLine The line that was actually chosen by the strategy
     */
    static AggregatedData createAggregatedData(const LineData *allLines, int totalPeople, int numberOfLines, int recommendedLine);

    /**
     * Get current timestamp as std::string
     */
    static std::string getCurrentTimestamp();
};