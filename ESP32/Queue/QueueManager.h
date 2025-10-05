#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>

using namespace std;


class QueueManager {

public:
    QueueManager(size_t maxSize, size_t numberOfLines);

    ~QueueManager() = default;

    bool enqueue();

    bool dequeue(int lineNumber);

    bool enqueueOnLine(int lineNumber);

    size_t size() const;

    bool isEmpty() const;

    bool isFull() const;

    int getNextLineNumber() const;

    size_t getNumberOfLines() const;

    int getLineCount(int lineNumber) const;

    void setLineCount(int lineNumber, int count);

    void reset();

private:

    struct Line {
        int lineNumber;
        int peopleCount;
    };

    struct LineComparator {
        bool operator()(const shared_ptr<Line>& lhs, const shared_ptr<Line>& rhs) const {
            if (lhs->peopleCount == rhs->peopleCount) {
                return lhs->lineNumber < rhs->lineNumber;
            }
            return lhs->peopleCount < rhs->peopleCount;
        }
    };

    size_t m_maxSize;
    size_t m_numberOfLines;
    size_t m_totalPeople;
    set<shared_ptr<Line>, LineComparator> queue;
    unordered_map<int, shared_ptr<Line>> lineMap; // To track lines by their number

};















