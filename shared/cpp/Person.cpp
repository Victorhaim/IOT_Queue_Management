#include "Person.h"
#include <sstream>
#include <iomanip>

// Initialize static member
long long Person::s_simulationStartTime = 0;

Person::Person(double expectedWaitTime, int lineNumber)
    : m_expectedWaitTime(expectedWaitTime)
    , m_enteringTimestamp(getCurrentTimestamp())
    , m_exitingTimestamp(0)
    , m_lineNumber(lineNumber)
    , m_personId(0) // Will be set by QueueManager
{
}

double Person::getActualWaitTime() const
{
    if (m_exitingTimestamp == 0)
    {
        return 0.0; // Person hasn't exited yet
    }
    
    return static_cast<double>(m_exitingTimestamp - m_enteringTimestamp); // Already in seconds
}

void Person::recordExit()
{
    if (m_exitingTimestamp == 0) // Only record exit once
    {
        m_exitingTimestamp = getCurrentTimestamp();
    }
}

std::string Person::getId() const
{
    std::ostringstream oss;
    oss << "person_" << m_personId;
    return oss.str();
}

void Person::setSimulationStartTime()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    s_simulationStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void Person::setPersonId(int id)
{
    m_personId = id;
}

long long Person::getCurrentTimestamp()
{
    if (s_simulationStartTime == 0) {
        // If simulation start time not set, set it now
        setSimulationStartTime();
        return 0; // First timestamp is always 0
    }
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto currentMillis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Return seconds since simulation start
    return (currentMillis - s_simulationStartTime) / 1000;
}