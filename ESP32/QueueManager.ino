/*
 * ESP32 Queue Management System
 * 
 * This firmware runs on ESP32 boards to manage individual queue monitoring.
 * Each ESP32:
 * - Reads ultrasonic sensors to detect people in each line
 * - Calculates queue statistics using QueueManager
 * - Sends computed results to Firebase Realtime Database
 * - Flutter app reads this data and displays the best queue
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - HC-SR04 ultrasonic sensors (one per queue line)
 * - Stable WiFi connection
 * 
 * Libraries Required:
 * - Firebase ESP Client by Mobizt
 * - WiFi (built-in)
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "config.h"
#include "../shared/cpp/QueueManager.h"
#include "../shared/cpp/QueueStructures.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Queue manager instance
QueueManager* queueManager;

// Timing variables
unsigned long lastSensorUpdate = 0;
unsigned long lastFirebaseUpdate = 0;

// Previous line counts for change detection
int previousLineCounts[NUMBER_OF_LINES] = {0};

// ===== Function Declarations =====
void setupWiFi();
void setupFirebase();
void readSensors();
int measureDistance(int trigPin, int echoPin);
int estimatePeopleCount(int distance, int lineNumber);
void updateQueueManager();
void sendDataToFirebase();
int calculateWaitTime(int lineNumber);
void debugPrint(const char* message);

// ===== Setup Function =====
void setup() {
  Serial.begin(115200);
  debugPrint("\n\n=== ESP32 Queue Management System ===");
  
  // Initialize ultrasonic sensor pins
  for (int i = 0; i < NUMBER_OF_LINES; i++) {
    pinMode(ULTRASONIC_TRIGGER_PINS[i], OUTPUT);
    pinMode(ULTRASONIC_ECHO_PINS[i], INPUT);
  }
  debugPrint("Ultrasonic sensors initialized");
  
  // Initialize queue manager
  queueManager = new QueueManager(MAX_QUEUE_SIZE, NUMBER_OF_LINES);
  debugPrint("QueueManager initialized");
  
  // Connect to WiFi
  setupWiFi();
  
  // Setup Firebase
  setupFirebase();
  
  debugPrint("Setup complete! Starting main loop...\n");
}

// ===== Main Loop =====
void loop() {
  unsigned long currentMillis = millis();
  
  // Read sensors at defined interval
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL) {
    lastSensorUpdate = currentMillis;
    readSensors();
  }
  
  // Update Firebase at defined interval
  if (currentMillis - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL) {
    lastFirebaseUpdate = currentMillis;
    sendDataToFirebase();
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}

// ===== WiFi Setup =====
void setupWiFi() {
  debugPrint("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 50) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    debugPrint("\nWiFi connection failed! Please check credentials.");
  }
}

// ===== Firebase Setup =====
void setupFirebase() {
  debugPrint("Setting up Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Anonymous authentication
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  // Assign the callback function for token generation task
  config.token_status_callback = tokenStatusCallback;
  
  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  // Wait for Firebase to be ready
  int attempts = 0;
  while (!Firebase.ready() && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (Firebase.ready()) {
    debugPrint("\nFirebase connected successfully!");
  } else {
    debugPrint("\nFirebase connection failed!");
  }
}

// ===== Read Sensors =====
void readSensors() {
  #if DEBUG_MODE
    Serial.println("\n--- Reading Sensors ---");
  #endif
  
  bool hasChanges = false;
  
  for (int lineNumber = 1; lineNumber <= NUMBER_OF_LINES; lineNumber++) {
    int arrayIndex = lineNumber - 1;
    
    // Measure distance using ultrasonic sensor
    int distance = measureDistance(
      ULTRASONIC_TRIGGER_PINS[arrayIndex],
      ULTRASONIC_ECHO_PINS[arrayIndex]
    );
    
    // Estimate number of people based on distance
    int peopleCount = estimatePeopleCount(distance, lineNumber);
    
    // Check if count changed
    if (peopleCount != previousLineCounts[arrayIndex]) {
      hasChanges = true;
      
      #if DEBUG_MODE
        Serial.printf("Line %d: %d people (was %d) - Distance: %d cm\n",
          lineNumber, peopleCount, previousLineCounts[arrayIndex], distance);
      #endif
      
      // Update queue manager
      queueManager->setLineCount(lineNumber, peopleCount);
      previousLineCounts[arrayIndex] = peopleCount;
    }
  }
  
  if (!hasChanges) {
    #if DEBUG_MODE
      Serial.println("No changes detected");
    #endif
  }
}

// ===== Measure Distance with Ultrasonic Sensor =====
int measureDistance(int trigPin, int echoPin) {
  // Clear trigger pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send 10 microsecond pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read echo pin
  long duration = pulseIn(echoPin, HIGH, 30000); // 30ms timeout
  
  // Calculate distance in cm (duration/2 because sound travels there and back)
  int distance = duration * 0.034 / 2;
  
  // Return -1 if no echo received (timeout)
  if (duration == 0) {
    return -1;
  }
  
  return distance;
}

// ===== Estimate People Count from Distance =====
int estimatePeopleCount(int distance, int lineNumber) {
  // If sensor error or out of range, return current count
  if (distance < 0 || distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM) {
    return queueManager->getLineCount(lineNumber);
  }
  
  // Simple estimation: each person occupies about 50cm in the queue
  // This is a basic algorithm - you can make it more sophisticated
  int estimatedPeople = 0;
  
  if (distance <= PERSON_DISTANCE_CM) {
    // Someone is very close - at least 1 person
    estimatedPeople = 1;
    
    // For every additional 50cm, add another person
    if (distance > PERSON_DISTANCE_CM) {
      estimatedPeople += (distance - PERSON_DISTANCE_CM) / 50;
    }
  }
  
  return estimatedPeople;
}

// ===== Calculate Wait Time for a Line =====
int calculateWaitTime(int lineNumber) {
  int peopleCount = queueManager->getLineCount(lineNumber);
  // Wait time = number of people * average service time (in seconds)
  return (peopleCount * AVERAGE_SERVICE_TIME_MS) / 1000;
}

// ===== Send Data to Firebase =====
void sendDataToFirebase() {
  if (!Firebase.ready()) {
    debugPrint("Firebase not ready, skipping update");
    return;
  }
  
  #if DEBUG_MODE
    Serial.println("\n=== Updating Firebase ===");
  #endif
  
  // Prepare path for this queue
  String basePath = String("/queues/") + QUEUE_ID;
  
  // Create JSON object with all queue data
  FirebaseJson json;
  
  // Basic info
  json.set("name", QUEUE_NAME);
  json.set("length", (int)queueManager->size());
  json.set("recommendedLine", queueManager->getNextLineNumber());
  
  // Lines data with wait times
  FirebaseJson linesJson;
  for (int lineNumber = 1; lineNumber <= NUMBER_OF_LINES; lineNumber++) {
    int peopleCount = queueManager->getLineCount(lineNumber);
    int waitTime = calculateWaitTime(lineNumber);
    
    String lineKey = String(lineNumber);
    linesJson.set(lineKey + "/people", peopleCount);
    linesJson.set(lineKey + "/waitTime", waitTime);
  }
  json.set("lines", linesJson);
  
  // Sensor data (raw distances)
  FirebaseJson sensorsJson;
  for (int lineNumber = 1; lineNumber <= NUMBER_OF_LINES; lineNumber++) {
    int arrayIndex = lineNumber - 1;
    int distance = measureDistance(
      ULTRASONIC_TRIGGER_PINS[arrayIndex],
      ULTRASONIC_ECHO_PINS[arrayIndex]
    );
    
    String sensorKey = "line" + String(lineNumber);
    sensorsJson.set(sensorKey + "/distance", distance);
    sensorsJson.set(sensorKey + "/people", queueManager->getLineCount(lineNumber));
  }
  json.set("sensors", sensorsJson);
  
  // Timestamp (server-side)
  json.set("updatedAt/.sv", "timestamp");
  
  // Send to Firebase
  if (Firebase.RTDB.updateNode(&fbdo, basePath.c_str(), &json)) {
    #if DEBUG_MODE
      Serial.println("✓ Firebase updated successfully");
      Serial.printf("  Total people: %d\n", queueManager->size());
      Serial.printf("  Recommended line: %d\n", queueManager->getNextLineNumber());
    #endif
  } else {
    #if DEBUG_MODE
      Serial.print("✗ Firebase update failed: ");
      Serial.println(fbdo.errorReason().c_str());
    #endif
  }
}

// ===== Debug Print Helper =====
void debugPrint(const char* message) {
  #if DEBUG_MODE
    Serial.println(message);
  #endif
}
