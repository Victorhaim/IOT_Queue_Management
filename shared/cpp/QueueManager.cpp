#include "QueueManager.h"
#include <cstring>

// Implementation updated to use std::vector for line storage instead of fixed-size C array.

QueueManager::QueueManager(int maxSize, int numberOfLines)
    : m_maxSize(maxSize), m_numberOfLines(numberOfLines), m_totalPeople(0), m_lines(), m_lineThroughputs()
{
    if (m_numberOfLines < 0)
        m_numberOfLines = 0;
    if (m_numberOfLines > MAX_LINES)
        m_numberOfLines = MAX_LINES;    // enforce historical cap
    m_lines.reserve(m_numberOfLines);   // reserve capacity to avoid reallocations
    m_lines.assign(m_numberOfLines, 0); // initialize with zeros
    
    // Initialize throughput tracking for each line
    m_lineThroughputs.reserve(m_numberOfLines);
    m_lineThroughputs.assign(m_numberOfLines, DEFAULT_THROUGHPUT);
}

bool QueueManager::enqueue()
{
    if (isFull())
    {
        return false;
    }

    int lineNumber = getNextLineNumber();
    if (lineNumber == -1)
    {
        return false;
    }

    // External API uses 1-based line numbers; internal storage is 0-based
    m_lines[lineNumber - 1]++;
    m_totalPeople++;
    return true;
}

bool QueueManager::dequeue(int lineNumber)
{
    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    // Check if line has people
    if (m_lines[lineNumber - 1] == 0)
    {
        return false;
    }

    m_lines[lineNumber - 1]--;
    m_totalPeople--;
    return true;
}

bool QueueManager::enqueueOnLine(int lineNumber)
{
    if (isFull())
    {
        return false;
    }

    if (!isValidLineNumber(lineNumber))
    {
        return false;
    }

    m_lines[lineNumber - 1]++;
    m_totalPeople++;
    return true;
}

int QueueManager::size() const
{
    return m_totalPeople;
}

bool QueueManager::isEmpty() const
{
    return m_totalPeople == 0;
}

bool QueueManager::isFull() const
{
    if (m_maxSize == 0)
    {
        return false; // 0 means unlimited
    }
    return m_totalPeople >= m_maxSize;
}

int QueueManager::getNextLineNumber() const
{
    if (m_numberOfLines == 0)
    {
        return -1;
    }

    // Smart line selection: find line with shortest estimated wait time
    double minWaitTime = getEstimatedWaitTime(1);
    int bestLine = 1; // return value stays 1-based
    
    for (int i = 2; i <= m_numberOfLines; i++)
    {
        double waitTime = getEstimatedWaitTime(i);
        if (waitTime < minWaitTime)
        {
            minWaitTime = waitTime;
            bestLine = i;
        }
    }
    return bestLine;
}

int QueueManager::getNumberOfLines() const
{
    return m_numberOfLines;
}

int QueueManager::getLineCount(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return -1;
    }

    return m_lines[lineNumber - 1];
}

void QueueManager::setLineCount(int lineNumber, int count)
{
    if (!isValidLineNumber(lineNumber))
    {
        return;
    }

    // Update total people count
    m_totalPeople -= m_lines[lineNumber - 1];
    m_lines[lineNumber - 1] = (count < 0) ? 0 : count;
    m_totalPeople += m_lines[lineNumber - 1];
}

void QueueManager::reset()
{
    for (int i = 0; i < m_numberOfLines; i++)
    {
        m_lines[i] = 0;
    }
    m_totalPeople = 0;
}

bool QueueManager::isValidLineNumber(int lineNumber) const
{
    return lineNumber >= 1 && lineNumber <= m_numberOfLines;
}

void QueueManager::updateTotalPeople()
{
    m_totalPeople = 0;
    for (int i = 0; i < m_numberOfLines; i++)
    {
        m_totalPeople += m_lines[i];
    }
}

void QueueManager::updateLineThroughput(int lineNumber, double throughputPerSecond)
{
    if (!isValidLineNumber(lineNumber))
    {
        return;
    }
    
    // Ensure reasonable bounds for throughput
    double boundedThroughput = std::max(0.1, std::min(5.0, throughputPerSecond));
    m_lineThroughputs[lineNumber - 1] = boundedThroughput;
}

double QueueManager::getLineThroughput(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return DEFAULT_THROUGHPUT;
    }
    
    return m_lineThroughputs[lineNumber - 1];
}

double QueueManager::getEstimatedWaitTime(int lineNumber) const
{
    if (!isValidLineNumber(lineNumber))
    {
        return 999.0; // Return high wait time for invalid lines
    }
    
    int peopleInLine = m_lines[lineNumber - 1];
    double throughput = m_lineThroughputs[lineNumber - 1];
    
    // Estimated wait time = number of people ahead / service rate
    // Add small penalty for being in line vs. empty line
    if (peopleInLine == 0)
    {
        return 0.0; // No wait for empty line
    }
    
    return static_cast<double>(peopleInLine) / throughput;
}