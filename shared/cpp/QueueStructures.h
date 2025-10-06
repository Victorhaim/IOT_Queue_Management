#ifndef QUEUE_STRUCTURES_H
#define QUEUE_STRUCTURES_H

#include <cstring>
#include <cstdint>

/// C++ structures for queue data that can be shared between ESP32 and Flutter
/// Designed to be FFI-compatible and memory-efficient

struct QueueLineData {
    int lineNumber;
    int peopleCount;
    int waitTimeSeconds;
    bool isRecommended;
    
    QueueLineData() : lineNumber(0), peopleCount(0), waitTimeSeconds(0), isRecommended(false) {}
    QueueLineData(int line, int people, int wait = 0, bool recommended = false)
        : lineNumber(line), peopleCount(people), waitTimeSeconds(wait), isRecommended(recommended) {}
};

struct QueueSensorData {
    static const int MAX_SENSOR_NAME_LENGTH = 32;
    static const int MAX_SENSORS = 16;
    
    struct Sensor {
        char name[MAX_SENSOR_NAME_LENGTH];
        float value;
        int64_t timestamp; // Unix timestamp in milliseconds
        
        Sensor() : value(0.0f), timestamp(0) {
            name[0] = '\0';
        }
        
        void setName(const char* sensorName) {
            strncpy(name, sensorName, MAX_SENSOR_NAME_LENGTH - 1);
            name[MAX_SENSOR_NAME_LENGTH - 1] = '\0';
        }
    };
    
    Sensor sensors[MAX_SENSORS];
    int sensorCount;
    
    QueueSensorData() : sensorCount(0) {}
    
    bool addSensor(const char* name, float value, int64_t timestamp = 0);
    float getSensorValue(const char* name) const;
    bool removeSensor(const char* name);
    void clear();
};

struct QueueData {
    static const int MAX_QUEUE_NAME_LENGTH = 64;
    static const int MAX_LINES = 10;
    
    char id[MAX_QUEUE_NAME_LENGTH];
    char name[MAX_QUEUE_NAME_LENGTH];
    int totalPeople;
    int maxCapacity;
    int numberOfLines;
    int recommendedLine;
    int64_t lastUpdated; // Unix timestamp in milliseconds
    
    QueueLineData lines[MAX_LINES];
    QueueSensorData sensorData;
    
    QueueData() : totalPeople(0), maxCapacity(0), numberOfLines(0), 
                  recommendedLine(-1), lastUpdated(0) {
        id[0] = '\0';
        name[0] = '\0';
    }
    
    void setId(const char* queueId);
    void setName(const char* queueName);
    void updateTimestamp();
    
    // Line management
    bool setLineCount(int lineNumber, int count);
    int getLineCount(int lineNumber) const;
    void calculateRecommendedLine();
    void updateTotalPeople();
    
    // Validation
    bool isValid() const;
};

// C-style interface for FFI
extern "C" {
    // QueueData operations
    QueueData* queue_data_create();
    void queue_data_destroy(QueueData* qd);
    
    void queue_data_set_id(QueueData* qd, const char* id);
    void queue_data_set_name(QueueData* qd, const char* name);
    void queue_data_set_line_count(QueueData* qd, int lineNumber, int count);
    void queue_data_update_timestamp(QueueData* qd);
    void queue_data_calculate_recommended_line(QueueData* qd);
    
    const char* queue_data_get_id(QueueData* qd);
    const char* queue_data_get_name(QueueData* qd);
    int queue_data_get_total_people(QueueData* qd);
    int queue_data_get_recommended_line(QueueData* qd);
    int queue_data_get_line_count(QueueData* qd, int lineNumber);
    int64_t queue_data_get_last_updated(QueueData* qd);
    
    // Sensor operations
    bool queue_data_add_sensor(QueueData* qd, const char* name, float value, int64_t timestamp);
    float queue_data_get_sensor_value(QueueData* qd, const char* name);
    bool queue_data_remove_sensor(QueueData* qd, const char* name);
    void queue_data_clear_sensors(QueueData* qd);
    int queue_data_get_sensor_count(QueueData* qd);
    
    // Array operations for FFI
    const QueueLineData* queue_data_get_lines_array(QueueData* qd);
    void queue_data_set_lines_from_array(QueueData* qd, const int* lineCounts, int arraySize);
}

#endif // QUEUE_STRUCTURES_H