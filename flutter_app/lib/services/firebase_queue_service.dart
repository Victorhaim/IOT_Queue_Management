import 'dart:async';
import 'package:firebase_database/firebase_database.dart';

/// Firebase service for queue data operations
/// Handles Firebase Realtime Database communication for queue data
class FirebaseQueueService {
  FirebaseQueueService({FirebaseDatabase? database})
    : _database = database ?? FirebaseDatabase.instance;

  final FirebaseDatabase _database;

  /// Gets a reference to a specific queue in the database
  DatabaseReference _getQueueReference(String queueId) =>
      _database.ref('queues/$queueId');

  /// Update queue fields with raw data
  Future<void> updateQueueFields(
    String queueId,
    Map<String, dynamic> fields,
  ) async {
    final reference = _getQueueReference(queueId);
    final updateData = Map<String, dynamic>.from(fields);
    updateData['updatedAt'] = ServerValue.timestamp;
    await reference.update(updateData);
  }

  /// Stream queue updates (raw data)
  Stream<Map<String, dynamic>> watchQueueUpdatesRaw(String queueId) {
    final reference = _getQueueReference(queueId);
    return reference.onValue.map((event) {
      final rawData = (event.snapshot.value as Map?) ?? {};
      return Map<String, dynamic>.from(rawData);
    });
  }
}
