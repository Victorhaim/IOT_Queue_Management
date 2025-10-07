#include "QueueStructures.h"
#include <cstdlib>
#include <ctime>

// QueueSensorData implementation (vector-based)
bool QueueSensorData::addSensor(const char* name, float value, int64_t timestamp) {
    if (!name) return false;
    // Update existing
    for (auto &s : sensors) {
        if (strcmp(s.name, name) == 0) {
            s.value = value;
            s.timestamp = timestamp;
            return true;
        }
    }
    // Enforce capacity
    if (sensors.size() >= static_cast<size_t>(MAX_SENSORS)) return false;
    Sensor s; s.setName(name); s.value = value; s.timestamp = timestamp; sensors.push_back(s);
    return true;
}

float QueueSensorData::getSensorValue(const char* name) const {
    if (!name) return 0.0f;
    for (const auto &s : sensors) {
        if (strcmp(s.name, name) == 0) return s.value;
    }
    return 0.0f;
}

bool QueueSensorData::removeSensor(const char* name) {
    if (!name) return false;
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        if (strcmp(it->name, name) == 0) {
            sensors.erase(it); // vector maintains contiguity
            return true;
        }
    }
    return false;
}

void QueueSensorData::clear() {
    sensors.clear();
}

// QueueData implementation
void QueueData::setId(const char* queueId) {
    if (queueId) {
        strncpy(id, queueId, MAX_QUEUE_NAME_LENGTH - 1);
        id[MAX_QUEUE_NAME_LENGTH - 1] = '\0';
    }
}

void QueueData::setName(const char* queueName) {
    if (queueName) {
        strncpy(name, queueName, MAX_QUEUE_NAME_LENGTH - 1);
        name[MAX_QUEUE_NAME_LENGTH - 1] = '\0';
    }
}

void QueueData::updateTimestamp() {
    // Get current time in milliseconds
    lastUpdated = static_cast<int64_t>(time(nullptr)) * 1000;
}

bool QueueData::setLineCount(int lineNumber, int count) {
    if (lineNumber < 1 || lineNumber > numberOfLines || lineNumber > MAX_LINES) return false;
    int index = lineNumber - 1;
    if (static_cast<int>(lines.size()) < numberOfLines) {
        // Ensure vector populated up to numberOfLines if not already
        while (static_cast<int>(lines.size()) < numberOfLines) {
            lines.push_back(QueueLineData(static_cast<int>(lines.size()) + 1, 0));
        }
    }
    lines[index].lineNumber = lineNumber;
    lines[index].peopleCount = (count < 0) ? 0 : count;
    updateTotalPeople();
    calculateRecommendedLine();
    updateTimestamp();
    return true;
}

int QueueData::getLineCount(int lineNumber) const {
    if (lineNumber < 1 || lineNumber > numberOfLines || lineNumber > MAX_LINES) return -1;
    int index = lineNumber - 1;
    if (index >= static_cast<int>(lines.size())) return -1;
    return lines[index].peopleCount;
}

void QueueData::calculateRecommendedLine() {
    if (numberOfLines == 0 || lines.empty()) { recommendedLine = -1; return; }
    // Ensure lines vector has at least numberOfLines elements
    if (static_cast<int>(lines.size()) < numberOfLines) return; // can't calculate consistently yet
    int minPeople = lines[0].peopleCount;
    int bestLine = 1;
    for (int i = 0; i < numberOfLines; ++i) lines[i].isRecommended = false;
    for (int i = 1; i < numberOfLines; ++i) {
        if (lines[i].peopleCount < minPeople) { minPeople = lines[i].peopleCount; bestLine = i + 1; }
    }
    recommendedLine = bestLine;
    if (bestLine >= 1 && bestLine <= numberOfLines) lines[bestLine - 1].isRecommended = true;
}

void QueueData::updateTotalPeople() {
    totalPeople = 0;
    int limit = std::min(numberOfLines, static_cast<int>(lines.size()));
    for (int i = 0; i < limit; i++) totalPeople += lines[i].peopleCount;
}

bool QueueData::isValid() const {
    return id[0] != '\0' && numberOfLines > 0 && numberOfLines <= MAX_LINES;
}

// C-style interface for FFI
extern "C" {
    QueueData* queue_data_create() {
        return new QueueData();
    }

    void queue_data_destroy(QueueData* qd) {
        delete qd;
    }

    void queue_data_set_id(QueueData* qd, const char* id) {
        if (qd) {
            qd->setId(id);
        }
    }

    void queue_data_set_name(QueueData* qd, const char* name) {
        if (qd) {
            qd->setName(name);
        }
    }

    void queue_data_set_line_count(QueueData* qd, int lineNumber, int count) {
        if (qd) {
            qd->setLineCount(lineNumber, count);
        }
    }

    void queue_data_update_timestamp(QueueData* qd) {
        if (qd) {
            qd->updateTimestamp();
        }
    }

    void queue_data_calculate_recommended_line(QueueData* qd) {
        if (qd) {
            qd->calculateRecommendedLine();
        }
    }

    const char* queue_data_get_id(QueueData* qd) {
        return qd ? qd->id : nullptr;
    }

    const char* queue_data_get_name(QueueData* qd) {
        return qd ? qd->name : nullptr;
    }

    int queue_data_get_total_people(QueueData* qd) {
        return qd ? qd->totalPeople : 0;
    }

    int queue_data_get_recommended_line(QueueData* qd) {
        return qd ? qd->recommendedLine : -1;
    }

    int queue_data_get_line_count(QueueData* qd, int lineNumber) {
        return qd ? qd->getLineCount(lineNumber) : -1;
    }

    int64_t queue_data_get_last_updated(QueueData* qd) {
        return qd ? qd->lastUpdated : 0;
    }

    bool queue_data_add_sensor(QueueData* qd, const char* name, float value, int64_t timestamp) {
        return qd ? qd->sensorData.addSensor(name, value, timestamp) : false;
    }

    float queue_data_get_sensor_value(QueueData* qd, const char* name) {
        return qd ? qd->sensorData.getSensorValue(name) : 0.0f;
    }

    bool queue_data_remove_sensor(QueueData* qd, const char* name) {
        return qd ? qd->sensorData.removeSensor(name) : false;
    }

    void queue_data_clear_sensors(QueueData* qd) {
        if (qd) {
            qd->sensorData.clear();
        }
    }

    int queue_data_get_sensor_count(QueueData* qd) {
        return qd ? static_cast<int>(qd->sensorData.sensors.size()) : 0;
    }

    const QueueLineData* queue_data_get_lines_array(QueueData* qd) {
        if (!qd) return nullptr;
        if (qd->lines.empty()) return nullptr;
        return qd->lines.data();
    }

    void queue_data_set_lines_from_array(QueueData* qd, const int* lineCounts, int arraySize) {
        if (!qd || !lineCounts) return;
        
        int maxLines = (arraySize < qd->numberOfLines) ? arraySize : qd->numberOfLines;
        for (int i = 0; i < maxLines; i++) {
            qd->setLineCount(i + 1, lineCounts[i]);
        }
    }
}