import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../parameters/app_parameters.dart';
import '../graphics/animated_waves.dart';
import '../graphics/hover_box.dart';

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
        child: SingleChildScrollView(
          child: Container(
            constraints: BoxConstraints(
              minHeight: MediaQuery.of(context).size.height,
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                SizedBox(height: 50), // Top padding
                _buildTitle(),
                SizedBox(height: AppParameters.size_titleToNumberSpacing),
                _buildAnimatedNumber(),
                SizedBox(height: AppParameters.size_numberToBoxesSpacing),
                _buildHoverBoxes(),
                SizedBox(height: AppParameters.size_boxesToButtonSpacing),
                _buildAnimationButton(),
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

  Widget _buildAnimatedNumber() {
    final ref = FirebaseDatabase.instance.ref('currentBest/recommendedLine');
    return StreamBuilder<DatabaseEvent>(
      stream: ref.onValue,
      builder: (context, snapshot) {
        String display = AppStrings.string_queueNumber;
        if (snapshot.hasData) {
          final val = snapshot.data!.snapshot.value;
          if (val != null) {
            display = val.toString();
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
      ),
    );
  }

  Widget _buildDynamicPlaceBox() {
    final queueRef = FirebaseDatabase.instance.ref('currentBest');
    return StreamBuilder<DatabaseEvent>(
      stream: queueRef.onValue,
      builder: (context, snapshot) {
        // Use placeholder instead of static constant so it's obvious when data hasn't arrived
        String placeDisplay = '...';
        String dynamicTooltip = AppStrings.string_placeTooltip;
        String? lineContext; // store recommended line label
        if (snapshot.hasData && snapshot.data!.snapshot.value is Map) {
          final raw = snapshot.data!.snapshot.value as Map;
          final rec = raw['recommendedLine'];
          final lines = raw['lines'];
          final recommendedLineCount =
              raw['recommendedLineCount']; // New field from C++
          if (lines == null) {
            // Attempt a one-time initialization if lines missing
            _initializeQueueNode(queueRef);
            dynamicTooltip = 'Initializing queue lines...';
          }
          int? recommendedLine;
          if (rec is int)
            recommendedLine = rec;
          else if (rec is String)
            recommendedLine = int.tryParse(rec);

          // Use the recommendedLineCount from C++ if available, otherwise compute locally
          if (recommendedLine != null && recommendedLineCount != null) {
            // Use the count directly from C++ simulator
            int currentCount = 0;
            if (recommendedLineCount is int) {
              currentCount = recommendedLineCount;
            } else if (recommendedLineCount is String) {
              currentCount = int.tryParse(recommendedLineCount) ?? 0;
            }

            placeDisplay = (currentCount + 1).toString();
            lineContext = 'L$recommendedLine';
            dynamicTooltip =
                'Recommended line $recommendedLine, current people: $currentCount, you would be #$placeDisplay';
            // ignore: avoid_print
            print(
              '[PlaceBox] rec=$recommendedLine count=$currentCount place=$placeDisplay (from C++)',
            );
          }
          // Fallback to local computation if recommendedLineCount not available
          else if (recommendedLine != null && lines is Map) {
            final key = '${recommendedLine}';
            final currentCountRaw = lines[key];
            final int currentCount = _extractPeopleCount(currentCountRaw);
            placeDisplay = (currentCount + 1).toString();
            lineContext = 'L$recommendedLine';
            dynamicTooltip =
                'Recommended line $recommendedLine, current people: $currentCount, you would be #$placeDisplay';
            // ignore: avoid_print
            print(
              '[PlaceBox] rec=$recommendedLine count=$currentCount place=$placeDisplay (computed locally)',
            );
          }
          // If recommendedLine not provided yet but lines exist, compute locally (mirrors server/device logic)
          else if (recommendedLine == null && lines is Map) {
            int? bestLine;
            int bestCount = 1 << 30;
            lines.forEach((k, v) {
              int count = _extractPeopleCount(v);
              final ln = int.tryParse(k.toString());
              if (ln != null) {
                if (count < bestCount ||
                    (count == bestCount &&
                        (bestLine == null ||
                            (bestLine != null && ln < bestLine!)))) {
                  bestLine = ln;
                  bestCount = count;
                }
              }
            });
            if (bestLine != null) {
              placeDisplay = (bestCount + 1).toString();
              lineContext = 'L$bestLine';
              dynamicTooltip =
                  'Computed recommended line $bestLine, current people: $bestCount, you would be #$placeDisplay';
            }
          }
        } else if (snapshot.hasData && snapshot.data!.snapshot.value == null) {
          // Entire node missing: initialize structure then show initializing state
          _initializeQueueNode(queueRef);
          dynamicTooltip = 'Creating queue structure...';
        }
        return HoverBox(
          icon: AppAssets.string_queueIcon,
          text: lineContext == null
              ? AppStrings.string_placeInLineText
              : '${AppStrings.string_placeInLineText}($lineContext) ',
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

  // Initialize queue node with empty lines if missing to avoid perpetual '...'
  Future<void> _initializeQueueNode(DatabaseReference queueRef) async {
    try {
      final snap = await queueRef.get();
      if (!snap.exists) {
        await queueRef.set({
          'totalPeople': 0,
          'numberOfLines': 2,
          'recommendedLine': 1,
        });
        // ignore: avoid_print
        print('[Init] currentBest created');
      } else {
        final val = snap.value;
        if (val is Map && val['lines'] == null) {
          await queueRef.update({
            'lines': {'1': 0, '2': 0},
            'recommendedLine': 1,
            'updatedAt': ServerValue.timestamp,
          });
          // ignore: avoid_print
          print('[Init] currentBest lines initialized');
        }
      }
    } catch (e) {
      // ignore: avoid_print
      print('[Init] currentBest initialization error: $e');
    }
  }

  // Supports multiple possible formats for a line entry:
  // - int (direct count)
  // - String numeric
  // - Map { 'people': <int|string>, ... }
  // - Map { 'count': <int|string>, ... }
  int _extractPeopleCount(dynamic v) {
    if (v is int) return v;
    if (v is String) return int.tryParse(v) ?? 0;
    if (v is Map) {
      final people = v['people'] ?? v['count'] ?? v['value'];
      if (people is int) return people;
      if (people is String) return int.tryParse(people) ?? 0;
    }
    return 0;
  }

  Widget _buildAnimationButton() {
    return ElevatedButton(
      onPressed: _isAnimating ? null : _toggleAnimation,
      style: ElevatedButton.styleFrom(
        backgroundColor: AppParameters.color_primaryBlue,
        foregroundColor: AppParameters.color_backgroundColor,
        padding: EdgeInsets.symmetric(
          horizontal: AppParameters.size_buttonHorizontalPadding,
          vertical: AppParameters.size_buttonVerticalPadding,
        ),
      ),
      child: Text(
        _isAnimating
            ? AppStrings.string_animatingText
            : AppStrings.string_startAnimationText,
        style: TextStyle(fontSize: AppParameters.size_buttonFontSize),
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
