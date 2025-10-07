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
#include "../shared/cpp/FirebaseStructureBuilder.h"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Queue manager instance
QueueManager *queueManager;

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
void debugPrint(const char *message);

// ===== Setup Function =====
void setup()
{
  Serial.begin(115200);
  debugPrint("\n\n=== ESP32 Queue Management System ===");

  // Initialize ultrasonic sensor pins
  for (int i = 0; i < NUMBER_OF_LINES; i++)
  {
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

  // Clear existing Firebase data for fresh start
  debugPrint("Clearing existing Firebase data...");
  bool clearSuccess = true;

  // Clear individual line data
  for (int i = 1; i <= NUMBER_OF_LINES; i++)
  {
    String queuePath = FirebaseStructureBuilder::getLineDataPath(i).c_str();
    if (!Firebase.RTDB.deleteNode(&fbdo, queuePath.c_str()))
    {
      clearSuccess = false;
    }
  }

  // Clear aggregated data
  String aggPath = FirebaseStructureBuilder::getAggregatedDataPath().c_str();
  if (!Firebase.RTDB.deleteNode(&fbdo, aggPath.c_str()))
  {
    clearSuccess = false;
  }

  // Also clear entire queues node for fresh start
  if (!Firebase.RTDB.deleteNode(&fbdo, "queues"))
  {
    clearSuccess = false;
  }

  if (clearSuccess)
  {
    debugPrint("✓ Firebase data cleared successfully");
  }
  else
  {
    debugPrint("⚠ Warning: Failed to clear some Firebase data");
  }

  debugPrint("Setup complete! Starting main loop...\n");
}

// ===== Main Loop =====
void loop()
{
  unsigned long currentMillis = millis();

  // Read sensors at defined interval
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
  {
    lastSensorUpdate = currentMillis;
    readSensors();
  }

  // Update Firebase at defined interval
  if (currentMillis - lastFirebaseUpdate >= FIREBASE_UPDATE_INTERVAL)
  {
    lastFirebaseUpdate = currentMillis;
    sendDataToFirebase();
  }

  // Small delay to prevent watchdog issues
  delay(10);
}

// ===== WiFi Setup =====
void setupWiFi()
{
  debugPrint("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 50)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    debugPrint("\nWiFi connection failed! Please check credentials.");
  }
}

// ===== Firebase Setup =====
void setupFirebase()
{
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
  while (!Firebase.ready() && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (Firebase.ready())
  {
    debugPrint("\nFirebase connected successfully!");
  }
  else
  {
    debugPrint("\nFirebase connection failed!");
  }
}

// ===== Read Sensors =====
void readSensors()
{
#if DEBUG_MODE
  Serial.println("\n--- Reading Sensors ---");
#endif

  bool hasChanges = false;

  for (int lineNumber = 1; lineNumber <= NUMBER_OF_LINES; lineNumber++)
  {
    int arrayIndex = lineNumber - 1;

    // Measure distance using ultrasonic sensor
    int distance = measureDistance(
        ULTRASONIC_TRIGGER_PINS[arrayIndex],
        ULTRASONIC_ECHO_PINS[arrayIndex]);

    // Estimate number of people based on distance
    int peopleCount = estimatePeopleCount(distance, lineNumber);

    // Check if count changed
    if (peopleCount != previousLineCounts[arrayIndex])
    {
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

  if (!hasChanges)
  {
#if DEBUG_MODE
    Serial.println("No changes detected");
#endif
  }
}

// ===== Measure Distance with Ultrasonic Sensor =====
int measureDistance(int trigPin, int echoPin)
{
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
  if (duration == 0)
  {
    return -1;
  }

  return distance;
}

// ===== Estimate People Count from Distance =====
int estimatePeopleCount(int distance, int lineNumber)
{
  // If sensor error or out of range, return current count
  if (distance < 0 || distance < MIN_DISTANCE_CM || distance > MAX_DISTANCE_CM)
  {
    return queueManager->getLineCount(lineNumber);
  }

  // Simple estimation: each person occupies about 50cm in the queue
  // This is a basic algorithm - you can make it more sophisticated
  int estimatedPeople = 0;

  if (distance <= PERSON_DISTANCE_CM)
  {
    // Someone is very close - at least 1 person
    estimatedPeople = 1;

    // For every additional 50cm, add another person
    if (distance > PERSON_DISTANCE_CM)
    {
      estimatedPeople += (distance - PERSON_DISTANCE_CM) / 50;
    }
  }

  return estimatedPeople;
}

// ===== Send Data to Firebase =====
void sendDataToFirebase()
{
  if (!Firebase.ready())
  {
    debugPrint("Firebase not ready, skipping update");
    return;
  }

#if DEBUG_MODE
  Serial.println("\n=== Updating Firebase ===");
#endif

  // Collect data for all lines and calculate metrics
  FirebaseStructureBuilder::LineData allLines[NUMBER_OF_LINES];
  int totalPeople = 0;

  for (int lineNumber = 1; lineNumber <= NUMBER_OF_LINES; lineNumber++)
  {
    int arrayIndex = lineNumber - 1;
    int peopleCount = queueManager->getLineCount(lineNumber);

    // Calculate throughput factor based on sensor data and historical patterns
    // For ESP32, we'll use a simplified throughput calculation
    // This could be enhanced with actual service time measurements
    double throughputFactor = 0.5; // Default: 0.5 people per second service rate

    // If there are people in line, adjust throughput based on occupancy changes
    if (peopleCount > 0)
    {
      // Simple heuristic: higher occupancy might indicate slower service
      throughputFactor = max(0.1, 0.8 - (peopleCount * 0.05));
    }

    double averageWaitTime = FirebaseStructureBuilder::calculateAverageWaitTime(
        peopleCount, throughputFactor);

    totalPeople += peopleCount;

    // Create line data object
    allLines[arrayIndex] = FirebaseStructureBuilder::LineData(
        peopleCount, throughputFactor, averageWaitTime, lineNumber);

    // Generate JSON and update Firebase for this line
    std::string jsonStr = FirebaseStructureBuilder::generateLineDataJson(allLines[arrayIndex]);
    std::string pathStr = FirebaseStructureBuilder::getLineDataPath(lineNumber);

    // Convert std::string to Arduino String for Firebase ESP Client
    String jsonArduino = jsonStr.c_str();
    String pathArduino = pathStr.c_str();

    // Parse JSON string into FirebaseJson object
    FirebaseJson json;
    json.setJsonData(jsonArduino);

    if (Firebase.RTDB.updateNode(&fbdo, pathArduino.c_str(), &json))
    {
#if DEBUG_MODE
      Serial.printf("✓ Line %d updated - People: %d, Wait: %.1fs\n",
                    lineNumber, peopleCount, averageWaitTime);
#endif
    }
    else
    {
#if DEBUG_MODE
      Serial.printf("✗ Failed to update line %d: %s\n",
                    lineNumber, fbdo.errorReason().c_str());
#endif
    }
  }

  // Calculate recommended line and update aggregated data
  int recommendedLine = FirebaseStructureBuilder::calculateRecommendedLine(
      allLines, NUMBER_OF_LINES);

  FirebaseStructureBuilder::AggregatedData aggData(
      totalPeople, NUMBER_OF_LINES, recommendedLine);

  // Generate aggregated JSON and update Firebase
  std::string aggJsonStr = FirebaseStructureBuilder::generateAggregatedDataJson(aggData);
  std::string aggPathStr = FirebaseStructureBuilder::getAggregatedDataPath();

  // Convert to Arduino String
  String aggJsonArduino = aggJsonStr.c_str();
  String aggPathArduino = aggPathStr.c_str();

  // Parse JSON string into FirebaseJson object
  FirebaseJson aggJson;
  aggJson.setJsonData(aggJsonArduino);

  if (Firebase.RTDB.updateNode(&fbdo, aggPathArduino.c_str(), &aggJson))
  {
#if DEBUG_MODE
    Serial.printf("✓ Aggregated data updated - Total: %d, Recommended: %d\n",
                  totalPeople, recommendedLine);
#endif
  }
  else
  {
#if DEBUG_MODE
    Serial.printf("✗ Failed to update aggregated data: %s\n",
                  fbdo.errorReason().c_str());
#endif
  }
}

// ===== Debug Print Helper =====
void debugPrint(const char *message)
{
#if DEBUG_MODE
  Serial.println(message);
#endif
}
