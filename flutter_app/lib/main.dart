import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';

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
      theme: ThemeData.light().copyWith(
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
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const SizedBox(width: 20),
                const Text(
                  '3',
                  style: TextStyle(fontSize: 100, fontWeight: FontWeight.bold, color: Color(0xFF3868F6)),
                ),
                const SizedBox(width: 20),
              ],
            ),
            const SizedBox(height: 40),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                SvgPicture.asset(
                  'assets/images/queue_icon.svg',
                  width: 30,
                  height: 30,
                  colorFilter: const ColorFilter.mode(Color(0xFF3868F6), BlendMode.srcIn),
                ),
                const SizedBox(width: 10),
                const Text(
                  'expected place in line: 4',
                  style: TextStyle(fontSize: 40),
                ),
              ],
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                SvgPicture.asset(
                  'assets/images/clock_icon.svg',
                  width: 30,
                  height: 30,
                  colorFilter: const ColorFilter.mode(Color(0xFF3868F6), BlendMode.srcIn),
                ),
                const SizedBox(width: 10),
                const Text(
                  'expected waiting time: 20 min',
                  style: TextStyle(fontSize: 40),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
