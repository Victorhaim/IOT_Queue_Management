import 'dart:async';
import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../parameters/app_parameters.dart';
import '../graphics/animated_waves.dart';
import '../graphics/hover_box.dart';
import '../parameters/time_conversion_util.dart';
import 'connectivity_indicator.dart';

/// Queue app main widget
class QueueApp extends StatelessWidget {
  const QueueApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: AppStrings.string_appTitle,
      theme: ThemeData.light().copyWith(
        colorScheme: ColorScheme.fromSeed(
          seedColor: AppParameters.color_primaryBlue,
        ),
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

class _QueueScreenState extends State<QueueScreen>
    with TickerProviderStateMixin {
  late AnimationController _waveController;
  late AnimationController _clockController;
  late AnimationController _placeHoverController;
  late AnimationController _waitHoverController;

  bool _isAnimating = false;
  bool _isPlaceHovered = false;
  bool _isWaitHovered = false;
  String? _previousLineNumber;
  String _selectedStrategy = 'project'; // Default strategy

  // Countdown timer state
  Timer? _countdownTimer;
  double? _currentWaitTimeSeconds;
  double? _lastFirebaseValue; // Track the last Firebase value we received
  String _countdownDisplay = '...';
  String _countdownSuffix = '';

  // Available simulation strategies
  final List<Map<String, String>> _strategies = [
    {'value': 'project', 'display': 'project'},
    {'value': 'ESP32', 'display': 'ESP32'},
    {'value': 'shortest', 'display': 'Fewest People'},
    {'value': 'farthest', 'display': 'Farthest from Entrance'},
  ];

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
    _countdownTimer?.cancel();
    super.dispose();
  }

  void _startCountdown(double waitTimeSeconds) {
    // Cancel existing timer
    _countdownTimer?.cancel();

    // Set initial values
    _currentWaitTimeSeconds = waitTimeSeconds;
    _lastFirebaseValue = waitTimeSeconds;

    // Update initial display (no setState here!)
    _updateCountdownDisplay();

    // Start countdown timer that checks every minute (60 seconds)
    _countdownTimer = Timer.periodic(Duration(seconds: 60), (timer) {
      if (_currentWaitTimeSeconds == null) {
        timer.cancel();
        return;
      }

      // Convert to minutes for countdown logic
      double currentMinutes = _currentWaitTimeSeconds! / 60;

      // If we're already at or below 1 minute, stop the countdown
      // Only show 0 min if the original Firebase value was exactly 0
      if (currentMinutes <= 1) {
        timer.cancel();
        // Keep the current display as is - don't force to 0 unless original was 0
        if (_lastFirebaseValue == 0) {
          _currentWaitTimeSeconds = 0;
        } else {
          // Keep at minimum 1 minute equivalent in seconds
          _currentWaitTimeSeconds = 60;
        }
        _updateCountdownDisplay();
        if (mounted) {
          setState(() {});
        }
        return;
      }

      // Decrement by 1 minute (60 seconds)
      _currentWaitTimeSeconds = _currentWaitTimeSeconds! - 60;

      if (_currentWaitTimeSeconds! < 0) {
        _currentWaitTimeSeconds = 0;
      }

      _updateCountdownDisplay();
      if (mounted) {
        setState(() {});
      }
    });
  }

  void _updateCountdownDisplay() {
    if (_currentWaitTimeSeconds == null) {
      _countdownDisplay = '...';
      _countdownSuffix = '';
      return;
    }

    final formattedTime = TimeConversionUtil.getFormattedTime(
      _currentWaitTimeSeconds!,
    );
    _countdownDisplay = formattedTime['value']!;
    _countdownSuffix = formattedTime['suffix']!;
  }

  void _triggerAnimationOnChange(String newValue) {
    if (_previousLineNumber != null &&
        _previousLineNumber != newValue &&
        !_isAnimating) {
      // Defer setState to after the build phase completes
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (mounted) {
          setState(() {
            _isAnimating = true;
            _waveController.reset();
            _waveController.forward().then((_) {
              if (mounted) {
                setState(() {
                  _isAnimating = false;
                });
              }
            });
          });
        }
      });
    }
    _previousLineNumber = newValue;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: AppParameters.color_backgroundColor,
      body: Stack(
        children: [
          // Connectivity indicator positioned at top-left
          Positioned(
            top: 40,
            left: 20,
            child: ConnectivityIndicator(
              iconSize: 28,
              showStatusText: true,
            ),
          ),
          // Strategy selector positioned at top-right
          Positioned(top: 40, right: 20, child: _buildStrategySelector()),
          // Main content centered
          Center(
            child: SingleChildScrollView(
              child: Container(
                constraints: BoxConstraints(
                  minHeight: MediaQuery.of(context).size.height,
                ),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  crossAxisAlignment: CrossAxisAlignment.center,
                  children: [
                    SizedBox(height: 30), // Top padding
                    _buildTitle(),
                    SizedBox(height: AppParameters.size_titleToNumberSpacing),
                    _buildAnimatedNumber(),
                    SizedBox(height: AppParameters.size_numberToBoxesSpacing),
                    _buildHoverBoxes(),
                    SizedBox(height: 50), // Bottom padding
                  ],
                ),
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildTitle() {
    return Text(
      AppStrings.string_goToLineText,
      style: TextStyle(
        fontSize: AppParameters.size_titleFontSize,
        fontWeight: FontWeight.w900,
        fontFamily: AppParameters.string_fontFamily,
        color: AppParameters.color_primaryBlue,
      ),
    );
  }

  Widget _buildStrategySelector() {
    return Container(
      padding: EdgeInsets.symmetric(horizontal: 12, vertical: 6),
      decoration: BoxDecoration(
        border: Border.all(color: AppParameters.color_primaryBlue, width: 1),
        borderRadius: BorderRadius.circular(6),
        color: Colors.white.withOpacity(0.9),
      ),
      child: DropdownButtonHideUnderline(
        child: DropdownButton<String>(
          value: _selectedStrategy,
          isDense: true,
          items: _strategies.map((strategy) {
            return DropdownMenuItem<String>(
              value: strategy['value'],
              child: Text(
                strategy['display']!,
                style: TextStyle(
                  fontSize: 12,
                  fontWeight: FontWeight.w500,
                  color: AppParameters.color_primaryBlue,
                ),
              ),
            );
          }).toList(),
          onChanged: (String? newValue) {
            if (newValue != null) {
              setState(() {
                _selectedStrategy = newValue;
              });
            }
          },
          icon: Icon(
            Icons.arrow_drop_down,
            color: AppParameters.color_primaryBlue,
            size: 18,
          ),
        ),
      ),
    );
  }

  Widget _buildAnimatedNumber() {
    final ref = FirebaseDatabase.instance.ref(
      'simulation_$_selectedStrategy/currentBest/recommendedLine',
    );
    return StreamBuilder<DatabaseEvent>(
      stream: ref.onValue,
      builder: (context, snapshot) {
        String display = AppStrings.string_queueNumber;
        if (snapshot.hasData) {
          final val = snapshot.data!.snapshot.value;
          if (val != null) {
            display = val.toString();
            _triggerAnimationOnChange(display);
          }
        }
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
                display,
                style: TextStyle(
                  fontSize: AppParameters.size_queueNumberFontSize,
                  fontWeight: FontWeight.w900,
                  fontFamily: AppParameters.string_expandedFontFamily,
                  color: AppParameters.color_primaryBlue,
                ),
              ),
            ],
          ),
        );
      },
    );
  }

