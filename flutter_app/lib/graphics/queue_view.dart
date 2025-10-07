import 'package:flutter/material.dart';
import '../services/firebase_queue_service.dart';
import 'dart:developer' as dev;

/// Displays a single queue's information using a realtime stream.
/// Uses C++ queue logic via Firebase data (no Dart queue logic duplicates)
class QueueView extends StatelessWidget {
  QueueView({
    super.key,
    required this.queueId,
    FirebaseQueueService? queueService,
  }) : _queueService = queueService ?? FirebaseQueueService();

  final String queueId;
  final FirebaseQueueService _queueService;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Queue: $queueId')),
      body: StreamBuilder<Map<String, dynamic>>(
        stream: _queueService.watchQueueUpdatesRaw(queueId),
        builder: (context, snapshot) {
          if (snapshot.connectionState == ConnectionState.waiting) {
            return const Center(child: CircularProgressIndicator());
          }
          if (snapshot.hasError) {
            return Center(child: Text('Error: ${snapshot.error}'));
          }
          if (!snapshot.hasData) {
            return const Center(child: Text('No data'));
          }

          final data = snapshot.data!;

          // Helpers
          int? _asInt(dynamic v) {
            if (v is int) return v;
            if (v is double) return v.toInt();
            if (v is String) return int.tryParse(v);
            return null;
          }

            Map<String, dynamic> _normalizeMap(dynamic raw) {
            if (raw is Map) {
              return raw.map((k, v) => MapEntry(k.toString(), v));
            }
            return const {};
          }

          final name = data['name']?.toString() ?? 'Unknown Queue';
          final totalPeople = _asInt(data['totalPeople']) ?? _asInt(data['length']) ?? 0;
          final recommendedLine = _asInt(data['recommendedLine']);
          final lines = _normalizeMap(data['lines']);
          final sensors = _normalizeMap(data['sensors']);
          final lastUpdatedRaw = data['updatedAt'];
          final lastUpdated = (lastUpdatedRaw is int) ? lastUpdatedRaw : null;

      dev.log('QueueView data update: totalPeople=$totalPeople lines=${lines.length}', name: 'QueueView');

          return Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(name, style: Theme.of(context).textTheme.headlineMedium),
                const SizedBox(height: 12),
                Text(
                  'Length: $totalPeople',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
                if (recommendedLine != null) ...[
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      const Icon(Icons.flag, size: 20),
                      const SizedBox(width: 6),
                      Text('Recommended Line: $recommendedLine'),
                    ],
                  ),
                ],
                const SizedBox(height: 12),
                if (lastUpdated != null)
                  Text('Updated: ${DateTime.fromMillisecondsSinceEpoch(lastUpdated)}')
                else
                  const Text('Updated: -'),
                if (lines.isNotEmpty) ...[
                  const SizedBox(height: 20),
                  Text('Lines', style: Theme.of(context).textTheme.titleMedium),
                  const SizedBox(height: 8),
                  Wrap(
                    spacing: 12,
                    runSpacing: 8,
                    children: lines.entries.map((e) {
                      final lineNum = int.tryParse(e.key) ?? 0;
                      final isRec = recommendedLine == lineNum;
                      return Chip(
                        label: Text(
                          'Line ${e.key}: ${e.value}${isRec ? ' ✓' : ''}',
                        ),
                        backgroundColor: isRec ? Colors.green.shade200 : null,
                      );
                    }).toList(),
                  ),
                ],
                const Divider(height: 32),
                Text('Sensors', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 8),
                Expanded(
                  child: sensors.isEmpty
                      ? const Text('No sensor data')
                      : ListView(
                          children: sensors.entries
                              .map(
                                (e) => ListTile(
                                  dense: true,
                                  title: Text(e.key),
                                  trailing: Text('${e.value}'),
                                ),
                              )
                              .toList(),
                        ),
                ),
                const SizedBox(height: 12),
                ElevatedButton.icon(
                  onPressed: () async {
                    // Simple demo increment using raw Firebase update
                    await _queueService.updateQueueFields(queueId, {
                      'totalPeople': totalPeople + 1,
                      'length': totalPeople + 1,
                    });
                  },
                  icon: const Icon(Icons.add),
                  label: const Text('Increment Length'),
                ),
              ],
            ),
          );
        },
      ),
    );
  }
}
