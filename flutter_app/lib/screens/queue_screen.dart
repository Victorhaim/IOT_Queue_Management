import 'package:flutter/material.dart';
import '../constants/app_constants.dart';
import '../widgets/animated_waves.dart';
import '../widgets/hover_box.dart';

/// Main queue display screen
class QueueScreen extends StatefulWidget {
  const QueueScreen({super.key});

  @override
  State<QueueScreen> createState() => _QueueScreenState();
}

class _QueueScreenState extends State<QueueScreen> with TickerProviderStateMixin {
  late AnimationController _waveController;
  late AnimationController _clockController;
  late AnimationController _placeHoverController;
  late AnimationController _waitHoverController;
  
  bool _isAnimating = false;
  bool _isPlaceHovered = false;
  bool _isWaitHovered = false;

  @override
  void initState() {
    super.initState();
    _initializeControllers();
  }

  void _initializeControllers() {
    _waveController = AnimationController(
      vsync: this,
      lowerBound: 0.0,
      upperBound: 1.0,
      duration: AppConstants.waveAnimationDuration,
    );
    
    _clockController = AnimationController(
      vsync: this,
      duration: AppConstants.clockAnimationDuration,
    )..repeat();
    
    _placeHoverController = AnimationController(
      vsync: this,
      duration: AppConstants.hoverAnimationDuration,
    );
    
    _waitHoverController = AnimationController(
      vsync: this,
      duration: AppConstants.hoverAnimationDuration,
    );
  }

  @override
  void dispose() {
    _waveController.dispose();
    _clockController.dispose();
    _placeHoverController.dispose();
    _waitHoverController.dispose();
    super.dispose();
  }

  void _toggleAnimation() {
    setState(() {
      if (!_isAnimating) {
        _isAnimating = true;
        _waveController.reset();
        _waveController.forward().then((_) {
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
      backgroundColor: AppConstants.backgroundColor,
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            _buildTitle(),
            const SizedBox(height: 10),
            _buildAnimatedNumber(),
            const SizedBox(height: 80),
            _buildHoverBoxes(),
            const SizedBox(height: 20),
            _buildAnimationButton(),
          ],
        ),
      ),
    );
  }

  Widget _buildTitle() {
    return const Text(
      AppStrings.goToLineText,
      style: TextStyle(
        fontSize: 50,
        fontWeight: FontWeight.w900,
        fontFamily: AppConstants.fontFamily,
        color: AppConstants.primaryBlue,
      ),
    );
  }

  Widget _buildAnimatedNumber() {
    return SizedBox(
      width: AppConstants.maxCircleRadius,
      height: AppConstants.maxCircleRadius,
      child: Stack(
        alignment: Alignment.center,
        children: [
          AnimatedWaves(
            controller: _waveController,
            isAnimating: _isAnimating,
          ),
          const Text(
            AppStrings.queueNumber,
            style: TextStyle(
              fontSize: 100,
              fontWeight: FontWeight.w900,
              fontFamily: AppConstants.expandedFontFamily,
              color: AppConstants.primaryBlue,
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildHoverBoxes() {
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: [
        HoverBox(
          icon: AppAssets.queueIcon,
          text: AppStrings.placeInLineText,
          number: AppStrings.placeInLineNumber,
          suffix: '',
          controller: _placeHoverController,
          isHovered: _isPlaceHovered,
          explanation: AppStrings.placeTooltip,
          onHover: _handlePlaceHover,
        ),
        const SizedBox(width: AppConstants.hoverBoxSpacing),
        HoverBox(
          icon: AppAssets.clockIcon,
          text: AppStrings.waitTimeText,
          number: AppStrings.waitTimeNumber,
          suffix: AppStrings.waitTimeSuffix,
          controller: _waitHoverController,
          isHovered: _isWaitHovered,
          explanation: AppStrings.waitTimeTooltip,
          onHover: _handleWaitHover,
          isClockIcon: true,
          clockController: _clockController,
        ),
      ],
    );
  }

  Widget _buildAnimationButton() {
    return ElevatedButton(
      onPressed: _isAnimating ? null : _toggleAnimation,
      style: ElevatedButton.styleFrom(
        backgroundColor: AppConstants.primaryBlue,
        foregroundColor: AppConstants.backgroundColor,
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
      ),
      child: Text(
        _isAnimating ? AppStrings.animatingText : AppStrings.startAnimationText,
        style: const TextStyle(fontSize: 16),
      ),
    );
  }

  void _handlePlaceHover(bool hovered) {
    setState(() {
      _isPlaceHovered = hovered;
    });
    if (hovered) {
      _placeHoverController.forward();
    } else {
      _placeHoverController.reverse();
    }
  }

  void _handleWaitHover(bool hovered) {
    setState(() {
      _isWaitHovered = hovered;
    });
    if (hovered) {
      _waitHoverController.forward();
    } else {
      _waitHoverController.reverse();
    }
  }
}
