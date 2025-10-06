import 'dart:async';
import 'dart:math';
import '../native/queue_manager_ffi.dart';
import '../services/firebase_queue_service.dart';

/// Simulates ESP32 queue + sensor updates by writing to Firebase RTDB
/// Uses C++ queue logic via FFI (same as ESP32) and Firebase for data sync
class QueueSimulator {
  QueueSimulator({
    FirebaseQueueService? queueService,
    this.queueId = 'queueA',
    Duration? interval,
    int numberOfLines = 3,
    int maxSize = 0,
  })  : _queueService = queueService ?? FirebaseQueueService(),
        _interval = interval ?? const Duration(seconds: 3),
        _queueManager = QueueManagerFFI(maxSize, numberOfLines);

  final FirebaseQueueService _queueService;
  final String queueId;
  final Duration _interval;
  final QueueManagerFFI _queueManager; // Using C++ implementation via FFI!
  final _rand = Random();

  Timer? _timer;
  bool get isRunning => _timer?.isActive ?? false;

  /// Start simulation loop.
  void start() {
    if (isRunning) return;
    // Initial prime write to ensure node exists
    _writeUpdate();
    _timer = Timer.periodic(_interval, (_) => _writeUpdate());
  }

  /// Stop simulation loop.
  void stop() {
    _timer?.cancel();
    _timer = null;
  }

  /// Cleanup FFI resources
  void dispose() {
    stop();
    _queueManager.dispose();
  }

  void _writeUpdate() async {
    // Randomly decide operations for each tick using C++ QueueManager
    final ops = _rand.nextInt(3) + 1; // 1..3 operations
    for (var i = 0; i < ops; i++) {
      if (_rand.nextBool()) {
        // Enqueue to auto-selected line (C++ chooses best line)
        _queueManager.enqueue();
      } else {
        // Occasionally dequeue from a random line
        final line = _rand.nextInt(_queueManager.numberOfLines) + 1;
        if (_rand.nextBool()) {
          _queueManager.dequeue(line);
        } else {
          _queueManager.enqueueOnLine(line);
        }
      }
    }

    // Generate sensor-ish values per line using C++ data
    final sensorMap = <String, dynamic>{};
    for (var line = 1; line <= _queueManager.numberOfLines; line++) {
      sensorMap['line$line'] = {
        'people': _queueManager.getLineCount(line),
        'ultrasonic': 100 + _rand.nextInt(60),
      };
    }

    // Use C++ queue logic for all decisions - NO Dart duplicates!
    final queueData = _queueManager.toJson();

    await _queueService.updateQueueFields(queueId, {
      'name': 'Queue Hub (Simulated)',
      'length': queueData['totalPeople'],
      'totalPeople': queueData['totalPeople'],
      'lines': queueData['lines'],
      'recommendedLine': queueData['recommendedLine'],
      'sensors': sensorMap,
    });
  }
}
