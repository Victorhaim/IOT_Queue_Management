import 'package:flutter/material.dart';

void main() {
  runApp(const QueueApp());
}

class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Queue Display',
      theme: ThemeData.dark().copyWith(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue),
      ),
      home: const QueueScreen(),
    );
  }
}

class QueueScreen extends StatelessWidget {
  const QueueScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Text(
              'Recommended line',
              style: TextStyle(fontSize: 40, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 10),
            const Text(
              '3',
              style: TextStyle(fontSize: 100, fontWeight: FontWeight.bold, color: Colors.white),
            ),
            const SizedBox(height: 40),
            const Text(
              'expected place in line: 4',
              style: TextStyle(fontSize: 40),
            ),
            const Text(
              'expected waiting time: 20 min',
              style: TextStyle(fontSize: 40),
            ),
          ],
        ),
      ),
    );
  }
}