  Widget _buildHoverBoxes() {
    return SingleChildScrollView(
      scrollDirection: Axis.horizontal,
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          _buildDynamicPlaceBox(),
          SizedBox(width: AppParameters.size_hoverBoxSpacing),
          _buildDynamicWaitBox(),
        ],
      ),
    );
  }

  Widget _buildDynamicPlaceBox() {
    final queueRef = FirebaseDatabase.instance.ref(
      'simulation_$_selectedStrategy/currentBest',
    );
    return StreamBuilder<DatabaseEvent>(
      stream: queueRef.onValue,
      builder: (context, snapshot) {
        String placeDisplay = '...';
        String dynamicTooltip = AppStrings.string_placeTooltip;

        if (snapshot.hasData && snapshot.data!.snapshot.value is Map) {
          final raw = snapshot.data!.snapshot.value as Map;
          final recommendedLine = raw['recommendedLine'];
          final currentOccupancy = raw['currentOccupancy'];

          if (currentOccupancy != null && recommendedLine != null) {
            int peopleCount = (currentOccupancy as num).toInt();
            placeDisplay = peopleCount.toString();
          }
        }

        return HoverBox(
          icon: AppAssets.string_queueIcon,
          text: AppStrings.string_placeInLineText,
          number: placeDisplay,
          suffix: '',
          controller: _placeHoverController,
          isHovered: _isPlaceHovered,
          explanation: dynamicTooltip,
          onHover: _handlePlaceHover,
        );
      },
    );
  }

  Widget _buildDynamicWaitBox() {
    final queueRef = FirebaseDatabase.instance.ref(
      'simulation_$_selectedStrategy/currentBest',
    );
    return StreamBuilder<DatabaseEvent>(
      stream: queueRef.onValue,
      builder: (context, snapshot) {
        String waitDisplay = _countdownDisplay;
        String waitSuffix = _countdownSuffix;
        String dynamicTooltip = AppStrings.string_waitTimeTooltip;

        if (snapshot.hasData && snapshot.data!.snapshot.value is Map) {
          final raw = snapshot.data!.snapshot.value as Map;
          final averageWaitTime = raw['averageWaitTime'];

          if (averageWaitTime != null) {
            double waitTimeSeconds = (averageWaitTime as num).toDouble();

            // Only start countdown if Firebase data actually changed
            if (_lastFirebaseValue == null ||
                _lastFirebaseValue != waitTimeSeconds) {
              _startCountdown(waitTimeSeconds);
            }
          } else {
            // No wait time data, show default
            waitDisplay = '...';
            waitSuffix = '';
          }
        } else {
          // No data, show default
          waitDisplay = '...';
          waitSuffix = '';
        }

        // Always use current countdown values if available
        if (_countdownDisplay != '...') {
          waitDisplay = _countdownDisplay;
          waitSuffix = _countdownSuffix;
        }

        return HoverBox(
          icon: AppAssets.string_clockIcon,
          text: AppStrings.string_waitTimeText,
          number: waitDisplay,
          suffix: waitSuffix,
          controller: _waitHoverController,
          isHovered: _isWaitHovered,
          explanation: dynamicTooltip,
          onHover: _handleWaitHover,
          isClockIcon: true,
          clockController: _clockController,
        );
      },
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
