#ifndef FIREBASE_STRUCTURE_BUILDER_H
#define FIREBASE_STRUCTURE_BUILDER_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

/**
 * FirebaseStructureBuilder - Unified class for creating consistent Firebase data structures
 *
 * This class ensures that both the simulation and ESP32 create the same Firebase structure:
 * - queues/line1, queues/line2, etc. (individual line data with detailed metrics)
 * - currentBest (aggregated data with recommendations)
 *
 * Works with both standard C++ and ESP32 environments
 */
class FirebaseStructureBuilder
{
public:
    struct LineData
    {
        int currentOccupancy;
        double throughputFactor;
        double averageWaitTime;
        int lineNumber;

        LineData(int occupancy, double throughput, double waitTime, int number)
            : currentOccupancy(occupancy), throughputFactor(throughput),
              averageWaitTime(waitTime), lineNumber(number) {}
    };

    struct AggregatedData
    {
        int totalPeople;
        int numberOfLines;
        int recommendedLine;
        double averageWaitTime;
        int currentOccupancy;

        AggregatedData(int total, int numLines, int recommended, double waitTime = 0.0, int occupancy = 0)
            : totalPeople(total), numberOfLines(numLines), recommendedLine(recommended),
              averageWaitTime(waitTime), currentOccupancy(occupancy) {}
    };

    /**
     * Generate JSON for individual queue line data
     * Structure: { currentOccupancy, throughputFactor, averageWaitTime, lastUpdated, lineNumber }
     */
    static std::string generateLineDataJson(const LineData &lineData);

    /**
     * Generate JSON for aggregated queue data (currentBest)
     * Structure: { totalPeople, numberOfLines, recommendedLine }
     */
    static std::string generateAggregatedDataJson(const AggregatedData &aggData);

    /**
     * Get the Firebase path for a specific line
     * Returns: "queues/line{lineNumber}"
     */
    static std::string getLineDataPath(int lineNumber);

    /**
     * Get the Firebase path for aggregated data
     * Returns: "currentBest"
     */
    static std::string getAggregatedDataPath();

    /**
     * Calculate recommended line based on occupancy and wait times
     * Returns the line number with the shortest effective wait time
     * Works with both C++ vectors (use .data()) and C-style arrays
     */
    static int calculateRecommendedLine(const LineData *allLines, int numberOfLines);

    /**
     * Create AggregatedData with recommended line and duplicated wait time/occupancy info
     * This function automatically finds the recommended line and copies its data
     * Works with both C++ vectors (use .data()) and C-style arrays
     */
    static AggregatedData createAggregatedData(const LineData *allLines, int totalPeople, int numberOfLines);

    /**
     * Calculate average wait time based on occupancy and throughput
     * waitTime = occupancy / throughputFactor (if throughput > 0, else 0)
     */
    static double calculateAverageWaitTime(int occupancy, double throughputFactor);

    /**
     * Get current timestamp as std::string
     */
    static std::string getCurrentTimestamp();
};

#endif // FIREBASE_STRUCTURE_BUILDER_H