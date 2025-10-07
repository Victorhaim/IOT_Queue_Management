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

#include <fstream>
#include <sstream>
#include <string>

class QueueSimulator
{
private:
    // Configuration (must be declared before other members that use them)
    const int maxQueueSize = 50;
    const int numberOfLines = 2;
    const double arrivalRate = 0.3;                       // Probability of arrival per second
    const double serviceRate = 0.2;                       // Probability of service completion per second
    const std::chrono::milliseconds updateInterval{1000}; // 1 second updates

    std::unique_ptr<QueueManager> queueManager;
    std::unique_ptr<FirebaseClient> firebaseClient;
    std::mt19937 rng;
    std::uniform_real_distribution<double> arrivalDist;
    std::uniform_real_distribution<double> serviceDist;
    std::uniform_int_distribution<int> lineDist;

    std::atomic<bool> running{false};
    std::thread simulationThread;

    // New metrics tracking
    std::vector<double> throughputFactors;                                // Throughput factor for each line
    std::vector<std::chrono::steady_clock::time_point> lastServiceTimes;  // Last service time for each line
    std::vector<int> serviceCompletionCounts;                             // Count of services completed per line
    std::vector<std::chrono::steady_clock::time_point> serviceStartTimes; // Track when service sessions started

public:
    QueueSimulator() : queueManager(std::make_unique<QueueManager>(maxQueueSize, numberOfLines)),
                       firebaseClient(std::make_unique<FirebaseClient>(
                           "iot-queue-management",
                           "https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app")),
                       rng(std::chrono::steady_clock::now().time_since_epoch().count()),
                       arrivalDist(0.0, 1.0),
                       serviceDist(0.0, 1.0),
                       lineDist(1, numberOfLines),
                       throughputFactors(numberOfLines, 1.0), // Initialize with default value 1.0
                       lastServiceTimes(numberOfLines),
                       serviceCompletionCounts(numberOfLines, 0),
                       serviceStartTimes(numberOfLines)
    {
        std::cout << "Queue Simulator initialized with " << numberOfLines
                  << " lines, max size: " << maxQueueSize << std::endl;

        // Initialize random throughput factors for each line (between 0.5 and 2.0)
        std::uniform_real_distribution<double> throughputDist(0.5, 2.0);
        auto now = std::chrono::steady_clock::now();

        for (int i = 0; i < numberOfLines; ++i)
        {
            throughputFactors[i] = throughputDist(rng);
            lastServiceTimes[i] = now;
            serviceStartTimes[i] = now;
            std::cout << "Line " << (i + 1) << " initial throughput factor: "
                      << std::fixed << std::setprecision(2) << throughputFactors[i] << " people/second" << std::endl;
        }

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

    ~QueueSimulator()
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
        std::cout << "Starting queue simulation..." << std::endl;

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
            // Clear all queue lines
            for (int i = 1; i <= numberOfLines; ++i)
            {
                std::string queuePath = "queues/line" + std::to_string(i);
                if (firebaseClient->deleteData(queuePath))
                {
                    std::cout << "✅ Successfully cleared existing data for line " << i << std::endl;
                }
                else
                {
                    std::cout << "ℹ️  Note: No existing data found for line " << i << " or failed to clear" << std::endl;
                }
            }

            // Optional: Also clear the entire queues node to ensure a fresh start
            if (firebaseClient->deleteData("queues"))
            {
                std::cout << "✅ Successfully cleared all queue data" << std::endl;
            }
            else
            {
                std::cout << "ℹ️  Note: No existing queue data found or failed to clear all queues" << std::endl;
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
                    queueManager->enqueue();
                    std::cout << "New arrival! Queue size: " << queueManager->size() << std::endl;
                }
                else
                {
                    std::cout << "Queue full - customer turned away" << std::endl;
                }
            }

