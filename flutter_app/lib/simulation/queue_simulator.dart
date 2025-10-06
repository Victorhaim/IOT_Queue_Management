import 'dart:async';
import 'dart:math';
import '../native/queue_manager_wasm.dart';
import '../services/firebase_queue_service.dart';

/// Simulates ESP32 queue + sensor updates by writing to Firebase RTDB
/// Uses C++ queue logic via WASM (same as ESP32) and Firebase for data sync
class QueueSimulator {
  QueueSimulator({
    FirebaseQueueService? queueService,
    this.queueId = 'queueA',
    Duration? interval,
    int numberOfLines = 3,
    int maxSize = 0,
  }) : _queueService = queueService ?? FirebaseQueueService(),
       _interval = interval ?? const Duration(seconds: 3),
       _numberOfLines = numberOfLines,
       _maxSize = maxSize;

  final FirebaseQueueService _queueService;
  final String queueId;
  final Duration _interval;
  final int _numberOfLines;
  final int _maxSize;
  QueueManagerWASM? _queueManager; // Using C++ implementation via WASM!
  final _rand = Random();

  Timer? _timer;
  bool get isRunning => _timer?.isActive ?? false;

  /// Initialize the WASM module and create queue manager instance
  Future<void> initialize() async {
    if (_queueManager != null) return;

    // Initialize WASM module if not already done
    await QueueManagerWASM.initialize();

    // Create the queue manager instance
    _queueManager = QueueManagerWASM(_maxSize, _numberOfLines);
  }

  /// Start simulation loop.
  Future<void> start() async {
    if (isRunning) return;

    // Ensure WASM is initialized
    await initialize();

    // Initial prime write to ensure node exists
    await _writeUpdate();
    _timer = Timer.periodic(_interval, (_) => _writeUpdate());
  }

  /// Stop simulation loop.
  void stop() {
    _timer?.cancel();
    _timer = null;
  }

  /// Cleanup WASM resources
  void dispose() {
    stop();
    _queueManager?.dispose();
  }

  Future<void> _writeUpdate() async {
    final queueManager = _queueManager;
    if (queueManager == null) return;

    // Randomly decide operations for each tick using C++ QueueManager
    final ops = _rand.nextInt(3) + 1; // 1..3 operations
    for (var i = 0; i < ops; i++) {
      if (_rand.nextBool()) {
        // Enqueue to auto-selected line (C++ chooses best line)
        queueManager.enqueue();
      } else {
        // Occasionally dequeue from a random line
        final line = _rand.nextInt(queueManager.numberOfLines) + 1;
        if (_rand.nextBool()) {
          queueManager.dequeue(line);
        } else {
          queueManager.enqueueOnLine(line);
        }
      }
    }

    // Generate sensor-ish values per line using C++ data
    final sensorMap = <String, dynamic>{};
    for (var line = 1; line <= queueManager.numberOfLines; line++) {
      sensorMap['line$line'] = {
        'people': queueManager.getLineCount(line),
        'ultrasonic': 100 + _rand.nextInt(60),
      };
    }

    // Use C++ queue logic for all decisions - NO Dart duplicates!
    final queueData = queueManager.toJson();

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
