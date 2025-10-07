#pragma once

#include <cstring>
#include <cstdint>
#include <vector>

/// C++ structures for queue data that can be shared between ESP32 and Flutter
/// Designed to be FFI-compatible and memory-efficient

struct QueueLineData
{
    int lineNumber;
    int peopleCount;
    int waitTimeSeconds;
    bool isRecommended;

    QueueLineData() : lineNumber(0), peopleCount(0), waitTimeSeconds(0), isRecommended(false) {}
    QueueLineData(int line, int people, int wait = 0, bool recommended = false)
        : lineNumber(line), peopleCount(people), waitTimeSeconds(wait), isRecommended(recommended) {}
};

struct QueueSensorData
{
    static const int MAX_SENSOR_NAME_LENGTH = 32;
    static const int MAX_SENSORS = 16;

    struct Sensor
    {
        char name[MAX_SENSOR_NAME_LENGTH];
        float value;
        int64_t timestamp; // Unix timestamp in milliseconds

        Sensor() : value(0.0f), timestamp(0) { name[0] = '\0'; }
        void setName(const char *sensorName)
        {
            strncpy(name, sensorName, MAX_SENSOR_NAME_LENGTH - 1);
            name[MAX_SENSOR_NAME_LENGTH - 1] = '\0';
        }
    };

    std::vector<Sensor> sensors; // capacity reserved to MAX_SENSORS; size() is active count

    QueueSensorData() : sensors() { sensors.reserve(MAX_SENSORS); }

    bool addSensor(const char *name, float value, int64_t timestamp = 0);
    float getSensorValue(const char *name) const;
    bool removeSensor(const char *name);
    void clear();
};

struct QueueData
{
    static const int MAX_QUEUE_NAME_LENGTH = 64;
    static const int MAX_LINES = 10;

    char id[MAX_QUEUE_NAME_LENGTH];
    char name[MAX_QUEUE_NAME_LENGTH];
    int totalPeople;
    int maxCapacity;
    int numberOfLines;
    int recommendedLine;
    int64_t lastUpdated; // Unix timestamp in milliseconds

    std::vector<QueueLineData> lines; // reserve MAX_LINES; size() == numberOfLines
    QueueSensorData sensorData;

    QueueData() : totalPeople(0), maxCapacity(0), numberOfLines(0),
                  recommendedLine(-1), lastUpdated(0), lines()
    {
        id[0] = '\0';
        name[0] = '\0';
        lines.reserve(MAX_LINES);
    }

    void setId(const char *queueId);
    void setName(const char *queueName);
    void updateTimestamp();

    // Line management
    bool setLineCount(int lineNumber, int count);
    int getLineCount(int lineNumber) const;
    void calculateRecommendedLine();
    void updateTotalPeople();

    // Validation
    bool isValid() const;
};
