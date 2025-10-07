// ESP32 Queue Management - Configuration File
// This file contains all configuration parameters for the queue management system

#ifndef CONFIG_H
#define CONFIG_H

// ===== WiFi Configuration =====
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ===== Firebase Configuration =====
#define API_KEY       "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL  "https://YOUR_PROJECT_ID.firebaseio.com"
// For anonymous authentication, leave these empty
#define USER_EMAIL    ""
#define USER_PASSWORD ""

// ===== Queue Configuration =====
// Each ESP32 board manages ONE queue - set the queue ID here
#define QUEUE_ID      "queueA"  // Change to queueB, queueC, etc. for different boards
#define QUEUE_NAME    "Queue Hub A"  // Friendly name for this queue

// Queue management parameters
#define NUMBER_OF_LINES 3        // Number of service lines in this queue
#define MAX_QUEUE_SIZE  0        // 0 = unlimited, or set a max capacity

// ===== Sensor Configuration =====
// Ultrasonic sensor pins for each line (HC-SR04)
// Each line needs a trigger and echo pin
const int ULTRASONIC_TRIGGER_PINS[NUMBER_OF_LINES] = {2, 16, 17};  // Trigger pins for lines 1, 2, 3
const int ULTRASONIC_ECHO_PINS[NUMBER_OF_LINES] = {15, 5, 18};     // Echo pins for lines 1, 2, 3

// Distance thresholds (in cm) - adjust based on your setup
#define MIN_DISTANCE_CM     10   // Minimum distance to detect a person
#define MAX_DISTANCE_CM     150  // Maximum distance for detection zone
#define PERSON_DISTANCE_CM  50   // Distance threshold to count as a person in line

// ===== Timing Configuration =====
#define SENSOR_UPDATE_INTERVAL  1000    // Read sensors every 1 second (milliseconds)
#define FIREBASE_UPDATE_INTERVAL 3000   // Update Firebase every 3 seconds (milliseconds)
#define AVERAGE_SERVICE_TIME_MS 30000   // Average time to serve one person (30 seconds)

// ===== Debugging =====
#define DEBUG_MODE true  // Set to false to disable serial debug messages

#endif // CONFIG_H
