import 'package:flutter/material.dart';
import 'package:firebase_core/firebase_core.dart';
import 'components/queue_components.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp();
  runApp(const QueueApp());
}
