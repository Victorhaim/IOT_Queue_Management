import 'package:flutter/material.dart';
import 'constants/app_constants.dart';
import 'screens/queue_screen.dart';

void main() {
  runApp(const QueueApp());
}

class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: AppStrings.appTitle,
      theme: ThemeData.light().copyWith(
        colorScheme: ColorScheme.fromSeed(seedColor: AppConstants.primaryBlue),
      ),
      home: const QueueScreen(),
    );
  }
}
