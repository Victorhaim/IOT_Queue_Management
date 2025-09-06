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

class QueueScreen extends StatefulWidget {
  const QueueScreen({super.key});

  @override
  State<QueueScreen> createState() => _QueueScreenState();
}

class _QueueScreenState extends State<QueueScreen> with TickerProviderStateMixin {
  late AnimationController _controller;
  bool _isAnimating = false;

  @override
  void initState() {
    super.initState();
    _controller = AnimationController(
      vsync: this,
      lowerBound: 0.5,
      duration: const Duration(seconds: 2), // Faster waves
    );
  }

  @override
  void dispose() {
    _controller.dispose();
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
              style: TextStyle(fontSize: 40, fontWeight: FontWeight.bold),
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
                  'place in line: 4',
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
            _buildContainer(60 * _controller.value),   // More waves with smaller increments
            _buildContainer(80 * _controller.value),
            _buildContainer(100 * _controller.value),
            _buildContainer(120 * _controller.value),
            _buildContainer(140 * _controller.value),
            _buildContainer(160 * _controller.value),
            _buildContainer(180 * _controller.value),
            _buildContainer(200 * _controller.value),
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
        color: const Color(0xFF3868F6).withOpacity(0.25 - (_controller.value * 0.2)), // Darker color
      ),
    );
  }
}
