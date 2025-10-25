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
#include <filesystem>
#include <sstream>
#include <fstream>
#include "../shared/cpp/QueueManager.h"
#include "../shared/cpp/ThroughputTracker.h"
#include "../shared/cpp/parameters.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

// ============================================================================
// SIMULATION CONFIGURATION - SINGLE SOURCE OF TRUTH
// ============================================================================
namespace SimConfig
{
    const int MAX_QUEUE_SIZE = 10000;
    const int NUMBER_OF_LINES = 6;
    const double ARRIVAL_RATE = 0.5;
    const std::vector<double> SERVICE_RATES = {0.08, 0.12, 0.18, 0.24, 0.30, 0.36};
    const std::chrono::milliseconds UPDATE_INTERVAL{2000};
}
// ============================================================================

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

public:
    StrategySimulator(StrategyType type, const std::string &name, const std::string &collection)
        : strategyType(type), strategyName(name), firestoreCollection(collection)
    {
        // Initialize line wait statistics
        lineWaitStats.resize(SimConfig::NUMBER_OF_LINES);

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

        queueManager = std::make_unique<QueueManager>(SimConfig::MAX_QUEUE_SIZE, SimConfig::NUMBER_OF_LINES, suffix, firestoreCollection);

        std::cout << "[" << strategyName << "] Initialized with " << SimConfig::NUMBER_OF_LINES
                  << " lines, max size per line: " << SimConfig::MAX_QUEUE_SIZE << std::endl;
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
        if (line >= 1 && line <= SimConfig::NUMBER_OF_LINES)
        {
            return lineWaitStats[line - 1];
        }
        return LineWaitStats(); // Return empty stats for invalid line
    }
};

// Firebase export functionality using REST API (no CLI setup required)
struct FirebaseExporter
{
private:
    std::string outputDirectory;

    // Firebase project configurations using existing secret
    struct FirebaseProject
    {
        std::string name;
        std::string url;
        std::string simulationPath;
    };

    // Only use the main project since all data is stored there
    std::vector<FirebaseProject> projects = {
        {"iot-queue-management", "https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app", "simulation"}};

public:
    FirebaseExporter(const std::string &dir) : outputDirectory(dir)
    {
        // Create output directory if it doesn't exist
        std::filesystem::create_directories(outputDirectory);
    }

    void exportAllFirebaseData()
    {
        std::cout << "\n🔥 Exporting all simulation data from Firebase..." << std::endl;

        std::string timestamp = getCurrentTimestamp();
        std::string filename = outputDirectory + "/firebase_export_" + timestamp + ".json";

        // Create a PowerShell script file and execute it
        std::string scriptPath = outputDirectory + "/export_script.ps1";
        if (createExportScript(scriptPath, filename))
        {
            std::cout << "  📤 Running REST API export script..." << std::endl;
            std::cout << "  📁 Output file: " << filename << std::endl;

            std::string command = "powershell -ExecutionPolicy Bypass -File \"" + scriptPath + "\"";
            int result = system(command.c_str());

            if (result == 0)
            {
                std::cout << "  ✅ Firebase export completed successfully!" << std::endl;
                std::cout << "  📄 Exported to: " << filename << std::endl;

                // Clean up script file
                std::remove(scriptPath.c_str());
            }
            else
            {
                std::cout << "  ❌ Firebase export failed!" << std::endl;
                std::cout << "  💡 Check your internet connection and Firebase credentials" << std::endl;
            }
        }
        else
        {
            std::cout << "  ❌ Failed to create export script!" << std::endl;
        }
    }

private:
    bool createExportScript(const std::string &scriptPath, const std::string &outputFile)
    {
        std::ofstream script(scriptPath);
        if (!script.is_open())
        {
            return false;
        }

        script << "# Firebase Export Script\n";
        script << "$secret = '" << FIREBASE_SECRET << "'\n";
        script << "$url = 'https://iot-queue-management-default-rtdb.europe-west1.firebasedatabase.app/.json?auth=' + $secret\n\n";

        script << "try {\n";
        script << "    Write-Host 'Connecting to Firebase...'\n";
        script << "    $response = Invoke-RestMethod -Uri $url -Method GET\n";
        script << "    if ($response) {\n";
        script << "        Write-Host '✅ Successfully exported from Firebase'\n";
        script << "        Write-Host '   Data contains: ' + ($response.PSObject.Properties.Name -join ', ')\n";
        script << "        \n";
        script << "        # Write complete Firebase data to file\n";
        script << "        $response | ConvertTo-Json -Depth 100 | Out-File -FilePath '" << outputFile << "' -Encoding UTF8\n";
        script << "        Write-Host '📄 Complete export saved to " << outputFile << "'\n";
        script << "    } else {\n";
        script << "        Write-Host '⚠️  No data returned from Firebase'\n";
        script << "    }\n";
        script << "} catch {\n";
        script << "    Write-Host '❌ Failed to export from Firebase: ' + $_.Exception.Message\n";
        script << "}\n";

        script.close();
        return true;
    }

