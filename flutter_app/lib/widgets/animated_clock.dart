import 'package:flutter/material.dart';
import 'dart:math' as math;
import '../parameters/app_parameters.dart';

/// Animated clock widget with rotating hands
class AnimatedClock extends StatelessWidget {
  final AnimationController controller;

  const AnimatedClock({
    super.key,
    required this.controller,
  });

  @override
  Widget build(BuildContext context) {
    return AnimatedBuilder(
      animation: controller,
      builder: (context, child) {
        return SizedBox(
          width: AppParameters.size_iconSize,
          height: AppParameters.size_iconSize,
          child: CustomPaint(
            painter: AnimatedClockPainter(
              minuteHandAngle: controller.value * 
                AppParameters.minuteHandRotations * 2 * math.pi,
              hourHandAngle: controller.value * 
                AppParameters.hourHandRotations * 2 * math.pi,
              color: AppParameters.color_primaryBlue,
            ),
          ),
        );
      },
    );
  }
}

/// Custom painter for the animated clock
class AnimatedClockPainter extends CustomPainter {
  final double minuteHandAngle;
  final double hourHandAngle;
  final Color color;

  AnimatedClockPainter({
    required this.minuteHandAngle,
    required this.hourHandAngle,
    required this.color,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final center = Offset(size.width / 2, size.height / 2);
    final radius = size.width / 2;
    
    final paint = Paint()
      ..color = color
      ..strokeWidth = AppParameters.size_clockCircleStrokeWidth
      ..style = PaintingStyle.stroke;

    // Draw clock circle
    canvas.drawCircle(center, radius - 1, paint);

    // Draw hour hand (shorter)
    final hourHandLength = radius * AppParameters.hourHandLengthRatio;
    final hourHandX = center.dx + hourHandLength * math.cos(hourHandAngle - math.pi / 2);
    final hourHandY = center.dy + hourHandLength * math.sin(hourHandAngle - math.pi / 2);
    canvas.drawLine(
      center,
      Offset(hourHandX, hourHandY),
      paint..strokeWidth = AppParameters.size_clockCircleStrokeWidth,
    );

    // Draw minute hand (longer)
    final minuteHandLength = radius * AppParameters.minuteHandLengthRatio;
    final minuteHandX = center.dx + minuteHandLength * math.cos(minuteHandAngle - math.pi / 2);
    final minuteHandY = center.dy + minuteHandLength * math.sin(minuteHandAngle - math.pi / 2);
    canvas.drawLine(
      center,
      Offset(minuteHandX, minuteHandY),
      paint..strokeWidth = AppParameters.size_clockCircleStrokeWidth,
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
