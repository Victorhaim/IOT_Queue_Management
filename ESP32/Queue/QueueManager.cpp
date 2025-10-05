#include "QueueManager.h"
#include <algorithm>

QueueManager::QueueManager(size_t maxSize, size_t numberOfLines)
    : m_maxSize(maxSize), m_numberOfLines(numberOfLines), m_totalPeople(0) {
    for (int i = 1; i <= m_numberOfLines; ++i) {
        auto line = make_shared<Line>(Line{i, 0});
        lineMap[i] = line;
        queue.insert(line);
    }
}

bool QueueManager::enqueue() {
    if (isFull()) {
        return false;
    }

    if (queue.empty()) {
        return false;
    }

    auto it = queue.begin();
    auto line = *it;
    queue.erase(it);
    line->peopleCount++;
    queue.insert(line);
    ++m_totalPeople;

    return true;
}

bool QueueManager::dequeue(int lineNumber) {
    auto it = lineMap.find(lineNumber);
    if (it == lineMap.end() || it->second->peopleCount == 0) {
        return false; // Invalid line number or no people in line
    }

    auto line = it->second;
    auto setIt = queue.find(line);
    if (setIt != queue.end()) {
        queue.erase(setIt);
    }

    line->peopleCount--;
    queue.insert(line);
    --m_totalPeople;

    return true;
}

bool QueueManager::enqueueOnLine(int lineNumber) {
    if (isFull()) {
        return false;
    }

    auto it = lineMap.find(lineNumber);
    if (it == lineMap.end()) {
        return false;
    }

    auto line = it->second;
    auto setIt = queue.find(line);
    if (setIt != queue.end()) {
        queue.erase(setIt);
    }

    line->peopleCount++;
    queue.insert(line);
    ++m_totalPeople;

    return true;
}

size_t QueueManager::size() const {
    return m_totalPeople;
}

bool QueueManager::isEmpty() const {
    return m_totalPeople == 0;
}

bool QueueManager::isFull() const {
    if (m_maxSize == 0) {
        return false;
    }
    return m_totalPeople >= m_maxSize;
}

int QueueManager::getNextLineNumber() const {
    if (queue.empty()) {
        return -1;
    }
    return (*queue.begin())->lineNumber;
}

size_t QueueManager::getNumberOfLines() const {
    return m_numberOfLines;
}

int QueueManager::getLineCount(int lineNumber) const {
    auto it = lineMap.find(lineNumber);
    if (it == lineMap.end()) {
        return -1;
    }
    return it->second->peopleCount;
}

void QueueManager::setLineCount(int lineNumber, int count) {
    auto it = lineMap.find(lineNumber);
    if (it == lineMap.end()) {
        return;
    }

    auto line = it->second;
    auto setIt = queue.find(line);
    if (setIt != queue.end()) {
        queue.erase(setIt);
    }

    m_totalPeople -= line->peopleCount;
    line->peopleCount = max(0, count);
    m_totalPeople += line->peopleCount;
    queue.insert(line);
}

void QueueManager::reset() {
    for (auto &entry : lineMap) {
        auto line = entry.second;
        auto setIt = queue.find(line);
        if (setIt != queue.end()) {
            queue.erase(setIt);
        }
        m_totalPeople -= line->peopleCount;
        line->peopleCount = 0;
        queue.insert(line);
    }
    m_totalPeople = 0;
}



