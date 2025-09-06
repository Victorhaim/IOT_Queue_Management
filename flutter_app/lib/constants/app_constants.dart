import 'package:flutter/material.dart';

/// Application-wide constants and configuration
class AppConstants {
  // Colors
  static const Color primaryBlue = Color(0xFF3868F6);
  static const Color backgroundColor = Colors.white;
  static const Color textColor = Colors.black;
  static const Color tooltipBackgroundColor = Colors.black87;
  static const Color tooltipTextColor = Colors.white;
  
  // Animation Durations
  static const Duration waveAnimationDuration = Duration(milliseconds: 1300);
  static const Duration clockAnimationDuration = Duration(seconds: 12);
  static const Duration hoverAnimationDuration = Duration(milliseconds: 200);
  
  // Wave Animation Parameters
  static const double maxCircleRadius = 200.0;
  static const double waveGrowthSpeed = 500.0; // pixels per progress unit
  static const double circle1StartOffset = 0.0;
  static const double circle2StartOffset = 0.2;
  static const double circle3StartOffset = 0.4;
  static const double circle4StartOffset = 0.6;
  
  // Circle Opacity Values
  static const double circle1Opacity = 0.4;
  static const double circle2Opacity = 0.3;
  static const double circle3Opacity = 0.2;
  static const double circle4Opacity = 0.1;
  static const double staticCircleOpacity = 0.05;
  
  // Text Styles
  static const String fontFamily = 'EncodeSans';
  static const String expandedFontFamily = 'EncodeSansExpanded';
  
  // UI Measurements
  static const double iconSize = 30.0;
  static const double hoverBoxPadding = 25.0;
  static const double hoverBoxVerticalPadding = 15.0;
  static const double hoverBoxBorderRadius = 50.0;
  static const double hoverBoxSpacing = 20.0;
  static const double tooltipVerticalOffset = 70.0;
  static const double hoverScaleMultiplier = 0.27;
  
  // Clock Settings
  static const double clockCircleStrokeWidth = 2.0;
  static const double hourHandLengthRatio = 0.5;
  static const double minuteHandLengthRatio = 0.7;
  static const int minuteHandRotations = 6;
  static const int hourHandRotations = 1;
}

/// Text content for the application
class AppStrings {
  static const String appTitle = 'Queue Display';
  static const String goToLineText = 'GO TO LINE';
  static const String queueNumber = '3';
  static const String placeInLineText = 'place in line: ';
  static const String placeInLineNumber = '4';
  static const String waitTimeText = 'wait time: ';
  static const String waitTimeNumber = '20';
  static const String waitTimeSuffix = ' min';
  static const String startAnimationText = 'Start Animation';
  static const String animatingText = 'Animating...';
  static const String placeTooltip = 'According to sensor\ndata collection';
  static const String waitTimeTooltip = 'Approximation based on calculations from sensor data collection';
}

/// Asset paths
class AppAssets {
  static const String queueIcon = 'assets/images/queue_icon.svg';
  static const String clockIcon = 'assets/images/clock_icon.svg';
  static const String personIcon = 'assets/images/person_icon.svg';
}
