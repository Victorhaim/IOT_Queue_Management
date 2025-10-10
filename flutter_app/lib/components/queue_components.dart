import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../parameters/app_parameters.dart';
import '../graphics/animated_waves.dart';
import '../graphics/hover_box.dart';
import '../parameters/time_conversion_util.dart';

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

  // Available simulation strategies
  final List<Map<String, String>> _strategies = [
    {'value': 'shortest', 'display': 'Fewest People'},
    {'value': 'farthest', 'display': 'Farthest from Entrance'},
    {'value': 'project', 'display': 'Shortest Wait Time'},
    {'value': 'ESP32', 'display': 'ESP32 Hardware'},
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
    super.dispose();
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
      body: Center(
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
                _buildStrategySelector(),
                SizedBox(height: 20),
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
      padding: EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      decoration: BoxDecoration(
        border: Border.all(color: AppParameters.color_primaryBlue),
        borderRadius: BorderRadius.circular(8),
      ),
      child: DropdownButtonHideUnderline(
        child: DropdownButton<String>(
          value: _selectedStrategy,
          items: _strategies.map((strategy) {
            return DropdownMenuItem<String>(
              value: strategy['value'],
              child: Text(
                'Strategy: ${strategy['display']}',
                style: TextStyle(
                  fontSize: 16,
                  fontWeight: FontWeight.w600,
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
        ),
      ),
    );
  }

  Widget _buildAnimatedNumber() {
    final ref = FirebaseDatabase.instance.ref('simulation_$_selectedStrategy/currentBest/recommendedLine');
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
    final queueRef = FirebaseDatabase.instance.ref('simulation_$_selectedStrategy/currentBest');
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
    final queueRef = FirebaseDatabase.instance.ref('simulation_$_selectedStrategy/currentBest');
    return StreamBuilder<DatabaseEvent>(
      stream: queueRef.onValue,
      builder: (context, snapshot) {
        String waitDisplay = '...';
        String waitSuffix = '';
        String dynamicTooltip = AppStrings.string_waitTimeTooltip;

        if (snapshot.hasData && snapshot.data!.snapshot.value is Map) {
          final raw = snapshot.data!.snapshot.value as Map;
          final averageWaitTime = raw['averageWaitTime'];

          if (averageWaitTime != null) {
            double waitTimeSeconds = (averageWaitTime as num).toDouble();
            final formattedTime = TimeConversionUtil.getFormattedTime(
              waitTimeSeconds,
            );
            waitDisplay = formattedTime['value']!;
            waitSuffix = formattedTime['suffix']!;
          }
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
