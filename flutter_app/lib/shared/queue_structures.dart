import 'package:firebase_database/firebase_database.dart';

/// Data models representing queue information shared between simulation and real ESP32 systems.
/// These models define the common data structure used across Firebase and both implementations.

/// Represents a complete queue entry with all its associated data.
/// Used by both simulation and real ESP32 implementation to maintain data consistency.
class QueueData {
  final String id;
  final String name;
  final int totalPeople;
  final Map<String, dynamic> sensorData;
  final DateTime? lastUpdated;
  final Map<String, int> lineDistribution; // lineNumber -> peopleCount
  final int? recommendedLine;

  QueueData({
    required this.id,
    required this.name,
    required this.totalPeople,
    required this.sensorData,
    required this.lastUpdated,
    required this.lineDistribution,
    required this.recommendedLine,
  });

  /// Creates a QueueData instance from a Firebase Realtime Database snapshot.
  /// Handles type conversion and provides safe defaults for missing data.
  factory QueueData.fromFirebaseSnapshot(String id, DataSnapshot snapshot) {
    final raw = (snapshot.value as Map?) ?? {};
    return QueueData(
      id: id,
      name: raw['name']?.toString() ?? 'Unnamed Queue',
      totalPeople: _parseAsInt(raw['length'] ?? raw['totalPeople']),
      sensorData: (raw['sensors'] is Map)
          ? (raw['sensors'] as Map).cast<String, dynamic>()
          : <String, dynamic>{},
      lastUpdated: raw['updatedAt'] is int
          ? DateTime.fromMillisecondsSinceEpoch(raw['updatedAt'] as int)
          : null,
      lineDistribution: (raw['lines'] is Map)
          ? (raw['lines'] as Map).map((k, v) => MapEntry(k.toString(), _parseAsInt(v)))
          : <String, int>{},
      recommendedLine: raw['recommendedLine'] != null
          ? _parseAsInt(raw['recommendedLine'])
          : null,
    );
  }

  /// Converts QueueData to a map suitable for Firebase storage.
  Map<String, dynamic> toFirebaseMap() {
    return {
      'name': name,
      'length': totalPeople, // Keep 'length' for backward compatibility
      'totalPeople': totalPeople,
      'sensors': sensorData,
      'lines': lineDistribution.map((k, v) => MapEntry(k, v)),
      'recommendedLine': recommendedLine,
      'updatedAt': lastUpdated?.millisecondsSinceEpoch ?? DateTime.now().millisecondsSinceEpoch,
    };
  }

  /// Creates a copy of this QueueData with updated values.
  QueueData copyWith({
    String? id,
    String? name,
    int? totalPeople,
    Map<String, dynamic>? sensorData,
    DateTime? lastUpdated,
    Map<String, int>? lineDistribution,
    int? recommendedLine,
  }) {
    return QueueData(
      id: id ?? this.id,
      name: name ?? this.name,
      totalPeople: totalPeople ?? this.totalPeople,
      sensorData: sensorData ?? this.sensorData,
      lastUpdated: lastUpdated ?? this.lastUpdated,
      lineDistribution: lineDistribution ?? this.lineDistribution,
      recommendedLine: recommendedLine ?? this.recommendedLine,
    );
  }

  /// Safe integer parsing with fallback to 0.
  static int _parseAsInt(dynamic value) {
    if (value is int) return value;
    if (value is double) return value.toInt();
    if (value is String) return int.tryParse(value) ?? 0;
    return 0;
  }

  @override
  String toString() {
    return 'QueueData(id: $id, name: $name, totalPeople: $totalPeople, lines: $lineDistribution, recommendedLine: $recommendedLine)';
  }
}