import 'package:flutter/material.dart';
import 'dart:math' as math;
import '../parameters/app_parameters.dart';

/// Animated ripple waves widget
class AnimatedWaves extends StatelessWidget {
  final AnimationController controller;
  final bool isAnimating;

  const AnimatedWaves({
    super.key,
    required this.controller,
    required this.isAnimating,
  });

  @override
  Widget build(BuildContext context) {
    if (!isAnimating) {
      return _buildStaticCircle();
    }

    return AnimatedBuilder(
      animation: CurvedAnimation(parent: controller, curve: Curves.fastOutSlowIn),
      builder: (context, child) {
        return _buildAnimatedWaves();
      },
    );
  }

  Widget _buildStaticCircle() {
    return Container(
      width: AppParameters.size_maxCircleRadius,
      height: AppParameters.size_maxCircleRadius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: AppParameters.color_primaryBlue.withOpacity(AppParameters.staticCircleOpacity),
      ),
    );
  }

  Widget _buildAnimatedWaves() {
    // All circles grow at same speed, animation ends when Circle 4 reaches 200px
    double progress = controller.value; // 0 to 1
    
    // Staggered start times, all grow at same speed
    double circle1Size = progress * AppParameters.size_waveGrowthSpeed;
    double circle2Size = math.max(0, (progress - AppParameters.circle2StartOffset) * AppParameters.size_waveGrowthSpeed);
    double circle3Size = math.max(0, (progress - AppParameters.circle3StartOffset) * AppParameters.size_waveGrowthSpeed);
    double circle4Size = math.max(0, (progress - AppParameters.circle4StartOffset) * AppParameters.size_waveGrowthSpeed);
    
    // Cap the first circles at max radius so they don't grow too big
    circle1Size = math.min(AppParameters.size_maxCircleRadius, circle1Size);
    circle2Size = math.min(AppParameters.size_maxCircleRadius, circle2Size);
    circle3Size = math.min(AppParameters.size_maxCircleRadius, circle3Size);

    return Stack(
      alignment: Alignment.center,
      children: <Widget>[
        _buildAnimatedContainer(circle1Size, AppParameters.circle1Opacity),
        _buildAnimatedContainer(circle2Size, AppParameters.circle2Opacity),
        _buildAnimatedContainer(circle3Size, AppParameters.circle3Opacity),
        _buildAnimatedContainer(circle4Size, AppParameters.circle4Opacity),
        _buildStaticOuterCircle(),
      ],
    );
  }

  Widget _buildAnimatedContainer(double radius, double startOpacity) {
    // Fade based on animation progress (0 to 1)
    double fadeProgress = controller.value;
    
    // Fade from startOpacity to 0 (completely transparent)
    double targetOpacity = 0.0;
    double currentOpacity = startOpacity + (targetOpacity - startOpacity) * fadeProgress;
    
    return Container(
      width: radius,
      height: radius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: AppParameters.color_primaryBlue.withOpacity(currentOpacity),
      ),
    );
  }

  Widget _buildStaticOuterCircle() {
    return Container(
      width: AppParameters.size_maxCircleRadius,
      height: AppParameters.size_maxCircleRadius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: AppParameters.color_primaryBlue.withOpacity(AppParameters.staticCircleOpacity),
      ),
    );
  }
}
