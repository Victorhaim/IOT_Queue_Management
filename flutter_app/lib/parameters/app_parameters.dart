import 'package:flutter/material.dart';

/// Application-wide parameters and configuration
class AppParameters {
  // Colors
  static const Color color_primaryBlue = Color(0xFF3868F6);
  static const Color color_backgroundColor = Colors.white;
  static const Color color_textColor = Colors.black;
  static const Color color_tooltipBackgroundColor = Colors.black87;
  static const Color color_tooltipTextColor = Colors.white;

  // Animation Durations
  static const Duration waveAnimationDuration = Duration(milliseconds: 1300);
  static const Duration clockAnimationDuration = Duration(seconds: 12);
  static const Duration hoverAnimationDuration = Duration(milliseconds: 200);

  // Wave Animation Parameters
  static const double size_maxCircleRadius = 200.0;
  static const double size_waveGrowthSpeed = 500.0; // pixels per progress unit
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

  // UI Opacity Values
  static const double hoverBoxBackgroundOpacity = 0.1;
  static const double hoverBoxBorderOpacity = 0.3;
  static const double tooltipBackgroundOpacity = 0.8;

  // Text Styles
  static const String string_fontFamily = 'EncodeSans';
  static const String string_expandedFontFamily = 'EncodeSansExpanded';

  // UI Measurements
  static const double size_iconSize = 30.0;
  static const double size_hoverBoxPadding = 25.0;
  static const double size_hoverBoxVerticalPadding = 15.0;
  static const double size_hoverBoxBorderRadius = 50.0;
  static const double size_hoverBoxSpacing = 20.0;
  static const double size_tooltipVerticalOffset = 70.0;
  static const double hoverScaleMultiplier = 0.27;

  // Layout Spacing
  static const double size_titleToNumberSpacing = 10.0;
  static const double size_numberToBoxesSpacing = 80.0;
  static const double size_boxesToButtonSpacing = 20.0;
  static const double size_iconToTextSpacing = 10.0;
  static const double size_textSpacing = 2.0;

  // Font Sizes
  static const double size_titleFontSize = 50.0;
  static const double size_queueNumberFontSize = 100.0;
  static const double size_hoverBoxFontSize = 22.0;
  static const double size_buttonFontSize = 16.0;
  static const double size_tooltipFontSize = 12.0;

  // Button Properties
  static const double size_buttonHorizontalPadding = 24.0;
  static const double size_buttonVerticalPadding = 12.0;

  // Tooltip Properties
  static const double size_tooltipHorizontalPadding = 12.0;
  static const double size_tooltipVerticalPadding = 6.0;
  static const double size_tooltipBorderRadius = 8.0;

  // Border Properties
  static const double size_borderWidth = 1.0;

  // Clock Settings
  static const double size_clockCircleStrokeWidth = 2.0;
  static const double hourHandLengthRatio = 0.5;
  static const double minuteHandLengthRatio = 0.7;
  static const int minuteHandRotations = 6;
  static const int hourHandRotations = 1;
  static const double size_clockRadiusOffset = 1.0;
}

/// Text content for the application
class AppStrings {
  static const String string_appTitle = 'Queue Display';
  static const String string_goToLineText = 'GO TO LINE';
  static const String string_queueNumber = '';
  static const String string_placeInLineText = 'people before you: ';
  static const String string_placeInLineNumber = '';
  static const String string_waitTimeText = 'wait time: ';
  static const String string_waitTimeNumber = '';
  static const String string_startAnimationText = 'Start Animation';
  static const String string_animatingText = 'Animating...';
  static const String string_placeTooltip =
      'According to sensor\ndata collection';
  static const String string_waitTimeTooltip =
      'Approximation based on calculations from sensor data collection';
}

/// Asset paths
class AppAssets {
  static const String string_queueIcon = 'assets/images/queue_icon.svg';
  static const String string_clockIcon = 'assets/images/clock_icon.svg';
  static const String string_personIcon = 'assets/images/person_icon.svg';
}
