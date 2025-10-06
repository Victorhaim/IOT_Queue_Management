import 'dart:async';
import 'package:firebase_database/firebase_database.dart';
import 'queue_data.dart';

/// Repository encapsulating all Realtime Database interactions related to queues.
class QueueRepository {
  QueueRepository({FirebaseDatabase? database})
      : _db = database ?? FirebaseDatabase.instance;

  final FirebaseDatabase _db;

  DatabaseReference _queueRef(String id) => _db.ref('queues/$id');

  /// Stream a single queue's data as QueueData.
  Stream<QueueData> watchQueue(String id) {
    final ref = _queueRef(id);
    return ref.onValue.map((event) => QueueData.fromSnapshot(id, event.snapshot));
  }

  /// Update only the queue length and timestamp.
  Future<void> updateQueueLength(String id, int length) async {
    final ref = _queueRef(id);
    await ref.update({
      'length': length,
      'updatedAt': ServerValue.timestamp,
    });
  }

  /// Batch update queue with arbitrary fields (merging JSON).
  Future<void> updateQueue(String id, Map<String, dynamic> data) async {
    final ref = _queueRef(id);
    final Map<String, dynamic> payload = Map.of(data);
    payload['updatedAt'] = ServerValue.timestamp;
    await ref.update(payload);
  }
}
