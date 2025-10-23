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

class StrategySimulator
{
private:
    std::unique_ptr<QueueManager> queueManager;
    StrategyType strategyType;
    std::string strategyName;
    std::string firestoreCollection;

    // Configuration (shared across all strategies)
    const int maxQueueSize = 10000;
    const int numberOfLines = 3;
    const std::vector<double> serviceRates = {0.08, 0.12, 0.18};

public:
    StrategySimulator(StrategyType type, const std::string &name, const std::string &collection)
        : strategyType(type), strategyName(name), firestoreCollection(collection)
    {

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
            queueManager->dequeue(line);
            return true;
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

    // Simulators for each strategy
    std::vector<std::unique_ptr<StrategySimulator>> simulators;

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

            // Print periodic stats (every 20 seconds)
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatsTime).count() >= 20)
            {
                for (const auto &simulator : simulators)
                {
                    printStrategyStatistics(simulator.get());
                }
                lastStatsTime = now;
            }

            // Wait before generating next batch of events
            std::this_thread::sleep_for(updateInterval);
        }

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
        std::cout << "  Wait times: ";
        for (int line = 1; line <= numberOfLines; ++line)
        {
            std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                      << simulator->getEstimatedWaitTime(line) << "s";
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