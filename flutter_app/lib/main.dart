import 'package:flutter/material.dart';
import 'package:flutter_svg/flutter_svg.dart';
import 'dart:math' as math;

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

class QueueScreen extends StatefulWidget {
  const QueueScreen({super.key});

  @override
  State<QueueScreen> createState() => _QueueScreenState();
}

class _QueueScreenState extends State<QueueScreen> with TickerProviderStateMixin {
  late AnimationController _controller;
  late AnimationController _clockController;
  bool _isAnimating = false;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      lowerBound: 0.0,
      upperBound: 1.0, // Normal 0 to 1 animation
      duration: const Duration(seconds: 2),
    );
    
    // Clock animation controller for continuous rotation
    _clockController = AnimationController(
      vsync: this,
      duration: const Duration(seconds: 6), // 6 seconds total
    )..repeat(); // Continuous rotation
  }

  @override
  void dispose() {
    _controller.dispose();
    _clockController.dispose();
    super.dispose();
  }

  void _toggleAnimation() {
    setState(() {
      if (!_isAnimating) {
        _isAnimating = true;
        _controller.reset();
        _controller.forward().then((_) {
          // Animation completed, return to static state
          setState(() {
            _isAnimating = false;
          });
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            const Text(
              'Recommended line',
              style: TextStyle(
                fontSize: 45, 
                fontWeight: FontWeight.w900,
              ),
            ),
            const SizedBox(height: 10),
            // Animated number with ripple waves behind
            SizedBox(
              width: 200,
              height: 200,
              child: Stack(
                alignment: Alignment.center,
                children: [
                  // Animated ripple waves
                  _buildAnimatedWaves(),
                  // The number on top
                  const Text(
                    '3',
                    style: TextStyle(
                      fontSize: 100,
                      fontWeight: FontWeight.bold,
                      color: Color(0xFF3868F6),
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 40),
            // Both boxes in the same row
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                // Place in line with styled container
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                  decoration: BoxDecoration(
                    color: const Color(0xFF3868F6).withOpacity(0.1),
                    borderRadius: BorderRadius.circular(50), // More rounded
                    border: Border.all(
                      color: const Color(0xFF3868F6).withOpacity(0.3),
                      width: 1,
                    ),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      SvgPicture.asset(
                        'assets/images/queue_icon.svg',
                        width: 30,
                        height: 30,
                        colorFilter: const ColorFilter.mode(Color(0xFF3868F6), BlendMode.srcIn),
                      ),
                      const SizedBox(width: 10),
                      const Text(
                        'place in line: 4',
                        style: TextStyle(
                          fontSize: 20, // Smaller to fit in row
                          fontWeight: FontWeight.bold,
                          color: Colors.black, // Black text
                        ),
                      ),
                    ],
                  ),
                ),
                // Wait time with styled container
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                  decoration: BoxDecoration(
                    color: const Color(0xFF3868F6).withOpacity(0.1),
                    borderRadius: BorderRadius.circular(50), // More rounded
                    border: Border.all(
                      color: const Color(0xFF3868F6).withOpacity(0.3),
                      width: 1,
                    ),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      _buildAnimatedClock(),
                      const SizedBox(width: 10),
                      const Text(
                        'wait time: 20 min',
                        style: TextStyle(
                          fontSize: 20, // Smaller to fit in row
                          fontWeight: FontWeight.bold,
                          color: Colors.black, // Black text
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            const SizedBox(height: 20),
            // Button to toggle animation
            ElevatedButton(
              onPressed: _isAnimating ? null : _toggleAnimation, // Disable while animating
              style: ElevatedButton.styleFrom(
                backgroundColor: const Color(0xFF3868F6),
                foregroundColor: Colors.white,
                padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
              ),
              child: Text(
                _isAnimating ? 'Animating...' : 'Start Animation',
                style: const TextStyle(fontSize: 16),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAnimatedWaves() {
    if (!_isAnimating) {
      // Show static circle at max radius when not animating
      return Container(
        width: 200, // Maximum radius
        height: 200,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: const Color(0xFF3868F6).withOpacity(0.05), // Very light static circle
        ),
      );
    }

    return AnimatedBuilder(
      animation: CurvedAnimation(parent: _controller, curve: Curves.fastOutSlowIn),
      builder: (context, child) {
        return Stack(
          alignment: Alignment.center,
          children: <Widget>[
            _buildAnimatedContainer(60 + (70 * _controller.value * 2), 0.4),   // Fades to transparent
            _buildAnimatedContainer(100 + (70 * _controller.value * 2), 0.3),  // Fades to transparent  
            _buildAnimatedContainer(140 + (70 * _controller.value * 2), 0.2),  // Fades to transparent
            _buildContainer(200),  // Outer circle stays at 200px with static color
          ],
        );
      },
    );
  }

  Widget _buildContainer(double radius) {
    return Container(
      width: radius,
      height: radius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: const Color(0xFF3868F6).withOpacity(0.05), // Same as static circle - no fading
      ),
    );
  }

  Widget _buildAnimatedContainer(double radius, double startOpacity) {
    // Fade based on animation progress (0 to 1)
    double fadeProgress = _controller.value;
    
    // Fade from startOpacity to 0 (completely transparent)
    double targetOpacity = 0.0;
    double currentOpacity = startOpacity + (targetOpacity - startOpacity) * fadeProgress;
    
    return Container(
      width: radius,
      height: radius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: const Color(0xFF3868F6).withOpacity(currentOpacity),
      ),
    );
  }

  Widget _buildAnimatedClock() {
    return AnimatedBuilder(
      animation: _clockController,
      builder: (context, child) {
        return SizedBox(
          width: 30,
          height: 30,
          child: CustomPaint(
            painter: AnimatedClockPainter(
              minuteHandAngle: _clockController.value * 6 * 2 * 3.14159, // 6 full rotations
              hourHandAngle: _clockController.value * 2 * 3.14159, // 1 full rotation
              color: const Color(0xFF3868F6),
            ),
          ),
        );
      },
    );
  }
}

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
      ..strokeWidth = 2
      ..style = PaintingStyle.stroke;

    // Draw clock circle
    canvas.drawCircle(center, radius - 1, paint);

    // Draw hour hand (shorter)
    final hourHandLength = radius * 0.5;
    final hourHandX = center.dx + hourHandLength * math.cos(hourHandAngle - 3.14159 / 2);
    final hourHandY = center.dy + hourHandLength * math.sin(hourHandAngle - 3.14159 / 2);
    canvas.drawLine(
      center,
      Offset(hourHandX, hourHandY),
      paint..strokeWidth = 2, // Same thickness as minute hand
    );

    // Draw minute hand (longer)
    final minuteHandLength = radius * 0.7;
    final minuteHandX = center.dx + minuteHandLength * math.cos(minuteHandAngle - 3.14159 / 2);
    final minuteHandY = center.dy + minuteHandLength * math.sin(minuteHandAngle - 3.14159 / 2);
    canvas.drawLine(
      center,
      Offset(minuteHandX, minuteHandY),
      paint..strokeWidth = 2, // Same thickness as hour hand
    );
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) => true;
}
