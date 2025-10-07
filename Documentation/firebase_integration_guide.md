# Firebase Integration Guide for C++ (ESP32) and Flutter

This document explains how to send and receive data between an **ESP32 (C++/Arduino)** backend and a **Flutter** frontend using **Firebase Realtime Database (RTDB)** or **Firestore**.

---

## 1. Choose Your Database Type

| Use Case | Realtime Database (RTDB) | Firestore |
|-----------|--------------------------|------------|
| Real-time updates | ✅ Excellent | ✅ Excellent |
| Data structure | JSON tree | Documents / Collections |
| Simplicity on ESP32 | ✅ Very easy | ⚠️ More complex |
| Cost model | Bandwidth + storage | Reads + writes + storage |
| Recommended for queue sensors | ✅ **RTDB** | Optional |

For IoT queue management projects (ESP32 + sensors), **Firebase Realtime Database** is simpler and faster to implement.

---

## 2. Flutter Setup (Frontend)

### 2.1 Add Dependencies

Add the following to your `pubspec.yaml`:

```yaml
dependencies:
  firebase_core: ^latest
  firebase_database: ^latest
  # or for Firestore:
  # cloud_firestore: ^latest
```

### 2.2 Initialize Firebase

In `main.dart`:

```dart
import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:firebase_database/firebase_database.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(); // make sure you configured firebase_options.dart
  runApp(const MyApp());
}
```

### 2.3 Read Data (Realtime Stream)

```dart
class QueueView extends StatelessWidget {
  final String queueId;
  const QueueView({super.key, required this.queueId});

  @override
  Widget build(BuildContext context) {
    final ref = FirebaseDatabase.instance.ref('queues/$queueId');

    return StreamBuilder(
      stream: ref.onValue,
      builder: (context, snapshot) {
        if (!snapshot.hasData) return const CircularProgressIndicator();
        final event = snapshot.data as DatabaseEvent;
        final data = (event.snapshot.value as Map?) ?? {};
        final length = data['length'] ?? 0;
        final name = data['name'] ?? 'Unknown';

        return ListTile(
          title: Text(name),
          subtitle: Text('Queue length: $length'),
        );
      },
    );
  }
}
```

### 2.4 Write Data from Flutter

```dart
Future<void> updateQueueLength(String queueId, int length) async {
  final ref = FirebaseDatabase.instance.ref('queues/$queueId');
  await ref.update({
    'length': length,
    'updatedAt': ServerValue.timestamp,
  });
}
```

---

## 3. ESP32 Setup (C++/Arduino)

### 3.1 Install Dependencies

