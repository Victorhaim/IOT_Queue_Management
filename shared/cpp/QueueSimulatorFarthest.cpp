#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include "QueueManager.h"
#include "FirebaseClient.h"
#include "FirebaseStructureBuilder.h"
#include "ThroughputTracker.h"

#include <fstream>
#include <sstream>
#include <string>

class QueueSimulatorFarthest
{
private:
    // Configuration (must be declared before other members that use them)
    const int maxQueueSize = 50;
    const int numberOfLines = 2;
    const double arrivalRate = 0.3;                       // Probability of arrival per second
    const std::vector<double> serviceRates = {0.15, 0.35}; // Different service rates per line (line 1: slow, line 2: fast)
    const std::chrono::milliseconds updateInterval{1000}; // 1 second updates

    std::unique_ptr<QueueManager> queueManager;
    std::unique_ptr<FirebaseClient> firebaseClient;
    std::mt19937 rng;
    std::uniform_real_distribution<double> arrivalDist;
    std::uniform_real_distribution<double> serviceDist;
    std::uniform_int_distribution<int> lineDist;

    std::atomic<bool> running{false};
    std::thread simulationThread;

    // Throughput tracking using shared code
    std::vector<ThroughputTracker> throughputTrackers; // One tracker per line

public:
    QueueSimulatorFarthest() : queueManager(std::make_unique<QueueManager>(maxQueueSize, numberOfLines)),
                       firebaseClient(std::make_unique<FirebaseClient>(
                           "iot-queue-management-farthest",
                           "https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app")),
                       rng(std::chrono::steady_clock::now().time_since_epoch().count()),
                       arrivalDist(0.0, 1.0),
                       serviceDist(0.0, 1.0),
                       lineDist(1, numberOfLines),
                       throughputTrackers(numberOfLines) // Initialize throughput trackers
    {
        std::cout << "Queue Simulator (FARTHEST FROM ENTRANCE STRATEGY) initialized with " << numberOfLines
                  << " lines, max size: " << maxQueueSize << std::endl;
        
        // Show service rate differences
        for (int i = 0; i < numberOfLines; i++)
        {
            std::cout << "Line " << (i + 1) << " service rate: " << std::fixed << std::setprecision(2) 
                      << serviceRates[i] << " (expected throughput: ~" 
                      << serviceRates[i] << " people/sec)" << std::endl;
        }

        std::cout << "Strategy: Choose line where last person is FARTHEST FROM ENTRANCE" << std::endl;
        std::cout << "Assumption: Higher line numbers = farther from entrance" << std::endl;
        std::cout << "Throughput trackers initialized for real-time measurement" << std::endl;

        // Initialize Firebase client
        if (!firebaseClient->initialize())
        {
            std::cerr << "Failed to initialize Firebase client!" << std::endl;
        }
        else
        {
            std::cout << "Firebase client initialized successfully" << std::endl;
        }
    }

    ~QueueSimulatorFarthest()
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
        std::cout << "Starting queue simulation (FARTHEST FROM ENTRANCE strategy)..." << std::endl;

        // Clear existing cloud data before starting simulation
        clearCloudData();

        simulationThread = std::thread([this]()
                                       { simulate(); });
    }

    void stop()
    {
        if (running.load())
        {
            std::cout << "Stopping simulation..." << std::endl;
            running.store(false);
            if (simulationThread.joinable())
            {
                simulationThread.join();
            }
            std::cout << "Simulation stopped." << std::endl;
        }
    }

