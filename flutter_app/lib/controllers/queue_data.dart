import 'package:firebase_database/firebase_database.dart';

/// Data model representing a queue entry coming from Realtime Database
class QueueData {
  final String id;
  final String name;
  final int length;
  final Map<String, dynamic> sensors;
  final DateTime? updatedAt;
  final Map<String, int> lines; // lineNumber -> peopleCount
  final int? recommendedLine;

  QueueData({
    required this.id,
    required this.name,
    required this.length,
    required this.sensors,
    required this.updatedAt,
    required this.lines,
    required this.recommendedLine,
  });

  factory QueueData.fromSnapshot(String id, DataSnapshot snapshot) {
    final raw = (snapshot.value as Map?) ?? {};
    return QueueData(
      id: id,
      name: raw['name']?.toString() ?? 'Unknown',
      length: _asInt(raw['length']),
      sensors: (raw['sensors'] is Map)
          ? (raw['sensors'] as Map).cast<String, dynamic>()
          : <String, dynamic>{},
      updatedAt: raw['updatedAt'] is int
          ? DateTime.fromMillisecondsSinceEpoch(raw['updatedAt'] as int)
          : null,
    lines: (raw['lines'] is Map)
      ? (raw['lines'] as Map).map((k, v) => MapEntry(k.toString(), _asInt(v)))
      : <String, int>{},
    recommendedLine: raw['recommendedLine'] != null
      ? _asInt(raw['recommendedLine'])
      : null,
    );
  }

  static int _asInt(dynamic v) {
    if (v is int) return v;
    if (v is double) return v.toInt();
    if (v is String) return int.tryParse(v) ?? 0;
    return 0;
  }
}
