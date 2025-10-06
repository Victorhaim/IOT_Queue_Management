# Firebase Cloud Functions for Queue Management

Implements server-side authoritative queue logic so that Flutter and ESP32 clients do not have to compute `recommendedLine`.

## Features
- RTDB trigger `onLineCountWrite`: Recomputes `recommendedLine`, `length` (total people), and updates `updatedAt` when any `/queues/{queueId}/lines/{lineNumber}` changes.
- Callable `enqueueAuto`: Atomically enqueues a user into the least-populated line.
- Callable `enqueueOnLine`: Atomically enqueues a user into a specific line.

## Data Model (RTDB)
```
queues/
  queueA/
    lines: { "1": 3, "2": 5, "3": 2 }
    recommendedLine: 3
    length: 10
    updatedAt: <server timestamp>
```

## Deploy Steps
1. Install Firebase CLI if not already:
   ```bash
   npm install -g firebase-tools
   ```
2. Login & set project:
   ```bash
   firebase login
   firebase use iot-queue-management
   ```
3. Install dependencies:
   ```bash
   cd functions
   npm install
   ```
4. Build & deploy:
   ```bash
   npm run deploy
   ```

## Local Emulation (optional)
```bash
firebase emulators:start --only functions,database
```

## Flutter Client (enqueue auto example)
```dart
import 'package:cloud_functions/cloud_functions.dart';

final result = await FirebaseFunctions.instance.httpsCallable('enqueueAuto').call({
  'queueId': 'queueA',
});
print(result.data); // {recommendedLine: X, total: Y, lines: {...}}
```

## ESP32 Option
Send only line increments to `queues/queueA/lines/<lineNumber>`; let the trigger maintain derived fields.

## Alternative: Cloud Run + C++
You can wrap the existing C++ QueueManager in a minimal REST container (see docs section 10.2) if you prefer compiled logic. Cloud Functions TS version is simpler to iterate on.
