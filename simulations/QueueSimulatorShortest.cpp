#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include "../shared/cpp/QueueManager.h"
#include "../shared/cpp/ThroughputTracker.h"

#include <fstream>
#include <sstream>
#include <string>

class QueueSimulatorShortest
{
private:
    // Configuration (must be declared before other members that use them)
    const int maxQueueSize = 7; // Per-line limit
    const int numberOfLines = 3;
    const double arrivalRate = 0.18;                       // Probability of arrival per second (~11 people/minute)
    const std::vector<double> serviceRates = {0.08, 0.18}; // Slower service rates per line (line 1: very slow, line 2: slow)
    const std::chrono::milliseconds updateInterval{2000};  // 2 second updates

    std::unique_ptr<QueueManager> queueManager;
    std::mt19937 rng;
    std::uniform_real_distribution<double> arrivalDist;
    std::uniform_real_distribution<double> serviceDist;
    std::uniform_int_distribution<int> lineDist;

    std::atomic<bool> running{false};
    std::thread simulationThread;

public:
    QueueSimulatorShortest() : queueManager(std::make_unique<QueueManager>(maxQueueSize, numberOfLines, "_shortest", "iot-queue-management-shortest")), // Shortest strategy
                               rng(std::chrono::steady_clock::now().time_since_epoch().count()),
                               arrivalDist(0.0, 1.0),
                               serviceDist(0.0, 1.0),
                               lineDist(1, numberOfLines)
    {
        std::cout << "Queue Simulator (FEWEST PEOPLE STRATEGY) initialized with " << numberOfLines
                  << " lines, max size per line: " << maxQueueSize << std::endl;

        // Show service rate differences
        for (int i = 0; i < numberOfLines; i++)
        {
            std::cout << "Line " << (i + 1) << " service rate: " << std::fixed << std::setprecision(2)
                      << serviceRates[i] << " (expected throughput: ~"
                      << serviceRates[i] << " people/sec)" << std::endl;
        }

        std::cout << "Strategy: Always choose line with FEWEST PEOPLE" << std::endl;
    }

    ~QueueSimulatorShortest()
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
        std::cout << "Starting queue simulation (FEWEST PEOPLE strategy)..." << std::endl;

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
    void simulate()
    {
        std::cout << "Simulation loop started" << std::endl;

        while (running.load())
        {
            // Simulate arrivals
            if (arrivalDist(rng) < arrivalRate)
            {
                // Use FEWEST_PEOPLE strategy
                if (queueManager->enqueue(LineSelectionStrategy::FEWEST_PEOPLE))
                {
                    int selectedLine = queueManager->getNextLineNumber(LineSelectionStrategy::FEWEST_PEOPLE);
                    std::cout << "New arrival! Selected line " << selectedLine << " (FEWEST PEOPLE strategy)"
                              << " (people in line: " << queueManager->getLineCount(selectedLine) << ")"
                              << " Total queue size: " << queueManager->size() << std::endl;
                }
                else
                {
                    std::cout << "All lines full - customer turned away" << std::endl;
                }
            }

            // Simulate service completions with different rates per line
            for (int line = 1; line <= numberOfLines; ++line)
            {
                double lineServiceRate = serviceRates[line - 1]; // Get specific rate for this line
                if (queueManager->getLineCount(line) > 0 && serviceDist(rng) < lineServiceRate)
                {
                    queueManager->dequeue(line);

                    std::cout << "Service completed on line " << line
                              << " (rate=" << std::fixed << std::setprecision(2) << lineServiceRate << ")"
                              << ", remaining: " << queueManager->getLineCount(line)
                              << ", est. wait: " << std::fixed << std::setprecision(1)
                              << queueManager->getEstimatedWaitTime(line) << "s" << std::endl;
                }
            }

            // Wait for next update
            std::this_thread::sleep_for(updateInterval);
        }
    }
};

// Global simulator instance for signal handling
std::unique_ptr<QueueSimulatorShortest> g_simulator;

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
    std::cout << "=== Queue Management System - C++ Simulator (FEWEST PEOPLE) ===" << std::endl;
    std::cout << "This simulator will generate realistic queue data using FEWEST PEOPLE strategy" << std::endl;
    std::cout << "Strategy: Always choose the line with the fewest people" << std::endl;
    std::cout << "Press Ctrl+C to stop the simulation" << std::endl;
    std::cout << "=================================================" << std::endl;

    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try
    {
        // Create and start simulator
        g_simulator = std::make_unique<QueueSimulatorShortest>();
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