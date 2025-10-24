#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <mutex>
#include <vector>
#include <memory>
#include <queue>
#include <condition_variable>
#include "../shared/cpp/QueueManager.h"
#include "../shared/cpp/ThroughputTracker.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

// Event structure for coordinating between threads
struct SimulationEvent
{
    enum Type
    {
        ARRIVAL,
        SERVICE
    } type;
    int line = 0; // For service events
    std::chrono::steady_clock::time_point timestamp;

    SimulationEvent(Type t, int l = 0) : type(t), line(l), timestamp(std::chrono::steady_clock::now()) {}
};

enum class StrategyType
{
    FEWEST_PEOPLE,
    SHORTEST_WAIT_TIME,
    FARTHEST_FROM_ENTRANCE
};

// Structure to track line-specific actual wait time statistics
struct LineWaitStats
{
    double totalWaitTime = 0.0;
    int completedPeople = 0;
    double averageWaitTime = 0.0;

    void addCompletedPerson(double actualWaitTime)
    {
        totalWaitTime += actualWaitTime;
        completedPeople++;
        averageWaitTime = completedPeople > 0 ? totalWaitTime / completedPeople : 0.0;
    }
};

class StrategySimulator
{
private:
    std::unique_ptr<QueueManager> queueManager;
    StrategyType strategyType;
    std::string strategyName;
    std::string firestoreCollection;

    // Line-specific wait time tracking
    std::vector<LineWaitStats> lineWaitStats;

    // Configuration (shared across all strategies)
    const int maxQueueSize = 10000;
    const int numberOfLines = 3;
    const std::vector<double> serviceRates = {0.08, 0.12, 0.18};

public:
    StrategySimulator(StrategyType type, const std::string &name, const std::string &collection)
        : strategyType(type), strategyName(name), firestoreCollection(collection)
    {
        // Initialize line wait statistics
        lineWaitStats.resize(numberOfLines);

        std::string suffix;
        switch (type)
        {
        case StrategyType::FEWEST_PEOPLE:
            suffix = "_shortest";
            break;
        case StrategyType::SHORTEST_WAIT_TIME:
            suffix = "_project";
            break;
        case StrategyType::FARTHEST_FROM_ENTRANCE:
            suffix = "_farthest";
            break;
        }

        queueManager = std::make_unique<QueueManager>(maxQueueSize, numberOfLines, suffix, firestoreCollection);

        std::cout << "[" << strategyName << "] Initialized with " << numberOfLines
                  << " lines, max size per line: " << maxQueueSize << std::endl;
    }

    bool processArrival()
    {
        switch (strategyType)
        {
        case StrategyType::FEWEST_PEOPLE:
            return queueManager->enqueue(LineSelectionStrategy::FEWEST_PEOPLE);
        case StrategyType::SHORTEST_WAIT_TIME:
            return queueManager->enqueueAuto(); // Uses adaptive strategy
        case StrategyType::FARTHEST_FROM_ENTRANCE:
            return queueManager->enqueue(LineSelectionStrategy::FARTHEST_FROM_ENTRANCE);
        }
        return false;
    }

    bool processService(int line)
    {
        if (queueManager->getLineCount(line) > 0)
        {
            // Get the person being served before removing them
            auto peopleInLine = queueManager->getPeopleInLine(line);
            if (!peopleInLine.empty())
            {
                const auto &personBeingServed = peopleInLine.front();
                if (personBeingServed.hasExited())
                {
                    // Track actual wait time for this line
                    double actualWaitTime = personBeingServed.getActualWaitTime();
                    lineWaitStats[line - 1].addCompletedPerson(actualWaitTime);
                }
            }

            // Use appropriate dequeue method for this simulator type
            bool success = false;
            switch (strategyType)
            {
            case StrategyType::FEWEST_PEOPLE:
                success = queueManager->dequeue(line, LineSelectionStrategy::FEWEST_PEOPLE);
                break;
            case StrategyType::SHORTEST_WAIT_TIME:
                success = queueManager->dequeueAuto(line); // Uses adaptive strategy
                break;
            case StrategyType::FARTHEST_FROM_ENTRANCE:
                success = queueManager->dequeue(line, LineSelectionStrategy::FARTHEST_FROM_ENTRANCE);
                break;
            }
            return success;
        }
        return false;
    }

