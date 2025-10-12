#include "Person.h"
#include <sstream>
#include <iomanip>

Person::Person(double expectedWaitTime, int lineNumber)
    : m_expectedWaitTime(expectedWaitTime)
    , m_enteringTimestamp(getCurrentTimestamp())
    , m_exitingTimestamp(0)
    , m_lineNumber(lineNumber)
{
}

double Person::getActualWaitTime() const
{
    if (m_exitingTimestamp == 0)
    {
        return 0.0; // Person hasn't exited yet
    }
    
    return static_cast<double>(m_exitingTimestamp - m_enteringTimestamp) / 1000.0; // Convert to seconds
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
    oss << "person_" << m_enteringTimestamp;
    return oss.str();
}

long long Person::getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return millis;
}