- **Arduino IDE or PlatformIO**
- Library: [Firebase ESP Client by Mobizt](https://github.com/mobizt/Firebase-ESP-Client)

Install it via Arduino Library Manager.

---

### 3.2 ESP32 Example Code (RTDB)

```cpp
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID     "YOUR_WIFI"
#define WIFI_PASSWORD "YOUR_PASS"
#define API_KEY       "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL  "https://<your-project-id>.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println("\nWiFi connected.");

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Anonymous authentication
  auth.user.email = "";
  auth.user.password = "";

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  int queueLength = 7; // Example sensor data

  if (Firebase.RTDB.setInt(&fbdo, "/queues/queueA/length", queueLength)) {
    Serial.println("Queue length updated");
  } else {
    Serial.printf("Error: %s\n", fbdo.errorReason().c_str());
  }

  FirebaseJson json;
  json.set("updatedAt/.sv", "timestamp");
  Firebase.RTDB.updateNode(&fbdo, "/queues/queueA", &json);

  delay(2000);
}
```

---

## 4. Database Structure Example

```json
{
  "queues": {
    "queueA": {
      "name": "Entrance A",
      "length": 7,
      "sensors": {
        "ultrasonic1": 123,
        "ultrasonic2": 118
      },
      "updatedAt": 1730820000000
    },
    "queueB": {
      "name": "Entrance B",
      "length": 2,
      "updatedAt": 1730820050000
    }
  }
}
```

---

## 5. Security Rules (RTDB)

### Development (unsafe, use only for testing)
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

### Production (basic authentication)
```json
{
  "rules": {
    "queues": {
      "$id": {
        ".read": "auth != null",
        ".write": "auth != null"
      }
    }
  }
}
```

For production, consider:
- Firebase Authentication (anonymous or email/password)
- A secure proxy via **Cloud Run / Cloud Functions**

---

## 6. Optimization Tips

- Send updates **only when data changes**
- Use `updateNode` to batch writes
- Cache previous values locally
- For many sensors, group them under `/sensors` and send one JSON update

---

## 7. Testing Locally

- Simulate ESP32 sensor values and send fake data
- Use `StreamBuilder` in Flutter to visualize updates live
- For unit testing, mock FirebaseDatabase with a fake client

---

## 8. Recommended Workflow for Queue Management System

1. **ESP32** updates `/queues/<id>` with length and sensor data  
2. **Firebase RTDB** synchronizes instantly  
3. **Flutter app** listens to `/queues` using `onValue`  
4. **Users** see live queue lengths and updates in real time  


## 9. Next Steps



## 10. Implemented in This Repository

The Flutter app already includes:

- `lib/firebase_options.dart` – Generated Firebase configuration.
- Realtime Database test writes inside `QueueScreen` (`queue_components.dart`).
- A production-style streaming widget `QueueView` (`lib/widgets/queue_view.dart`) that listens to `queues/<id>` and shows name, length, sensor map, and provides an increment demo.
- A `QueueRepository` (`lib/controllers/queue_repository.dart`) encapsulating RTDB operations (`watchQueue`, `updateQueueLength`, `updateQueue`).
- A `QueueData` model (`lib/controllers/queue_data.dart`).
- Navigation button on the main screen: “Open QueueA Live View”. Create the node `queues/queueA` in RTDB to see live updates.

To test quickly, write an object in RTDB at `queues/queueA`:
```json
{
  "name": "Entrance A",
  "length": 3,
  "sensors": {"ultrasonic1": 120},
  "updatedAt": 0
}
```

Then open the QueueA view and press “Increment Length” to observe real-time updates.

### 10.1 ESP32 Simulation (No Hardware Needed)

A built-in simulator can generate queue + sensor updates to mimic an ESP32:

- File: `lib/controllers/queue_simulator.dart`
- UI Control: "Start Simulation" / "Stop Simulation" button on the main screen.
- Behavior: Every ~3s it writes randomized `length`, two ultrasonic readings, and `occupancy` under `queues/queueA`.

Usage:
1. Run the Flutter app.
2. Tap "Start Simulation".
3. Open "Open QueueA Live View" to watch streaming changes.
4. Stop when done to prevent extra writes.

This allows validating the UI and RTDB structure without an ESP32 connected.

### 10.2 Multi-Line Queue Recommendation

The simulator now publishes additional fields under `queues/queueA`:

```json
{
  "lines": { "1": 3, "2": 5, "3": 2 },
  "recommendedLine": 3,
  "totalPeople": 10
}
```

Logic:
- `recommendedLine` = line number with fewest people (ties -> smallest id).
- `lines` maps each line to its current people count.
- The main circle on the home screen streams `recommendedLine` in real time.

To integrate with a real ESP32 firmware, mirror updates to:
`/queues/queueA/lines/<lineNumber>` and recompute `recommendedLine` either on-device or via a Cloud Function.

### 10.3 Server Authoritative Logic (Cloud Functions)

Implemented under `functions/` (TypeScript):
- Trigger `onLineCountWrite` recomputes `recommendedLine`, `length`, and `updatedAt` whenever a line changes.
- Callable `enqueueAuto` picks the least populated line and increments it atomically.
- Callable `enqueueOnLine` increments a specified line atomically.

Why server logic?
- Prevents race conditions with multiple clients.
- Avoids trusting Flutter or ESP32 to compute global state.
- Allows future scaling (Cloud Functions or Cloud Run replacement).

Flutter can call:
```dart
final r = await FirebaseFunctions.instance.httpsCallable('enqueueAuto').call({'queueId':'queueA'});
```

ESP32 can just write increments to `/queues/queueA/lines/<n>`; trigger handles derived data.


**Author:** Victor A.  
**Last Updated:** October 2025  
**Usage:** Internal – Queue Management Project
