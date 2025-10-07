#include "FirebaseStructureBuilder.h"

std::string FirebaseStructureBuilder::generateLineDataJson(const LineData &lineData)
{
    std::ostringstream json;
    json << "{\n";
    json << "    \"currentOccupancy\": " << lineData.currentOccupancy << ",\n";
    json << "    \"throughputFactor\": " << std::fixed << std::setprecision(4) << lineData.throughputFactor << ",\n";
    json << "    \"averageWaitTime\": " << std::fixed << std::setprecision(2) << lineData.averageWaitTime << ",\n";
    json << "    \"lastUpdated\": \"" << getCurrentTimestamp() << "\",\n";
    json << "    \"lineNumber\": " << lineData.lineNumber << "\n";
    json << "}";
    return json.str();
}

std::string FirebaseStructureBuilder::generateAggregatedDataJson(const AggregatedData &aggData)
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"totalPeople\": " << aggData.totalPeople << ",\n";
    json << "  \"numberOfLines\": " << aggData.numberOfLines << ",\n";
    json << "  \"recommendedLine\": " << aggData.recommendedLine << "\n";
    json << "}";
    return json.str();
}

std::string FirebaseStructureBuilder::getLineDataPath(int lineNumber)
{
    return "queues/line" + std::to_string(lineNumber);
}

std::string FirebaseStructureBuilder::getAggregatedDataPath()
{
    return "currentBest";
}

int FirebaseStructureBuilder::calculateRecommendedLine(const std::vector<LineData> &allLines)
{
    if (allLines.empty())
    {
        return 0;
    }

    int recommendedLine = 1;
    double bestScore = allLines[0].averageWaitTime; // Lower wait time is better

    for (size_t i = 0; i < allLines.size(); ++i)
    {
        // Use average wait time as the primary metric for recommendation
        double currentScore = allLines[i].averageWaitTime;

        // If wait times are very close, prefer line with fewer people
        if (std::abs(currentScore - bestScore) < 0.5)
        {
            if (allLines[i].currentOccupancy < allLines[recommendedLine - 1].currentOccupancy)
            {
                recommendedLine = allLines[i].lineNumber;
                bestScore = currentScore;
            }
        }
        else if (currentScore < bestScore)
        {
            recommendedLine = allLines[i].lineNumber;
            bestScore = currentScore;
        }
    }

    return recommendedLine;
}

int FirebaseStructureBuilder::calculateRecommendedLine(const LineData *allLines, int numberOfLines)
{
    if (numberOfLines <= 0)
    {
        return 0;
    }

    int recommendedLine = 1;
    double bestScore = allLines[0].averageWaitTime; // Lower wait time is better

    for (int i = 0; i < numberOfLines; i++)
    {
        // Use average wait time as the primary metric for recommendation
        double currentScore = allLines[i].averageWaitTime;

        // If wait times are very close, prefer line with fewer people
        if (std::abs(currentScore - bestScore) < 0.5)
        {
            if (allLines[i].currentOccupancy < allLines[recommendedLine - 1].currentOccupancy)
            {
                recommendedLine = allLines[i].lineNumber;
                bestScore = currentScore;
            }
        }
        else if (currentScore < bestScore)
        {
            recommendedLine = allLines[i].lineNumber;
            bestScore = currentScore;
        }
    }

    return recommendedLine;
}

double FirebaseStructureBuilder::calculateAverageWaitTime(int occupancy, double throughputFactor)
{
    return (throughputFactor > 0.0) ? occupancy / throughputFactor : 0.0;
}

std::string FirebaseStructureBuilder::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}