import 'package:flutter/material.dart';
import '../controllers/queue_repository.dart';
import '../controllers/queue_data.dart';

/// Displays a single queue's information using a realtime stream.
class QueueView extends StatelessWidget {
  QueueView({super.key, required this.queueId, QueueRepository? repository})
      : _repository = repository ?? QueueRepository();

  final String queueId;
  final QueueRepository _repository;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text('Queue: $queueId')),
      body: StreamBuilder<QueueData>(
        stream: _repository.watchQueue(queueId),
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
          final q = snapshot.data!;
          return Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(q.name, style: Theme.of(context).textTheme.headlineMedium),
                const SizedBox(height: 12),
                Text('Length: ${q.length}', style: Theme.of(context).textTheme.titleLarge),
                if (q.recommendedLine != null) ...[
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      const Icon(Icons.flag, size: 20),
                      const SizedBox(width: 6),
                      Text('Recommended Line: ${q.recommendedLine}')
                    ],
                  ),
                ],
                const SizedBox(height: 12),
                if (q.updatedAt != null)
                  Text('Updated: ${q.updatedAt}')
                else
                  const Text('Updated: -'),
                if (q.lines.isNotEmpty) ...[
                  const SizedBox(height: 20),
                  Text('Lines', style: Theme.of(context).textTheme.titleMedium),
                  const SizedBox(height: 8),
                  Wrap(
                    spacing: 12,
                    runSpacing: 8,
                    children: q.lines.entries.map((e) {
                      final isRec = q.recommendedLine == int.tryParse(e.key);
                      return Chip(
                        label: Text('Line ${e.key}: ${e.value}${isRec ? ' ✓' : ''}'),
                        backgroundColor: isRec ? Colors.green.shade200 : null,
                      );
                    }).toList(),
                  ),
                ],
                const Divider(height: 32),
                Text('Sensors', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 8),
                Expanded(
                  child: q.sensors.isEmpty
                      ? const Text('No sensor data')
                      : ListView(
                          children: q.sensors.entries
                              .map((e) => ListTile(
                                    dense: true,
                                    title: Text(e.key),
                                    trailing: Text('${e.value}'),
                                  ))
                              .toList(),
                        ),
                ),
                const SizedBox(height: 12),
                ElevatedButton.icon(
                  onPressed: () async {
                    // Simple demo increment
                    await _repository.updateQueueLength(queueId, q.length + 1);
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