            // Simulate service completions
            for (int line = 1; line <= numberOfLines; ++line)
            {
                if (queueManager->getLineCount(line) > 0 && serviceDist(rng) < serviceRate)
                {
                    queueManager->dequeue(line);

                    // Update throughput calculation
                    auto currentTime = std::chrono::steady_clock::now();
                    auto timeSinceLastService = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                    currentTime - lastServiceTimes[line - 1])
                                                    .count();

                    serviceCompletionCounts[line - 1]++;

                    // Update throughput factor: calculate people served per second
                    if (timeSinceLastService > 0 && serviceCompletionCounts[line - 1] > 1)
                    {
                        auto totalServiceTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                    currentTime - serviceStartTimes[line - 1])
                                                    .count();

                        if (totalServiceTime > 0)
                        {
                            // Calculate throughput as people per second
                            throughputFactors[line - 1] = (double)serviceCompletionCounts[line - 1] / (totalServiceTime / 1000.0);
                        }
                    }

                    lastServiceTimes[line - 1] = currentTime;

                    std::cout << "Service completed on line " << line
                              << ", remaining: " << queueManager->getLineCount(line)
                              << ", throughput: " << std::fixed << std::setprecision(3)
                              << throughputFactors[line - 1] << " people/sec" << std::endl;
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
            // Write data for each queue line separately
            int totalPeople = 0;
            int recommendedLineLocal = -1;
            int minPeople = INT32_MAX;
            auto nowSys = std::chrono::system_clock::now();
            auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(nowSys.time_since_epoch()).count();
            for (int line = 1; line <= numberOfLines; ++line)
            {
                int currentOccupancy = queueManager->getLineCount(line);
                double throughputFactor = throughputFactors[line - 1];
                double averageWaitTime = (throughputFactor > 0.0) ? currentOccupancy / throughputFactor : 0.0;
                totalPeople += currentOccupancy;
                if (currentOccupancy < minPeople)
                {
                    minPeople = currentOccupancy;
                    recommendedLineLocal = line;
                }

                // Create JSON data for this specific queue line
                std::ostringstream json;
                json << "{\n";
                json << "    \"currentOccupancy\": " << currentOccupancy << ",\n";
                json << "    \"throughputFactor\": " << std::fixed << std::setprecision(4) << throughputFactor << ",\n";
                json << "    \"averageWaitTime\": " << std::fixed << std::setprecision(2) << averageWaitTime << ",\n";
                json << "    \"lastUpdated\": \"" << getCurrentTimestamp() << "\",\n"; // human readable
                json << "    \"lineNumber\": " << line << "\n";
                json << "}\n";

                // Write to Firebase path for this specific line
                std::string queuePath = "queues/line" + std::to_string(line);

                if (firebaseClient->updateData(queuePath, json.str()))
                {
                    std::cout << "✅ Line " << line << " updated - Occupancy: " << currentOccupancy
                              << ", Throughput: " << std::fixed << std::setprecision(3) << throughputFactor
                              << ", Avg Wait: " << std::fixed << std::setprecision(1) << averageWaitTime << "s" << std::endl;
                }
                else
                {
                    std::cerr << "❌ Failed to update Firebase for line " << line << std::endl;
                }
            }

            // Also write aggregated queue object expected by Flutter UI
            // Path: currentBest (queueId = "currentBest")
            if (numberOfLines > 0)
            {
                std::ostringstream agg;
                agg << "{\n";
                agg << "  \"totalPeople\": " << totalPeople << ",\n";
                agg << "  \"numberOfLines\": " << numberOfLines << ",\n";
                agg << "  \"recommendedLine\": " << (recommendedLineLocal == -1 ? 0 : recommendedLineLocal) << "\n";
                agg << "}\n";

                if (firebaseClient->updateData("currentBest", agg.str()))
                {
                    std::cout << "✅ Aggregated queue object updated (currentBest) totalPeople=" << totalPeople
                              << " recommendedLine=" << recommendedLineLocal << std::endl;
                }
                else
                {
                    std::cerr << "❌ Failed to update aggregated queue object" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error writing to Firebase: " << e.what() << std::endl;
        }
    }

    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

// Global simulator instance for signal handling
std::unique_ptr<QueueSimulator> g_simulator;

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
    std::cout << "=== Queue Management System - C++ Simulator ===" << std::endl;
    std::cout << "This simulator will generate realistic queue data" << std::endl;
    std::cout << "Press Ctrl+C to stop the simulation" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Create and start simulator
        g_simulator = std::make_unique<QueueSimulator>();
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