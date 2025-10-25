#pragma once

#include <chrono>
#include <string>

/**
 * @brief Represents a person in the queue system
 * 
 * Contains all tracking information for an individual person in the queue,
 * including timing data and line assignment.
 */
class Person
{
public:
    /**
     * @brief Constructs a new Person entering the queue
     * @param expectedWaitTime Estimated wait time when entering the queue (in seconds)
     * @param lineNumber The line number this person is assigned to (1-based indexing)
     */
    Person(double expectedWaitTime, int lineNumber);

    /**
     * @brief Default constructor for containers
     */
    Person() = default;

    // Getters
    /**
     * @brief Gets the expected wait time when this person entered the queue
     * @return Expected wait time in seconds
     */
    double getExpectedWaitTime() const { return m_expectedWaitTime; }

    /**
     * @brief Gets the timestamp when this person entered the queue
     * @return Timestamp in milliseconds since epoch
     */
    long long getEnteringTimestamp() const { return m_enteringTimestamp; }

    /**
     * @brief Gets the timestamp when this person exited the queue
     * @return Timestamp in milliseconds since epoch, or 0 if not yet exited
     */
    long long getExitingTimestamp() const { return m_exitingTimestamp; }

    /**
     * @brief Gets the line number this person is assigned to
     * @return Line number (1-based indexing)
     */
    int getLineNumber() const { return m_lineNumber; }

    /**
     * @brief Calculates the actual wait time for this person
     * @return Actual wait time in seconds, or 0 if not yet exited
     */
    double getActualWaitTime() const;

    /**
     * @brief Checks if this person has exited the queue
     * @return true if person has exited, false otherwise
     */
    bool hasExited() const { return m_exitingTimestamp != 0; }

    // Setters
    /**
     * @brief Records the exit timestamp for this person
     */
    void recordExit();

    /**
     * @brief Sets the line number for this person
     * @param lineNumber New line number (1-based indexing)
     */
    void setLineNumber(int lineNumber) { m_lineNumber = lineNumber; }

    /**
     * @brief Gets a unique identifier for this person
     * @return Unique ID string based on entering timestamp
     */
    std::string getId() const;

    /**
     * @brief Sets the simulation start time to current time (call once at simulation start)
     */
    static void setSimulationStartTime();

    /**
     * @brief Sets the person ID for this person (used by QueueManager)
     * @param id The unique ID to assign to this person
     */
    void setPersonId(int id);

private:
    double m_expectedWaitTime;      ///< Expected wait time when entering (seconds)
    long long m_enteringTimestamp;  ///< Timestamp when entering queue (seconds since simulation start)
    long long m_exitingTimestamp;   ///< Timestamp when exiting queue (seconds since simulation start, 0 = not exited)
    int m_lineNumber;               ///< Line number assignment (1-based indexing)
    int m_personId;                 ///< Unique person ID assigned by QueueManager

    /**
     * @brief Gets current timestamp in seconds since simulation start
     * @return Current timestamp relative to simulation start
     */
    static long long getCurrentTimestamp();

    static long long s_simulationStartTime; ///< Start time of simulation in system milliseconds
};