    std::string getCurrentTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
        return oss.str();
    }
};

class UnifiedQueueSimulator
{
private:
    // Simulators for each strategy
    std::vector<std::unique_ptr<StrategySimulator>> simulators;

    // Firebase export manager
    std::unique_ptr<FirebaseExporter> firebaseExporter;

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
        // Initialize Firebase export manager
        std::string outputDir = "simulation_output";
        firebaseExporter = std::make_unique<FirebaseExporter>(outputDir);

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
        std::cout << "  Max queue size per line: " << SimConfig::MAX_QUEUE_SIZE << std::endl;
        std::cout << "  Number of lines: " << SimConfig::NUMBER_OF_LINES << std::endl;
        std::cout << "  Arrival rate: " << SimConfig::ARRIVAL_RATE << std::endl;
        std::cout << "  Service rates: ";
        for (size_t i = 0; i < SimConfig::SERVICE_RATES.size(); ++i)
        {
            std::cout << SimConfig::SERVICE_RATES[i];
            if (i < SimConfig::SERVICE_RATES.size() - 1)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Update interval: " << SimConfig::UPDATE_INTERVAL.count() << "ms" << std::endl;
        std::cout << "  Firebase export directory: " << outputDir << std::endl;
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

    void exportFirebaseData()
    {
        std::lock_guard<std::mutex> lock(outputMutex);

        // Print simulation summary before exporting
        std::cout << "\n📊 Final Simulation Summary:" << std::endl;
        for (const auto &simulator : simulators)
        {
            auto summary = simulator->getCumulativePeopleSummary();
            std::cout << "   [" << simulator->getName() << "] Total: " << summary.totalPeople
                      << ", Active: " << summary.activePeople
                      << ", Completed: " << summary.completedPeople
                      << ", Avg Actual Wait: " << std::fixed << std::setprecision(1)
                      << summary.historicalAvgActualWait << "s" << std::endl;
        }

        // Export data from Firebase
        firebaseExporter->exportAllFirebaseData();
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
            if (arrivalDist(rng) < SimConfig::ARRIVAL_RATE)
            {
                eventsToProcess.emplace_back(SimulationEvent::ARRIVAL);
            }

            // Generate service events for each line
            for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
            {
                if (serviceDist(rng) < SimConfig::SERVICE_RATES[line - 1])
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

            // Wait before generating next batch of events
            std::this_thread::sleep_for(SimConfig::UPDATE_INTERVAL);
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
        for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
        {
            std::cout << "L" << line << ":" << simulator->getLineCount(line);
            if (line < SimConfig::NUMBER_OF_LINES)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Estimated wait times: ";
        for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
        {
            std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                      << simulator->getEstimatedWaitTime(line) << "s";
            if (line < SimConfig::NUMBER_OF_LINES)
                std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "  Actual avg wait times: ";
        for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
        {
            auto lineStats = simulator->getLineActualWaitStats(line);
            std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                      << lineStats.averageWaitTime << "s(" << lineStats.completedPeople << ")";
            if (line < SimConfig::NUMBER_OF_LINES)
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
            for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
            {
                std::cout << "L" << line << ":" << simulator->getLineCount(line);
                if (line < SimConfig::NUMBER_OF_LINES)
                    std::cout << ", ";
            }
            std::cout << std::endl;
            std::cout << "  Wait times: ";
            for (int line = 1; line <= SimConfig::NUMBER_OF_LINES; ++line)
            {
                std::cout << "L" << line << ":" << std::fixed << std::setprecision(1)
                          << simulator->getEstimatedWaitTime(line) << "s";
                if (line < SimConfig::NUMBER_OF_LINES)
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

        // Export Firebase data before shutting down
        std::cout << "\n🔥 Exporting final data from Firebase..." << std::endl;
        g_simulator->exportFirebaseData();
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