    int getLineCount(int line) const
    {
        return queueManager->getLineCount(line);
    }

    int getTotalSize() const
    {
        return queueManager->size();
    }

    double getEstimatedWaitTime(int line) const
    {
        return queueManager->getEstimatedWaitTime(line);
    }

    int getNextLineNumber() const
    {
        switch (strategyType)
        {
        case StrategyType::FEWEST_PEOPLE:
            return queueManager->getNextLineNumber(LineSelectionStrategy::FEWEST_PEOPLE);
        case StrategyType::SHORTEST_WAIT_TIME:
            // Determine which strategy is currently being used
            return queueManager->getNextLineNumber(
                (queueManager->getCumulativePeopleSummary().completedPeople >= 30)
                    ? LineSelectionStrategy::SHORTEST_WAIT_TIME
                    : LineSelectionStrategy::FEWEST_PEOPLE);
        case StrategyType::FARTHEST_FROM_ENTRANCE:
            return queueManager->getNextLineNumber(LineSelectionStrategy::FARTHEST_FROM_ENTRANCE);
        }
        return 1;
    }

    const std::string &getName() const
    {
        return strategyName;
    }

    std::string getCurrentStrategyDescription() const
    {
        if (strategyType == StrategyType::SHORTEST_WAIT_TIME)
        {
            bool usingShortestWait = queueManager->getCumulativePeopleSummary().completedPeople >= 30;
            return usingShortestWait ? "SHORTEST_WAIT_TIME" : "FEWEST_PEOPLE (adaptive)";
        }
        return strategyName;
    }

    // New methods for JSON output
    std::vector<Person> getAllPeople() const
    {
        return queueManager->getAllPeople();
    }

    FirebasePeopleStructureBuilder::PeopleSummary getCumulativePeopleSummary() const
    {
        return queueManager->getCumulativePeopleSummary();
    }

    LineWaitStats getLineActualWaitStats(int line) const
    {
        if (line >= 1 && line <= numberOfLines)
        {
            return lineWaitStats[line - 1];
        }
        return LineWaitStats(); // Return empty stats for invalid line
    }
};

// JSON output structure that mirrors Firebase
struct JSONOutputManager
{
    std::string outputDirectory;

    JSONOutputManager(const std::string &dir) : outputDirectory(dir)
    {
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outputDirectory);
    }

    void writeUnifiedStrategyData(const std::vector<std::unique_ptr<StrategySimulator>> &simulators)
    {
        std::string filename = outputDirectory + "/unified_simulation_data.json";
        std::ofstream file(filename);

        file << "{\n";
        file << "  \"timestamp\": \"" << getCurrentTimestamp() << "\",\n";
        file << "  \"strategies\": {\n";

        bool firstStrategy = true;
        for (const auto &simulator : simulators)
        {
            if (!firstStrategy)
                file << ",\n";
            firstStrategy = false;

            std::string strategyKey;
            if (simulator->getName() == "FEWEST_PEOPLE")
                strategyKey = "shortest";
            else if (simulator->getName() == "SHORTEST_WAIT_TIME")
                strategyKey = "project";
            else if (simulator->getName() == "FARTHEST_FROM_ENTRANCE")
                strategyKey = "farthest";

            file << "    \"" << strategyKey << "\": {\n";

            // Overall stats
            auto summary = simulator->getCumulativePeopleSummary();
            file << "      \"overallStats\": {\n";
            file << "        \"totalPeople\": " << summary.totalPeople << ",\n";
            file << "        \"activePeople\": " << summary.activePeople << ",\n";
            file << "        \"completedPeople\": " << summary.completedPeople << ",\n";
            file << "        \"historicalAvgExpectedWait\": " << std::fixed << std::setprecision(2)
                 << summary.historicalAvgExpectedWait << ",\n";
            file << "        \"historicalAvgActualWait\": " << std::fixed << std::setprecision(2)
                 << summary.historicalAvgActualWait << "\n";
            file << "      },\n";

            // Line stats
            file << "      \"lineStats\": {\n";
            for (int line = 1; line <= 3; ++line)
            {
                if (line > 1)
                    file << ",\n";

                auto lineActualWaitStats = simulator->getLineActualWaitStats(line);

                file << "        \"line" << line << "\": {\n";
                file << "          \"currentPeople\": " << simulator->getLineCount(line) << ",\n";
                file << "          \"estimatedWaitTime\": " << std::fixed << std::setprecision(2)
                     << simulator->getEstimatedWaitTime(line) << ",\n";
                file << "          \"actualAvgWaitTime\": " << std::fixed << std::setprecision(2)
                     << lineActualWaitStats.averageWaitTime << ",\n";
                file << "          \"totalCompletedInLine\": " << lineActualWaitStats.completedPeople << ",\n";
                file << "          \"totalActualWaitTime\": " << std::fixed << std::setprecision(2)
                     << lineActualWaitStats.totalWaitTime << "\n";
                file << "        }";
            }
            file << "\n      },\n";

            // People data
            auto allPeople = simulator->getAllPeople();
            file << "      \"people\": {\n";
            bool firstPerson = true;
            for (const auto &person : allPeople)
            {
                if (!firstPerson)
                    file << ",\n";
                firstPerson = false;

                file << "        \"" << person.getId() << "\": {\n";
                file << "          \"personId\": \"" << person.getId() << "\",\n";
                file << "          \"expectedWaitTime\": " << std::fixed << std::setprecision(2)
                     << person.getExpectedWaitTime() << ",\n";
                file << "          \"enteringTimestamp\": " << person.getEnteringTimestamp() << ",\n";
                file << "          \"exitingTimestamp\": " << person.getExitingTimestamp() << ",\n";
                file << "          \"lineNumber\": " << person.getLineNumber() << ",\n";
                file << "          \"actualWaitTime\": " << std::fixed << std::setprecision(2)
                     << person.getActualWaitTime() << ",\n";
                file << "          \"hasExited\": " << (person.hasExited() ? "true" : "false") << "\n";
                file << "        }";
            }
            file << "\n      }\n";
            file << "    }";
        }

        file << "\n  }\n";
        file << "}";
        file.close();
    }

