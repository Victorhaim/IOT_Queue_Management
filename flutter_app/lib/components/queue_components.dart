import 'package:flutter/material.dart';
import '../parameters/app_parameters.dart';
import '../widgets/animated_waves.dart';
import '../widgets/hover_box.dart';

/// Queue app main widget
class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: AppStrings.string_appTitle,
      theme: ThemeData.light().copyWith(
        colorScheme: ColorScheme.fromSeed(seedColor: AppParameters.color_primaryBlue),
      ),
      home: const QueueScreen(),
    );
  }
}

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
      duration: AppParameters.waveAnimationDuration,
    );
    
    _clockController = AnimationController(
      vsync: this,
      duration: AppParameters.clockAnimationDuration,
    )..repeat();
    
    _placeHoverController = AnimationController(
      vsync: this,
      duration: AppParameters.hoverAnimationDuration,
    );
    
    _waitHoverController = AnimationController(
      vsync: this,
      duration: AppParameters.hoverAnimationDuration,
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
      backgroundColor: AppParameters.color_backgroundColor,
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
    return Text(
      AppStrings.string_goToLineText,
      style: TextStyle(
        fontSize: 50,
        fontWeight: FontWeight.w900,
        fontFamily: AppParameters.string_fontFamily,
        color: AppParameters.color_primaryBlue,
      ),
    );
  }

  Widget _buildAnimatedNumber() {
    return SizedBox(
      width: AppParameters.size_maxCircleRadius,
      height: AppParameters.size_maxCircleRadius,
      child: Stack(
        alignment: Alignment.center,
        children: [
          AnimatedWaves(
            controller: _waveController,
            isAnimating: _isAnimating,
          ),
          Text(
            AppStrings.string_queueNumber,
            style: TextStyle(
              fontSize: 100,
              fontWeight: FontWeight.w900,
              fontFamily: AppParameters.string_expandedFontFamily,
              color: AppParameters.color_primaryBlue,
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
          icon: AppAssets.string_queueIcon,
          text: AppStrings.string_placeInLineText,
          number: AppStrings.string_placeInLineNumber,
          suffix: '',
          controller: _placeHoverController,
          isHovered: _isPlaceHovered,
          explanation: AppStrings.string_placeTooltip,
          onHover: _handlePlaceHover,
        ),
        SizedBox(width: AppParameters.size_hoverBoxSpacing),
        HoverBox(
          icon: AppAssets.string_clockIcon,
          text: AppStrings.string_waitTimeText,
          number: AppStrings.string_waitTimeNumber,
          suffix: AppStrings.string_waitTimeSuffix,
          controller: _waitHoverController,
          isHovered: _isWaitHovered,
          explanation: AppStrings.string_waitTimeTooltip,
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
        backgroundColor: AppParameters.color_primaryBlue,
        foregroundColor: AppParameters.color_backgroundColor,
        padding: const EdgeInsets.symmetric(horizontal: 24, vertical: 12),
      ),
      child: Text(
        _isAnimating ? AppStrings.string_animatingText : AppStrings.string_startAnimationText,
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
