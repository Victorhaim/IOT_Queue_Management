// This is a basic Flutter widget test for the Queue Management app.
//
// To perform an interaction with a widget in your test, use the WidgetTester
// utility in the flutter_test package. For example, you can send tap and scroll
// gestures. You can also use WidgetTester to find child widgets in the widget
// tree, read text, and verify that the values of widget properties are correct.

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

import 'package:flutter_app/components/queue_components.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter_app/firebase_options.dart';

void main() {
  Future<void> _ensureFirebase() async {
    if (Firebase.apps.isEmpty) {
      WidgetsFlutterBinding.ensureInitialized();
      await Firebase.initializeApp(options: DefaultFirebaseOptions.currentPlatform);
    }
  }

  testWidgets('Queue app displays correct initial content', (WidgetTester tester) async {
    await _ensureFirebase();
    // Build our app and trigger a frame.
    await tester.pumpWidget(const QueueApp());

    // Verify that the main title is displayed
    expect(find.text('GO TO LINE'), findsOneWidget);
    
    // Verify that the queue number is displayed
    expect(find.text('3'), findsOneWidget);
    
    // Verify that place in line text is displayed
    expect(find.text('place in line: '), findsOneWidget);
    expect(find.text('4'), findsOneWidget);
    
    // Verify that wait time text is displayed
    expect(find.text('wait time: '), findsOneWidget);
    expect(find.text('20'), findsOneWidget);
    expect(find.text(' min'), findsOneWidget);
    
    // Verify that the animation button is displayed
    expect(find.text('Start Animation'), findsOneWidget);
  });

  testWidgets('Animation button works correctly', (WidgetTester tester) async {
    await _ensureFirebase();
    // Build our app and trigger a frame.
    await tester.pumpWidget(const QueueApp());

    // Find the animation button
    final animationButton = find.text('Start Animation');
    expect(animationButton, findsOneWidget);

    // Tap the animation button
    await tester.tap(animationButton);
    await tester.pump();

    // Verify that button text changes to "Animating..."
    expect(find.text('Animating...'), findsOneWidget);
    expect(find.text('Start Animation'), findsNothing);

    // Wait for animation to complete (1300ms + some buffer)
    await tester.pump(const Duration(milliseconds: 1500));

    // Verify that button text returns to "Start Animation"
    expect(find.text('Start Animation'), findsOneWidget);
    expect(find.text('Animating...'), findsNothing);
  });

  testWidgets('App displays correct queue information', (WidgetTester tester) async {
    await _ensureFirebase();
    // Build our app and trigger a frame.
    await tester.pumpWidget(const QueueApp());

    // Test that all queue-related information is present
    expect(find.textContaining('place in line'), findsOneWidget);
    expect(find.textContaining('wait time'), findsOneWidget);
    expect(find.textContaining('min'), findsOneWidget);
    
    // Verify the queue number is prominently displayed
    expect(find.text('3'), findsOneWidget);
    
    // Verify the place in line number
    expect(find.text('4'), findsOneWidget);
    
    // Verify the wait time number
    expect(find.text('20'), findsOneWidget);
  });

  testWidgets('App structure and widgets are present', (WidgetTester tester) async {
    await _ensureFirebase();
    // Build our app and trigger a frame.
    await tester.pumpWidget(const QueueApp());

    // Verify that the scaffold is present
    expect(find.byType(Scaffold), findsOneWidget);
    
  // Verify that a scroll view exists (allowing for future additions)
  expect(find.byType(SingleChildScrollView), findsAtLeastNWidgets(1));
    
    // Verify that the app has the expected structure
    expect(find.byType(Column), findsAtLeastNWidgets(1));
    expect(find.byType(Text), findsAtLeastNWidgets(5)); // Title, numbers, and labels
  });
}
