#include "QueueManager.h"
#include <cstring>

// Implementation updated to use std::vector for line storage instead of fixed-size C array.

QueueManager::QueueManager(int maxSize, int numberOfLines)
    : m_maxSize(maxSize), m_numberOfLines(numberOfLines), m_totalPeople(0), m_lines()
{
    if (m_numberOfLines < 0)
        m_numberOfLines = 0;
    if (m_numberOfLines > MAX_LINES)
        m_numberOfLines = MAX_LINES;    // enforce historical cap
    m_lines.reserve(m_numberOfLines);   // reserve capacity to avoid reallocations
    m_lines.assign(m_numberOfLines, 0); // initialize with zeros
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

    int minPeople = m_lines[0];
    int bestLine = 1; // return value stays 1-based
    for (int i = 1; i < m_numberOfLines; i++)
    {
        if (m_lines[i] < minPeople)
        {
            minPeople = m_lines[i];
            bestLine = i + 1; // convert to 1-based for public API
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