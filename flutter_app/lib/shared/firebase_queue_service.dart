import 'dart:async';
import 'package:firebase_database/firebase_database.dart';
import 'queue_structures.dart';

/// Firebase service for queue data operations shared between simulation and real ESP32 systems.
/// Provides a consistent interface for reading and writing queue data to Firebase Realtime Database.
class FirebaseQueueService {
  FirebaseQueueService({FirebaseDatabase? database})
    : _database = database ?? FirebaseDatabase.instance;

  final FirebaseDatabase _database;

  /// Gets a reference to a specific queue in the database.
  DatabaseReference _getQueueReference(String queueId) =>
      _database.ref('queues/$queueId');

  /// Streams real-time updates for a specific queue.
  /// Used by both simulation and real systems to monitor queue changes.
  Stream<QueueData> watchQueueUpdates(String queueId) {
    final reference = _getQueueReference(queueId);
    return reference.onValue.map(
      (event) => QueueData.fromFirebaseSnapshot(queueId, event.snapshot),
    );
  }

  /// Updates only the total people count and timestamp.
  /// Lightweight update for simple count changes.
  Future<void> updateTotalPeople(String queueId, int totalPeople) async {
    final reference = _getQueueReference(queueId);
    await reference.update({
      'length': totalPeople, // Keep for backward compatibility
      'totalPeople': totalPeople,
      'updatedAt': ServerValue.timestamp,
    });
  }

  /// Updates the distribution of people across queue lines.
  /// Used when line counts change due to people joining/leaving specific lines.
  Future<void> updateLineDistribution(
    String queueId,
    Map<String, int> lineDistribution,
  ) async {
    final reference = _getQueueReference(queueId);
    await reference.update({
      'lines': lineDistribution,
      'updatedAt': ServerValue.timestamp,
    });
  }

  /// Updates sensor data from ESP32 devices.
  /// Used by real ESP32 implementation to send sensor readings.
  Future<void> updateSensorData(
    String queueId,
    Map<String, dynamic> sensorData,
  ) async {
    final reference = _getQueueReference(queueId);
    await reference.update({
      'sensors': sensorData,
      'updatedAt': ServerValue.timestamp,
    });
  }

  /// Updates the recommended line number based on queue algorithms.
  /// Called when queue optimization calculations determine a new optimal line.
  Future<void> updateRecommendedLine(
    String queueId,
    int recommendedLine,
  ) async {
    final reference = _getQueueReference(queueId);
    await reference.update({
      'recommendedLine': recommendedLine,
      'updatedAt': ServerValue.timestamp,
    });
  }

  /// Performs a complete queue data update with all fields.
  /// Used for bulk updates or initialization.
  Future<void> updateCompleteQueueData(
    String queueId,
    QueueData queueData,
  ) async {
    final reference = _getQueueReference(queueId);
    final dataMap = queueData.toFirebaseMap();
    dataMap['updatedAt'] = ServerValue.timestamp;
    await reference.update(dataMap);
  }

  /// Batch update with custom data fields.
  /// Flexible method for partial updates with automatic timestamp.
  Future<void> updateQueueFields(
    String queueId,
    Map<String, dynamic> fields,
  ) async {
    final reference = _getQueueReference(queueId);
    final updateData = Map<String, dynamic>.from(fields);
    updateData['updatedAt'] = ServerValue.timestamp;
    await reference.update(updateData);
  }

  /// Gets a one-time snapshot of queue data.
  /// Useful for initialization or non-reactive data fetching.
  Future<QueueData?> getQueueSnapshot(String queueId) async {
    final reference = _getQueueReference(queueId);
    final snapshot = await reference.get();

    if (snapshot.exists) {
      return QueueData.fromFirebaseSnapshot(queueId, snapshot);
    }
    return null;
  }

  /// Initializes a new queue with default values.
  /// Used during queue setup for both simulation and real systems.
  Future<void> initializeQueue(
    String queueId, {
    String? name,
    int numberOfLines = 3,
    Map<String, dynamic>? initialSensorData,
  }) async {
    final reference = _getQueueReference(queueId);

    // Create initial line distribution with zero people
    final Map<String, int> initialLines = {};
    for (int i = 1; i <= numberOfLines; i++) {
      initialLines[i.toString()] = 0;
    }

    await reference.set({
      'name': name ?? 'Queue $queueId',
      'length': 0,
      'totalPeople': 0,
      'lines': initialLines,
      'sensors': initialSensorData ?? {},
      'recommendedLine': 1, // Default to first line
      'updatedAt': ServerValue.timestamp,
    });
  }
}
