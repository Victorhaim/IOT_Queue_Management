#include "FirebaseStructureBuilder.h"

std::string FirebaseStructureBuilder::generateLineDataJson(const LineData &lineData)
{
    std::ostringstream json;
    json << "{\n";
    json << "    \"queueLength\": " << lineData.queueLength << ",\n";
    json << "    \"serviceRatePeoplePerSec\": " << std::fixed << std::setprecision(4) << lineData.serviceRatePeoplePerSec << ",\n";
    json << "    \"estimatedWaitForNewPerson\": " << std::fixed << std::setprecision(2) << lineData.estimatedWaitForNewPerson << ",\n";
    json << "    \"lastUpdated\": \"" << getCurrentTimestamp() << "\",\n";
    json << "    \"lineNumber\": " << lineData.lineNumber << "\n";
    json << "}";
    return json.str();
}

std::string FirebaseStructureBuilder::generateAggregatedDataJson(const AggregatedData &aggData)
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"totalPeopleInAllQueues\": " << aggData.totalPeopleInAllQueues << ",\n";
    json << "  \"numberOfLines\": " << aggData.numberOfLines << ",\n";
    json << "  \"recommendedLine\": " << aggData.recommendedLine << ",\n";
    json << "  \"recommendedLineEstWaitTime\": " << std::fixed << std::setprecision(2) << aggData.recommendedLineEstWaitTime << ",\n";
    json << "  \"recommendedLineQueueLength\": " << aggData.recommendedLineQueueLength << "\n";
    json << "}";
    return json.str();
}

std::string FirebaseStructureBuilder::getLineDataPath(int lineNumber)
{
    return "queues/line" + std::to_string(lineNumber);
}

std::string FirebaseStructureBuilder::getAggregatedDataPath()
{
    return "recommendedChoice";
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

FirebaseStructureBuilder::AggregatedData FirebaseStructureBuilder::createAggregatedData(const LineData *allLines, int totalPeople, int numberOfLines, int recommendedLine)
{
    if (numberOfLines <= 0)
    {
        return AggregatedData(totalPeople, numberOfLines, 0, 0.0, 0);
    }

    // Find the recommended line data to duplicate wait time and occupancy
    double recommendedWaitTime = 0.0;
    int recommendedOccupancy = 0;
    for (int i = 0; i < numberOfLines; i++)
    {
        if (allLines[i].lineNumber == recommendedLine)
        {
            recommendedWaitTime = allLines[i].estimatedWaitForNewPerson;
            recommendedOccupancy = allLines[i].queueLength;
            break;
        }
    }

    return AggregatedData(totalPeople, numberOfLines, recommendedLine, recommendedWaitTime, recommendedOccupancy);
}