private:
    void clearCloudData()
    {
        std::cout << "🧹 Clearing existing cloud data..." << std::endl;

        try
        {
            // Clear all queue lines with "farthest" prefix
            for (int i = 1; i <= numberOfLines; ++i)
            {
                std::string queuePath = "queues_farthest/line" + std::to_string(i);
                if (firebaseClient->deleteData(queuePath))
                {
                    std::cout << "✅ Successfully cleared existing data for farthest line " << i << std::endl;
                }
                else
                {
                    std::cout << "ℹ️  Note: No existing data found for farthest line " << i << " or failed to clear" << std::endl;
                }
            }

            // Clear the currentBest aggregated data for farthest strategy
            if (firebaseClient->deleteData("currentBest_farthest"))
            {
                std::cout << "✅ Successfully cleared currentBest_farthest data" << std::endl;
            }
            else
            {
                std::cout << "ℹ️  Note: No existing currentBest_farthest data found or failed to clear" << std::endl;
            }

            // Optional: Also clear the entire queues_farthest node to ensure a fresh start
            if (firebaseClient->deleteData("queues_farthest"))
            {
                std::cout << "✅ Successfully cleared all farthest queue data" << std::endl;
            }
            else
            {
                std::cout << "ℹ️  Note: No existing farthest queue data found or failed to clear all queues" << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "⚠️  Warning: Error clearing cloud data: " << e.what() << std::endl;
            std::cerr << "Continuing with simulation..." << std::endl;
        }

        std::cout << "🚀 Starting fresh simulation..." << std::endl;
    }
    
    void simulate()
    {
        std::cout << "Simulation loop started" << std::endl;

        while (running.load())
        {
            // Simulate arrivals
            if (arrivalDist(rng) < arrivalRate)
            {
                if (!queueManager->isFull())
                {
                    // Use FARTHEST_FROM_ENTRANCE strategy
                    int selectedLine = queueManager->getNextLineNumber(LineSelectionStrategy::FARTHEST_FROM_ENTRANCE);
                    queueManager->enqueue(LineSelectionStrategy::FARTHEST_FROM_ENTRANCE);
                    std::cout << "New arrival! Selected line " << selectedLine << " (FARTHEST FROM ENTRANCE strategy)"
                              << " (people in line: " << queueManager->getLineCount(selectedLine) << ")"
                              << " Total queue size: " << queueManager->size() << std::endl;
                }
                else
                {
                    std::cout << "Queue full - customer turned away" << std::endl;
                }
            }

            // Simulate service completions with different rates per line
            for (int line = 1; line <= numberOfLines; ++line)
            {
                double lineServiceRate = serviceRates[line - 1]; // Get specific rate for this line
                if (queueManager->getLineCount(line) > 0 && serviceDist(rng) < lineServiceRate)
                {
                    queueManager->dequeue(line);

                    // Use shared throughput tracking
                    throughputTrackers[line - 1].recordServiceCompletion();

                    // Update QueueManager with current throughput data
                    double currentThroughput = throughputTrackers[line - 1].getCurrentThroughput();
                    queueManager->updateLineThroughput(line, currentThroughput);

                    std::cout << "Service completed on line " << line
                              << " (rate=" << std::fixed << std::setprecision(2) << lineServiceRate << ")"
                              << ", remaining: " << queueManager->getLineCount(line)
                              << ", throughput: " << std::fixed << std::setprecision(3)
                              << currentThroughput << " people/sec"
                              << " (based on " << throughputTrackers[line - 1].getServiceCount() << " services)"
                              << ", est. wait: " << std::fixed << std::setprecision(1) 
                              << queueManager->getEstimatedWaitTime(line) << "s" << std::endl;
                }
            }

            // Write current state to Firebase (JSON file for now)
            writeToFirebase();

            // Wait for next update
            std::this_thread::sleep_for(updateInterval);
        }
    }

    void writeToFirebase()
    {
        try
        {
            // Collect data for all lines
            std::vector<FirebaseStructureBuilder::LineData> allLinesData;
            int totalPeople = 0;

            for (int line = 1; line <= numberOfLines; ++line)
            {
                int currentOccupancy = queueManager->getLineCount(line);
                double throughputFactor = throughputTrackers[line - 1].getCurrentThroughput();
                double averageWaitTime = FirebaseStructureBuilder::calculateAverageWaitTime(
                    currentOccupancy, throughputFactor);

                totalPeople += currentOccupancy;

                // Create line data object
                FirebaseStructureBuilder::LineData lineData(
                    currentOccupancy, throughputFactor, averageWaitTime, line);
                allLinesData.push_back(lineData);

                // Generate JSON and write to Firebase for this specific line (farthest strategy)
                std::string json = FirebaseStructureBuilder::generateLineDataJson(lineData);
                std::string queuePath = "queues_farthest/line" + std::to_string(line);

                if (firebaseClient->updateData(queuePath, json))
                {
                    std::cout << "✅ Farthest Line " << line << " updated - Occupancy: " << currentOccupancy
                              << ", Throughput: " << std::fixed << std::setprecision(3) << throughputFactor
                              << ", Avg Wait: " << std::fixed << std::setprecision(1) << averageWaitTime << "s"
                              << " [" << (throughputTrackers[line - 1].hasReliableData() ? "measured" : "default") << "]" << std::endl;
                }
                else
                {
                    std::cerr << "❌ Failed to update Firebase for farthest line " << line << std::endl;
                }
            }

            // Calculate recommended line and write aggregated data
            if (!allLinesData.empty())
            {
                FirebaseStructureBuilder::AggregatedData aggData =
                    FirebaseStructureBuilder::createAggregatedData(allLinesData.data(), totalPeople, static_cast<int>(allLinesData.size()));

                std::string aggJson = FirebaseStructureBuilder::generateAggregatedDataJson(aggData);
                std::string aggPath = "currentBest_farthest";

                if (firebaseClient->updateData(aggPath, aggJson))
                {
                    std::cout << "✅ Aggregated farthest queue object updated (currentBest_farthest) totalPeople=" << totalPeople
                              << " recommendedLine=" << aggData.recommendedLine
                              << " waitTime=" << std::round(aggData.averageWaitTime) << "s"
                              << " placeInLine=" << aggData.currentOccupancy << std::endl;
                }
                else
                {
                    std::cerr << "❌ Failed to update aggregated farthest queue object" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error writing to Firebase: " << e.what() << std::endl;
        }
    }
};

// Global simulator instance for signal handling
std::unique_ptr<QueueSimulatorFarthest> g_simulator;

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
    std::cout << "=== Queue Management System - C++ Simulator (FARTHEST FROM ENTRANCE) ===" << std::endl;
    std::cout << "This simulator will generate realistic queue data using FARTHEST FROM ENTRANCE strategy" << std::endl;
    std::cout << "Strategy: Choose line where last person is farthest from entrance" << std::endl;
    std::cout << "Assumption: Higher line numbers are farther from entrance" << std::endl;
    std::cout << "Press Ctrl+C to stop the simulation" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Create and start simulator
        g_simulator = std::make_unique<QueueSimulatorFarthest>();
        g_simulator->start();

        std::cout << "Simulation running... Press Ctrl+C to stop" << std::endl;

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