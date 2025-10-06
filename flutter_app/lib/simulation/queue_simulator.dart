import 'dart:async';
import 'dart:math';
import '../shared/firebase_queue_service.dart';
import 'queue_manager.dart';

/// Simulates ESP32 queue + sensor updates by writing to Firebase RTDB.
class QueueSimulator {
  QueueSimulator({
    FirebaseQueueService? queueService,
    this.queueId = 'queueA',
    Duration? interval,
    int numberOfLines = 3,
    int maxSize = 0,
  })  : _queueService = queueService ?? FirebaseQueueService(),
        _interval = interval ?? const Duration(seconds: 3),
        _qm = QueueManagerDart(maxSize: maxSize, numberOfLines: numberOfLines);

  final FirebaseQueueService _queueService;
  final String queueId;
  final Duration _interval;
  final QueueManagerDart _qm;
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

  void _writeUpdate() async {
    // Randomly decide operations for each tick: enqueue some users.
    final ops = _rand.nextInt(3) + 1; // 1..3 operations
    for (var i = 0; i < ops; i++) {
      if (_rand.nextBool()) {
        // Enqueue to auto-selected line
        _qm.enqueue();
      } else {
        // Occasionally dequeue from a random line
        final line = _rand.nextInt(_qm.numberOfLines) + 1;
        if (_rand.nextBool()) {
          _qm.dequeue(line);
        } else {
          _qm.enqueueOnLine(line);
        }
      }
    }

    // Generate sensor-ish values per line
    final sensorMap = <String, dynamic>{};
    for (var line = 1; line <= _qm.numberOfLines; line++) {
      sensorMap['line$line'] = {
        'people': _qm.getLineCount(line),
        'ultrasonic': 100 + _rand.nextInt(60),
      };
    }

    final data = _qm.toJson();
    await _queueService.updateQueueFields(queueId, {
      'name': 'Queue Hub (Simulated)',
      'length': data['totalPeople'],
      'totalPeople': data['totalPeople'],
      'lines': data['lines'],
      'recommendedLine': data['recommendedLine'],
      'sensors': sensorMap,
    });
  }
}