private:
    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
        return oss.str();
    }
};

class UnifiedQueueSimulator
{
private:
    // Configuration (shared across all strategies)
    const int maxQueueSize = 50;
    const int numberOfLines = 3;
    const double arrivalRate = 0.5;
    const std::vector<double> serviceRates = {0.08, 0.12, 0.18};
    const std::chrono::milliseconds updateInterval{2000};
    const std::chrono::seconds jsonOutputInterval{30}; // Output JSON every 30 seconds

    // Simulators for each strategy
    std::vector<std::unique_ptr<StrategySimulator>> simulators;

    // JSON output manager
    std::unique_ptr<JSONOutputManager> jsonManager;

    // Shared random number generation (ensures same scenarios for all strategies)
    std::mt19937 rng;
    std::uniform_real_distribution<double> arrivalDist;
    std::uniform_real_distribution<double> serviceDist;

    std::atomic<bool> running{false};

    // Multithreading components
    std::thread eventGeneratorThread;

    // Event coordination - single threaded synchronous processing
    std::mutex eventMutex;

    // Output synchronization
    std::mutex outputMutex;

public:
    UnifiedQueueSimulator() : rng(std::chrono::steady_clock::now().time_since_epoch().count()),
                              arrivalDist(0.0, 1.0),
                              serviceDist(0.0, 1.0)
    {
        // Initialize JSON output manager
        std::string outputDir = "simulation_output";
        jsonManager = std::make_unique<JSONOutputManager>(outputDir);

        // Create simulators for all three strategies
        simulators.emplace_back(std::make_unique<StrategySimulator>(
            StrategyType::FEWEST_PEOPLE,
            "FEWEST_PEOPLE",
            "iot-queue-management-shortest"));

        simulators.emplace_back(std::make_unique<StrategySimulator>(
            StrategyType::SHORTEST_WAIT_TIME,
            "SHORTEST_WAIT_TIME",
            "iot-queue-management"));

        simulators.emplace_back(std::make_unique<StrategySimulator>(
            StrategyType::FARTHEST_FROM_ENTRANCE,
            "FARTHEST_FROM_ENTRANCE",
            "iot-queue-management-farthest"));

        std::cout << "\n=== UNIFIED QUEUE SIMULATOR ===" << std::endl;
        std::cout << "Running " << simulators.size() << " strategies simultaneously:" << std::endl;
        for (const auto &sim : simulators)
        {
            std::cout << "  - " << sim->getName() << std::endl;
        }
        std::cout << "\nShared Configuration:" << std::endl;
        std::cout << "  Max queue size per line: " << maxQueueSize << std::endl;
        std::cout << "  Number of lines: " << numberOfLines << std::endl;
        std::cout << "  Arrival rate: " << arrivalRate << std::endl;
        std::cout << "  Service rates: ";
        for (size_t i = 0; i < serviceRates.size(); ++i)
        {
            std::cout << serviceRates[i];
            if (i < serviceRates.size() - 1)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Update interval: " << updateInterval.count() << "ms" << std::endl;
        std::cout << "  JSON output directory: " << outputDir << std::endl;
        std::cout << "  JSON output interval: " << jsonOutputInterval.count() << "s" << std::endl;
        std::cout << "================================" << std::endl;
    }

    ~UnifiedQueueSimulator()
    {
        stop();
    }

    void start()
    {
        if (running.load())
        {
            std::cout << "Simulation already running!" << std::endl;
            return;
        }

        running.store(true);
        std::cout << "Starting synchronous unified queue simulation..." << std::endl;

        // Start event generator thread (single-threaded event processing)
        eventGeneratorThread = std::thread([this]()
                                           { generateAndProcessEvents(); });
    }

    void stop()
    {
        if (running.load())
        {
            std::cout << "Stopping simulation..." << std::endl;
            running.store(false);

            // Join event generator thread
            if (eventGeneratorThread.joinable())
            {
                eventGeneratorThread.join();
            }

            std::cout << "Simulation stopped." << std::endl;
        }
    }

private:
    void generateAndProcessEvents()
    {
        std::cout << "Event generator and processor thread started" << std::endl;

        auto lastStatsTime = std::chrono::steady_clock::now();
        auto lastJsonTime = std::chrono::steady_clock::now();

        while (running.load())
        {
            std::vector<SimulationEvent> eventsToProcess;

            // Generate arrival events
            if (arrivalDist(rng) < arrivalRate)
            {
                eventsToProcess.emplace_back(SimulationEvent::ARRIVAL);
            }

            // Generate service events for each line
            for (int line = 1; line <= numberOfLines; ++line)
            {
                if (serviceDist(rng) < serviceRates[line - 1])
                {
                    eventsToProcess.emplace_back(SimulationEvent::SERVICE, line);
                }
            }

            // Process each event for ALL strategies synchronously
            for (const auto &event : eventsToProcess)
            {
                for (size_t strategyIndex = 0; strategyIndex < simulators.size(); ++strategyIndex)
                {
                    processEventForStrategy(strategyIndex, event);
                }
            }

            auto now = std::chrono::steady_clock::now();

            // Print periodic stats (every 20 seconds)
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime).count() >= 20)
            {
                for (const auto &simulator : simulators)
                {
                    printStrategyStatistics(simulator.get());
                }
                lastStatsTime = now;
            }

            // Write JSON output periodically
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastJsonTime) >= jsonOutputInterval)
            {
                writeJSONOutput();
                lastJsonTime = now;
            }

            // Wait before generating next batch of events
            std::this_thread::sleep_for(updateInterval);
        }

        // Write final JSON output when stopping
        writeJSONOutput();
        std::cout << "Event generator and processor thread stopped" << std::endl;
    }

    void processEventForStrategy(size_t strategyIndex, const SimulationEvent &event)
    {
        auto &simulator = simulators[strategyIndex];

        if (event.type == SimulationEvent::ARRIVAL)
        {
            if (simulator->processArrival())
            {
                int selectedLine = simulator->getNextLineNumber();

                std::lock_guard<std::mutex> outputLock(outputMutex);
                std::cout << "[" << simulator->getName() << "] ARRIVAL -> Line "
                          << selectedLine << " (" << simulator->getCurrentStrategyDescription() << ")"
                          << " | People: " << simulator->getLineCount(selectedLine)
                          << " | Total: " << simulator->getTotalSize()
                          << " | Wait: " << std::fixed << std::setprecision(1)
                          << simulator->getEstimatedWaitTime(selectedLine) << "s" << std::endl;
            }
            else
            {
                std::lock_guard<std::mutex> outputLock(outputMutex);
                std::cout << "[" << simulator->getName() << "] ARRIVAL -> ALL LINES FULL!" << std::endl;
            }
        }
        else if (event.type == SimulationEvent::SERVICE)
        {
            if (simulator->processService(event.line))
            {
                std::lock_guard<std::mutex> outputLock(outputMutex);
                std::cout << "[" << simulator->getName() << "] SERVICE -> Line "
                          << event.line << " completed | Remaining: "
                          << simulator->getLineCount(event.line) << std::endl;
            }
        }
    }

    void printStrategyStatistics(StrategySimulator *simulator)
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "\n--- [" << simulator->getName() << "] STATS ---" << std::endl;
        std::cout << "  Total people in system: " << simulator->getTotalSize() << std::endl;
        std::cout << "  Line distribution: ";
        for (int line = 1; line <= numberOfLines; ++line)
        {
            std::cout << "L" << line << ":" << simulator->getLineCount(line);
            if (line < numberOfLines)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Estimated wait times: ";
        for (int line = 1; line <= numberOfLines; ++line)
        {
            std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                      << simulator->getEstimatedWaitTime(line) << "s";
            if (line < numberOfLines)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Actual avg wait times: ";
        for (int line = 1; line <= numberOfLines; ++line)
        {
            auto lineStats = simulator->getLineActualWaitStats(line);
            std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                      << lineStats.averageWaitTime << "s(" << lineStats.completedPeople << ")";
            if (line < numberOfLines)
                std::cout << ", ";
        }
        std::cout << std::endl;
    }

    void printSummaryStatistics()
    {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cout << "\n========== SUMMARY STATISTICS ==========" << std::endl;

        for (const auto &simulator : simulators)
        {
            std::cout << "[" << simulator->getName() << "]" << std::endl;
            std::cout << "  Total people in system: " << simulator->getTotalSize() << std::endl;
            std::cout << "  Line distribution: ";
            for (int line = 1; line <= numberOfLines; ++line)
            {
                std::cout << "L" << line << ":" << simulator->getLineCount(line);
                if (line < numberOfLines)
                    std::cout << ", ";
            }
            std::cout << std::endl;
            std::cout << "  Wait times: ";
            for (int line = 1; line <= numberOfLines; ++line)
            {
                std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                          << simulator->getEstimatedWaitTime(line) << "s";
                if (line < numberOfLines)
                    std::cout << ", ";
            }
            std::cout << std::endl
                      << std::endl;
        }
        std::cout << "=========================================" << std::endl;
    }

    void writeJSONOutput()
    {
        std::lock_guard<std::mutex> lock(outputMutex);

        std::cout << "\n📁 Writing unified JSON output file..." << std::endl;

        jsonManager->writeUnifiedStrategyData(simulators);

        // Print summary for all strategies
        for (const auto &simulator : simulators)
        {
            auto summary = simulator->getCumulativePeopleSummary();
            std::cout << "   [" << simulator->getName() << "] Total: " << summary.totalPeople
                      << ", Active: " << summary.activePeople
                      << ", Completed: " << summary.completedPeople
                      << ", Avg Actual Wait: " << std::fixed << std::setprecision(1)
                      << summary.historicalAvgActualWait << "s" << std::endl;
        }

        std::cout << "✅ Unified JSON file updated: simulation_output/unified_simulation_data.json" << std::endl;
    }
};

// Global simulator instance for signal handling
std::unique_ptr<UnifiedQueueSimulator> g_simulator;

void signalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    if (g_simulator)
    {
        g_simulator->stop();
    }
    exit(0);
}

int main()
{
    std::cout << "=== UNIFIED QUEUE MANAGEMENT SIMULATOR ===" << std::endl;
    std::cout << "This simulator runs all three queue strategies simultaneously" << std::endl;
    std::cout << "with identical scenarios for fair comparison:" << std::endl;
    std::cout << "  1. FEWEST_PEOPLE - Always choose line with fewest people" << std::endl;
    std::cout << "  2. SHORTEST_WAIT_TIME - Adaptive strategy (fewest people -> shortest wait)" << std::endl;
    std::cout << "  3. FARTHEST_FROM_ENTRANCE - Choose line farthest from entrance" << std::endl;
    std::cout << "Press Ctrl+C to stop the simulation" << std::endl;
    std::cout << "===============================================" << std::endl;

    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Create and start unified simulator
        g_simulator = std::make_unique<UnifiedQueueSimulator>();
        g_simulator->start();

        std::cout << "Unified simulation running... Press Ctrl+C to stop" << std::endl;

        // Keep main thread alive
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}