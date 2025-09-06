import 'package:flutter/material.dart';
import 'dart:math' as math;
import '../constants/app_constants.dart';

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
      width: AppConstants.maxCircleRadius,
      height: AppConstants.maxCircleRadius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: AppConstants.primaryBlue.withOpacity(AppConstants.staticCircleOpacity),
      ),
    );
  }

  Widget _buildAnimatedWaves() {
    // All circles grow at same speed, animation ends when Circle 4 reaches 200px
    double progress = controller.value; // 0 to 1
    
    // Staggered start times, all grow at same speed
    double circle1Size = progress * AppConstants.waveGrowthSpeed;
    double circle2Size = math.max(0, (progress - AppConstants.circle2StartOffset) * AppConstants.waveGrowthSpeed);
    double circle3Size = math.max(0, (progress - AppConstants.circle3StartOffset) * AppConstants.waveGrowthSpeed);
    double circle4Size = math.max(0, (progress - AppConstants.circle4StartOffset) * AppConstants.waveGrowthSpeed);
    
    // Cap the first circles at max radius so they don't grow too big
    circle1Size = math.min(AppConstants.maxCircleRadius, circle1Size);
    circle2Size = math.min(AppConstants.maxCircleRadius, circle2Size);
    circle3Size = math.min(AppConstants.maxCircleRadius, circle3Size);

    return Stack(
      alignment: Alignment.center,
      children: <Widget>[
        _buildAnimatedContainer(circle1Size, AppConstants.circle1Opacity),
        _buildAnimatedContainer(circle2Size, AppConstants.circle2Opacity),
        _buildAnimatedContainer(circle3Size, AppConstants.circle3Opacity),
        _buildAnimatedContainer(circle4Size, AppConstants.circle4Opacity),
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
        color: AppConstants.primaryBlue.withOpacity(currentOpacity),
      ),
    );
  }

  Widget _buildStaticOuterCircle() {
    return Container(
      width: AppConstants.maxCircleRadius,
      height: AppConstants.maxCircleRadius,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: AppConstants.primaryBlue.withOpacity(AppConstants.staticCircleOpacity),
      ),
    );
  }
}
